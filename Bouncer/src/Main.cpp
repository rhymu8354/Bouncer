/**
 * @file Main.cpp
 *
 * This module contains the implementation of the Bouncer::Main class.
 *
 * © 2019 by Richard Walters
 */

#include "TimeKeeper.hpp"

#include <AsyncData/MultiProducerSingleConsumerQueue.hpp>
#include <Bouncer/Main.hpp>
#include <condition_variable>
#include <functional>
#include <future>
#include <Http/Client.hpp>
#include <HttpNetworkTransport/HttpClientNetworkTransport.hpp>
#include <inttypes.h>
#include <Json/Value.hpp>
#include <map>
#include <math.h>
#include <memory>
#include <mutex>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <thread>
#include <TlsDecorator/TlsDecorator.hpp>
#include <Twitch/Messaging.hpp>
#include <TwitchNetworkTransport/Connection.hpp>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <SystemAbstractions/File.hpp>
#include <SystemAbstractions/NetworkConnection.hpp>
#include <SystemAbstractions/StringExtensions.hpp>

namespace {

    const auto configurationFilePath = SystemAbstractions::File::GetExeParentDirectory() + "/Bouncer.json";
    constexpr double twitchApiLookupCooldown = 1.0;
    constexpr double configurationAutoSaveCooldown = 60.0;
    constexpr size_t maxTwitchUserLookupsByLogin = 100;

    template< typename T > void WithoutLock(
        T& lock,
        std::function< void() > f
    ) {
        lock.unlock();
        f();
        lock.lock();
    }

    /**
     * This function loads the contents of the file with the given path
     * into the given string.
     *
     * @param[in] filePath
     *     This is the path of the file to load.
     *
     * @param[in] fileDescription
     *     This is a description of the file being loaded, used in any
     *     diagnostic messages published by the function.
     *
     * @param[in] diagnosticsSender
     *     This is the object to use to publish any diagnostic messages.
     *
     * @param[out] fileContents
     *     This is where to store the file's contents.
     *
     * @return
     *     An indication of whether or not the function succeeded is returned.
     */
    bool LoadFile(
        const std::string& filePath,
        const std::string& fileDescription,
        const SystemAbstractions::DiagnosticsSender& diagnosticsSender,
        std::string& fileContents
    ) {
        SystemAbstractions::File file(filePath);
        if (file.Open()) {
            std::vector< uint8_t > fileContentsAsVector(file.GetSize());
            if (file.Read(fileContentsAsVector) != fileContentsAsVector.size()) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "Unable to read %s file '%s'",
                    fileDescription.c_str(),
                    filePath.c_str()
                );
                return false;
            }
            (void)fileContents.assign(
                (const char*)fileContentsAsVector.data(),
                fileContentsAsVector.size()
            );
        } else {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "Unable to open %s file '%s'",
                fileDescription.c_str(),
                filePath.c_str()
            );
            return false;
        }
        return true;
    }

    /**
     * Convert the given time in UTC to the equivalent number of seconds
     * since the UNIX epoch (Midnight UTC January 1, 1970).
     *
     * @param[in] timestamp
     *     This is the timestamp to convert.
     *
     * @return
     *     The equivalent timetamp in seconds since the UNIX epoch
     *     is returned.
     */
    double ParseTimestamp(const std::string& timestamp) {
        int years, months, days, hours, minutes, seconds;
        (void)sscanf(
            timestamp.c_str(),
            "%d-%d-%dT%d:%d:%dZ",
            &years,
            &months,
            &days,
            &hours,
            &minutes,
            &seconds
        );
        static const auto isLeapYear = [](int year){
            if ((year % 4) != 0) { return false; }
            if ((year % 100) != 0) { return true; }
            return ((year % 400) == 0);
        };
        auto total = (intmax_t)seconds;
        for (int yy = 1970; yy < years; ++yy) {
            total += (isLeapYear(yy) ? 366 : 365) * 86400;
        }
        static const int daysPerMonth[] = {
            31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
        };
        for (int mm = 1; mm < months; ++mm) {
            total += daysPerMonth[mm - 1] * 86400;
            if (
                (mm == 2)
                && isLeapYear(years)
            ) {
                total += 86400;
            }
        }
        total += (days - 1) * 86400;
        total += hours * 3600;
        total += minutes * 60;
        return (double)total;
    }

    /**
     * This function saves the given string as the contents of the file with
     * the given path.
     *
     * @param[in] filePath
     *     This is the path of the file to save.
     *
     * @param[in] fileDescription
     *     This is a description of the file being saved, used in any
     *     diagnostic messages published by the function.
     *
     * @param[in] diagnosticsSender
     *     This is the object to use to publish any diagnostic messages.
     *
     * @param[in] fileContents
     *     This is the contents to store in the file.
     *
     * @return
     *     An indication of whether or not the function succeeded is returned.
     */
    bool SaveFile(
        const std::string& filePath,
        const std::string& fileDescription,
        const SystemAbstractions::DiagnosticsSender& diagnosticsSender,
        const std::string& fileContents
    ) {
        SystemAbstractions::File file(filePath);
        if (file.Create()) {
            SystemAbstractions::IFile::Buffer buffer(
                fileContents.begin(),
                fileContents.end()
            );
            if (file.Write(buffer) != buffer.size()) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "Unable to write %s file '%s'",
                    fileDescription.c_str(),
                    filePath.c_str()
                );
                return false;
            }
            if (!file.SetSize(buffer.size())) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "Unable to set size of %s file '%s'",
                    fileDescription.c_str(),
                    filePath.c_str()
                );
                return false;
            }
        } else {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "Unable to create %s file '%s'",
                fileDescription.c_str(),
                filePath.c_str()
            );
            return false;
        }
        return true;
    }

}

namespace Bouncer {

    /**
     * This contains the private properties of a Main instance.
     */
    struct Main::Impl {
        // Types

        enum class State {
            Unconfigured,
            Unconnected,
            OutsideRoom,
            InsideRoom,
        };

        struct StatusMessage {
            size_t level;
            std::string message;
        };

        /**
         * This is the object given to the Twitch messaging interface
         * to be used to receive callbacks.
         */
        struct TwitchDelegate
            : public Twitch::Messaging::User
        {
            // Properties

            std::weak_ptr< Impl > implWeak;

            // Twitch::Messaging::User

            virtual void LogIn() override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
                impl->OnLogIn();
            }

            virtual void LogOut() override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
                impl->OnLogOut();
            }

            virtual void Join(Twitch::Messaging::MembershipInfo&& membershipInfo) override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
                impl->OnJoin(std::move(membershipInfo));
            }

            virtual void Leave(Twitch::Messaging::MembershipInfo&& membershipInfo) override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
                impl->OnLeave(std::move(membershipInfo));
            }

            virtual void NameList(Twitch::Messaging::NameListInfo&& nameListInfo) override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
                impl->OnNameList(std::move(nameListInfo));
            }

            virtual void Message(Twitch::Messaging::MessageInfo&& messageInfo) override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
            }

            virtual void PrivateMessage(Twitch::Messaging::MessageInfo&& messageInfo) override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
            }

            virtual void Whisper(Twitch::Messaging::WhisperInfo&& whisperInfo) override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
            }

            virtual void Notice(Twitch::Messaging::NoticeInfo&& noticeInfo) override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
            }

            virtual void Host(Twitch::Messaging::HostInfo&& hostInfo) override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
            }

            virtual void Clear(Twitch::Messaging::ClearInfo&& clearInfo) override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
            }

            virtual void Mod(Twitch::Messaging::ModInfo&& modInfo) override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
            }

            virtual void UserState(Twitch::Messaging::UserStateInfo&& userStateInfo) override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
            }

            virtual void Sub(Twitch::Messaging::SubInfo&& subInfo) override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
            }

            virtual void Raid(Twitch::Messaging::RaidInfo&& raidInfo) override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
            }

            virtual void Ritual(Twitch::Messaging::RitualInfo&& ritualInfo) override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
            }

        };

        struct User {
            std::string login;
            std::string name;
            double createdAt = 0.0;
            double totalViewTime = 0.0;
            double joinTime = 0.0;
            double partTime = 0.0;
            double lastMessageTime = 0.0;
            size_t numMessages = 0;
            double timeout = 0.0;
            bool isBanned = false;
            bool isJoined = false;
            enum class Bot {
                Unknown,
                Yes,
                No,
            } bot = Bot::Unknown;
        };

        // Properties

        bool apiCallInProgress = false;
        AsyncData::MultiProducerSingleConsumerQueue< std::function< void() > > apiCalls;
        Configuration configuration;
        bool configurationChanged = false;
        std::mutex diagnosticsMutex;
        SystemAbstractions::DiagnosticsSender diagnosticsSender;
        std::thread diagnosticsWorker;
        std::shared_ptr< Host > host;
        std::shared_ptr< Http::Client > httpClient = std::make_shared< Http::Client >();

        /**
         * This holds onto pending HTTP request transactions being made.
         *
         * The keys are unique identifiers.  The values are the transactions.
         */
        std::map< int, std::shared_ptr< Http::IClient::Transaction > > httpClientTransactions;

        std::promise< void > loggedOut;
        std::recursive_mutex mutex;
        double nextApiCallTime = 0.0;
        double nextConfigurationAutoSaveTime = 0.0;

        /**
         * This is used to select unique identifiers as keys for the
         * httpClientTransactions collection.  It is incremented each
         * time a key is selected.
         */
        int nextHttpClientTransactionId = 1;

        std::weak_ptr< Impl > selfWeak;
        State state = State::Unconfigured;
        AsyncData::MultiProducerSingleConsumerQueue< StatusMessage > statusMessages;
        bool stopDiagnosticsWorker = false;
        bool stopWorker = false;
        std::map< std::string, intmax_t > userIdsByLogin;
        std::map< std::string, double > userJoinsByLogin;
        AsyncData::MultiProducerSingleConsumerQueue< std::string > userLookupsByLogin;
        bool userLookupsPending = false;
        std::map< intmax_t, User > usersById;
        std::condition_variable wakeDiagnosticsWorker;
        std::condition_variable_any wakeWorker;
        std::thread worker;


        /**
         * This is used to interact with Twitch.
         */
        Twitch::Messaging tmi;

        /**
         * This is the object given to the Twitch messaging interface to be
         * used to receive callbacks.  It holds state shared between Twitch and
         * the Golem.
         */
        std::shared_ptr< TwitchDelegate > twitchDelegate = std::make_shared< TwitchDelegate >();

        /**
         * This is used to track time for the Twitch interface.
         */
        std::shared_ptr< TimeKeeper > timeKeeper = std::make_shared< TimeKeeper >();

        // Methods

        Impl()
            : diagnosticsSender("Bouncer")
        {
            diagnosticsSender.SubscribeToDiagnostics(
                [this](
                    std::string senderName,
                    size_t level,
                    std::string message
                ){
                    StatusMessage statusMessage;
                    statusMessage.level = level;
                    statusMessage.message = message;
                    std::lock_guard< decltype(diagnosticsMutex) > lock(diagnosticsMutex);
                    statusMessages.Add(std::move(statusMessage));
                    wakeDiagnosticsWorker.notify_one();
                }
            );
        }

        void AwaitLogOut() {
            auto wasLoggedOut = loggedOut.get_future();
            (void)wasLoggedOut.wait_for(std::chrono::seconds(1));
        }

        void DiagnosticsWorker() {
            std::unique_lock< decltype(diagnosticsMutex) > lock(diagnosticsMutex);
            while (!stopDiagnosticsWorker) {
                WithoutLock(lock, [&]{ PublishMessages(); });
                const auto workerWakeCondition = [this]{
                    return (
                        stopDiagnosticsWorker
                        || !statusMessages.IsEmpty()
                    );
                };
                wakeDiagnosticsWorker.wait(lock, workerWakeCondition);
            }
            WithoutLock(lock, [&]{ PublishMessages(); });
        }

        void LoadConfiguration() {
            nextConfigurationAutoSaveTime = timeKeeper->GetCurrentTime() + configurationAutoSaveCooldown;
            std::string encodedConfiguration;
            if (
                !LoadFile(
                    configurationFilePath,
                    "configuration",
                    diagnosticsSender,
                    encodedConfiguration
                )
            ) {
                return;
            }
            const auto json = Json::Value::FromEncoding(encodedConfiguration);
            if (json.GetType() == Json::Value::Type::Invalid) {
                diagnosticsSender.SendDiagnosticInformationString(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "Unable to parse configuration file"
                );
                return;
            }
            configuration.account = (std::string)json["account"];
            configuration.token = (std::string)json["token"];
            configuration.clientId = (std::string)json["clientId"];
            configuration.channel = (std::string)json["channel"];
            configuration.newAccountAgeThreshold = (double)json["newAccountAgeThreshold"];
            const auto& whitelist = json["whitelist"];
            configuration.whitelist.clear();
            const auto numWhitelistEntries = whitelist.GetSize();
            configuration.whitelist.reserve(numWhitelistEntries);
            for (size_t i = 0; i < numWhitelistEntries; ++i) {
                configuration.whitelist.push_back((std::string)whitelist[i]);
            }
            const auto& users = json["users"];
            userIdsByLogin.clear();
            userJoinsByLogin.clear();
            usersById.clear();
            const auto numUsers = users.GetSize();
            for (size_t i = 0; i < numUsers; ++i) {
                const auto& userEncoded = users[i];
                const auto userid = (intmax_t)(size_t)userEncoded["id"];
                auto& user = usersById[userid];
                user.login = (std::string)userEncoded["login"];
                user.name = (std::string)userEncoded["name"];
                user.createdAt = (double)userEncoded["createdAt"];
                user.totalViewTime = (double)userEncoded["totalViewTime"];
                user.lastMessageTime = (double)userEncoded["lastMessageTime"];
                user.numMessages = (size_t)userEncoded["numMessages"];
                user.timeout = (double)userEncoded["timeout"];
                user.isBanned = (bool)userEncoded["isBanned"];
                if (userEncoded.Has("bot")) {
                    const auto bot = (std::string)userEncoded["bot"];
                    if (bot == "yes") {
                        user.bot = User::Bot::Yes;
                    } else if (bot == "no") {
                        user.bot = User::Bot::No;
                    }
                }
                userIdsByLogin[user.login] = userid;
            }
            configurationChanged = true;
        }

        void LogIn() {
            state = State::Unconnected;
            loggedOut = decltype(loggedOut)();
            tmi.LogIn(
                configuration.account,
                configuration.token
            );
        }

        void HandleConfigurationChanged() {
            const bool isConfigured = (
                !configuration.account.empty()
                && !configuration.token.empty()
                && !configuration.clientId.empty()
                && !configuration.channel.empty()
            );
            if (isConfigured) {
                switch (state) {
                    case State::Unconfigured: {
                        PostStatus("Now configured");
                        LogIn();
                    } break;

                    case State::Unconnected: {
                        PostStatus("Reconfigured");
                        LogIn();
                    } break;

                    default: break;
                }
            } else {
                if (state != State::Unconfigured) {
                    state = State::Unconfigured;
                    PostStatus("No longer configured");
                }
            }
        }

        void NextApiCall() {
            if (apiCalls.IsEmpty()) {
                nextApiCallTime = 0.0;
            } else {
                const auto apiCall = apiCalls.Remove();
                apiCall();
            }
        }

        void NotifyStopDiagnosticsWorker() {
            std::lock_guard< decltype(diagnosticsMutex) > lock(diagnosticsMutex);
            stopDiagnosticsWorker = true;
            wakeDiagnosticsWorker.notify_one();
        }

        void NotifyStopWorker() {
            std::lock_guard< decltype(mutex) > lock(mutex);
            stopWorker = true;
            wakeWorker.notify_one();
        }

        void OnLogIn() {
            std::lock_guard< decltype(mutex) > lock(mutex);
            PostStatus("Logged in");
            if (state == State::Unconnected) {
                state = State::OutsideRoom;
                tmi.Join(configuration.channel);
            }
        }

        void OnLogOut() {
            std::lock_guard< decltype(mutex) > lock(mutex);
            switch (state) {
                case State::OutsideRoom:
                case State::InsideRoom: {
                    loggedOut.set_value();
                    PostStatus("Logged out");
                    state = State::Unconnected;
                } break;

                default: break;
            }
        }

        void OnJoin(Twitch::Messaging::MembershipInfo&& membershipInfo) {
            std::lock_guard< decltype(mutex) > lock(mutex);
            if (membershipInfo.user == configuration.account) {
                PostStatus("Joined room");
                state = State::InsideRoom;
                //tmi.SendMessage(
                //    configuration.channel,
                //    "Hello, World!"
                //);
            } else {
                UsersJoined({std::move(membershipInfo.user)});
            }
        }

        void OnLeave(Twitch::Messaging::MembershipInfo&& membershipInfo) {
            std::lock_guard< decltype(mutex) > lock(mutex);
            if (membershipInfo.user == configuration.account) {
                PostStatus("Left room");
                state = State::OutsideRoom;
            } else {
                auto userIdsByLoginEntry = userIdsByLogin.find(membershipInfo.user);
                if (userIdsByLoginEntry != userIdsByLogin.end()) {
                    const auto userid = userIdsByLoginEntry->second;
                    auto& user = usersById[userid];
                    if (user.isJoined) {
                        UserParted(
                            userid,
                            timeKeeper->GetCurrentTime()
                        );
                    }
                }
            }
        }

        void OnNameList(Twitch::Messaging::NameListInfo&& nameListInfo) {
            std::lock_guard< decltype(mutex) > lock(mutex);
            UsersJoined(std::move(nameListInfo.names));
        }

        void PostApiCall(std::function< void() > apiCall) {
            apiCalls.Add(apiCall);
            wakeWorker.notify_one();
        }

        void PostApiCall(
            const std::string& targetUriString,
            std::function<
                void(
                    Impl& impl,
                    int id,
                    const Http::IClient::Transaction& transaction
                )
            > onCompletion
        ) {
            PostApiCall(
                [
                    onCompletion,
                    targetUriString,
                    this
                ]{
                    apiCallInProgress = true;
                    const auto id = nextHttpClientTransactionId++;
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        2,
                        "Twitch API call %d: %s",
                        id,
                        targetUriString.c_str()
                    );
                    Http::Request request;
                    request.method = "GET";
                    request.target.ParseFromString(targetUriString);
                    request.target.SetPort(443);
                    request.headers.SetHeader("Client-ID", configuration.clientId);
                    request.headers.SetHeader("Accept", "application/vnd.twitchtv.v5+json");
                    auto& httpClientTransaction = httpClientTransactions[id];
                    httpClientTransaction = httpClient->Request(request);
                    auto selfWeakCopy(selfWeak);
                    httpClientTransaction->SetCompletionDelegate(
                        [
                            id,
                            onCompletion,
                            selfWeakCopy
                        ]{
                            auto impl = selfWeakCopy.lock();
                            if (impl == nullptr) {
                                return;
                            }
                            std::lock_guard< decltype(impl->mutex) > lock(impl->mutex);
                            impl->apiCallInProgress = false;
                            impl->nextApiCallTime = impl->timeKeeper->GetCurrentTime() + twitchApiLookupCooldown;
                            impl->wakeWorker.notify_one();
                            auto httpClientTransactionsEntry = impl->httpClientTransactions.find(id);
                            if (httpClientTransactionsEntry == impl->httpClientTransactions.end()) {
                                return;
                            }
                            const auto& httpClientTransaction = httpClientTransactionsEntry->second;
                            onCompletion(*impl, id, *httpClientTransaction);
                            (void)impl->httpClientTransactions.erase(httpClientTransactionsEntry);
                            if (impl->userLookupsByLogin.IsEmpty()) {
                                impl->userLookupsPending = false;
                            } else {
                                impl->PostUserLookupsByLogin();
                            }
                        }
                    );
                }
            );
        }

        void PostStatus(const std::string& message) {
            diagnosticsSender.SendDiagnosticInformationString(3, message);
        }

        void PublishMessages() {
            while (!statusMessages.IsEmpty()) {
                const auto statusMessage = statusMessages.Remove();
                host->StatusMessage(
                    statusMessage.level,
                    statusMessage.message
                );
            }
        }

        void SaveConfiguration() {
            nextConfigurationAutoSaveTime = timeKeeper->GetCurrentTime() + configurationAutoSaveCooldown;
            auto json = Json::Object({
                {"account", configuration.account},
                {"token", configuration.token},
                {"clientId", configuration.clientId},
                {"channel", configuration.channel},
                {"newAccountAgeThreshold", configuration.newAccountAgeThreshold},
                {"whitelist", Json::Array({})},
                {"users", Json::Array({})},
            });
            auto& whitelist = json["whitelist"];
            for (const auto& whitelistEntry: configuration.whitelist) {
                whitelist.Add(whitelistEntry);
            }
            auto& users = json["users"];
            for (const auto& usersByIdEntry: usersById) {
                auto userEncoded = Json::Object({
                    {"id", (size_t)usersByIdEntry.first},
                    {"login", usersByIdEntry.second.login},
                    {"name", usersByIdEntry.second.name},
                    {"createdAt", usersByIdEntry.second.createdAt},
                    {"totalViewTime", usersByIdEntry.second.totalViewTime},
                    {"lastMessageTime", usersByIdEntry.second.lastMessageTime},
                    {"numMessages", usersByIdEntry.second.numMessages},
                    {"timeout", usersByIdEntry.second.timeout},
                    {"isBanned", usersByIdEntry.second.isBanned},
                });
                switch (usersByIdEntry.second.bot) {
                    case User::Bot::Yes: {
                        userEncoded["bot"] = "yes";
                    } break;

                    case User::Bot::No: {
                        userEncoded["bot"] = "no";
                    } break;

                    default: break;
                }
                users.Add(std::move(userEncoded));
            }

            Json::EncodingOptions jsonEncodingOptions;
            jsonEncodingOptions.pretty = true;
            jsonEncodingOptions.reencode = true;
            (void)SaveFile(
                configurationFilePath,
                "configuration",
                diagnosticsSender,
                json.ToEncoding(jsonEncodingOptions)
            );
        }

        void StartDiagnosticsWorker() {
            if (diagnosticsWorker.joinable()) {
                return;
            }
            std::lock_guard< decltype(diagnosticsMutex) > lock(diagnosticsMutex);
            stopDiagnosticsWorker = false;
            diagnosticsWorker = std::thread(&Impl::DiagnosticsWorker, this);
        }

        void StartWorker() {
            if (worker.joinable()) {
                return;
            }
            std::lock_guard< decltype(mutex) > lock(mutex);
            stopWorker = false;
            worker = std::thread(&Impl::Worker, this);
        }

        void StopDiagnosticsWorker() {
            if (!diagnosticsWorker.joinable()) {
                return;
            }
            NotifyStopDiagnosticsWorker();
            diagnosticsWorker.join();
        }

        void StopWorker() {
            if (!worker.joinable()) {
                return;
            }
            NotifyStopWorker();
            worker.join();
        }

        void UserParted(
            intmax_t userid,
            double partTime
        ) {
            auto& user = usersById[userid];
            user.isJoined = false;
            user.partTime = partTime;
            diagnosticsSender.SendDiagnosticInformationFormatted(
                2,
                "User %" PRIdMAX " (%s) has parted",
                userid,
                user.login.c_str()
            );
        }

        void UserJoined(
            intmax_t userid,
            double joinTime
        ) {
            auto& user = usersById[userid];
            user.isJoined = true;
            user.joinTime = joinTime;
            diagnosticsSender.SendDiagnosticInformationFormatted(
                2,
                "User %" PRIdMAX " (%s) has joined (account age: %lf)",
                userid,
                user.login.c_str(),
                timeKeeper->GetCurrentTime() - user.createdAt
            );
        }

        void UsersJoined(const std::vector< std::string >& logins) {
            const auto joinTime = timeKeeper->GetCurrentTime();
            for (const auto& login: logins) {
                auto userIdsByLoginEntry = userIdsByLogin.find(login);
                if (userIdsByLoginEntry == userIdsByLogin.end()) {
                    userLookupsByLogin.Add(login);
                    userJoinsByLogin[login] = joinTime;
                } else {
                    const auto userid = userIdsByLoginEntry->second;
                    auto& user = usersById[userid];
                    if (!user.isJoined) {
                        UserJoined(userid, joinTime);
                    }
                }
            }
            if (
                userLookupsByLogin.IsEmpty()
                || userLookupsPending
            ) {
                return;
            }
            PostUserLookupsByLogin();
        }

        void PostUserLookupsByLogin() {
            std::set< std::string > logins;
            while (
                !userLookupsByLogin.IsEmpty()
                && (logins.size() < maxTwitchUserLookupsByLogin)
            ) {
                (void)logins.insert(userLookupsByLogin.Remove());
            }
            userLookupsPending = true;
            std::string targetUriString = "https://api.twitch.tv/kraken/users?login=";
            bool first = true;
            for (const auto& login: logins) {
                if (first) {
                    first = false;
                } else {
                    targetUriString += ",";
                }
                targetUriString += login;
            }
            PostApiCall(
                targetUriString,
                [](
                    Impl& impl,
                    int id,
                    const Http::IClient::Transaction& transaction
                ){
                    if (transaction.response.statusCode == 200) {
                        const auto users = Json::Value::FromEncoding(transaction.response.body)["users"];
                        for (size_t i = 0; i < users.GetSize(); ++i) {
                            const auto& userEncoded = users[i];
                            intmax_t userid;
                            if (
                                sscanf(
                                    ((std::string)userEncoded["_id"]).c_str(), "%" SCNdMAX,
                                    &userid
                                ) != 1
                            ) {
                                impl.diagnosticsSender.SendDiagnosticInformationFormatted(
                                    SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                                    "Twitch API call %d returned user %zu with invalid ID",
                                    id,
                                    i
                                );
                                continue;
                            }
                            const auto login = (std::string)userEncoded["name"];
                            if (login.empty()) {
                                impl.diagnosticsSender.SendDiagnosticInformationFormatted(
                                    SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                                    "Twitch API call %d returned user %zu with missing login",
                                    id,
                                    i
                                );
                                continue;
                            }
                            auto userLookupsByLoginEntry = impl.userJoinsByLogin.find(login);
                            if (userLookupsByLoginEntry == impl.userJoinsByLogin.end()) {
                                impl.diagnosticsSender.SendDiagnosticInformationFormatted(
                                    SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                                    "Twitch API call %d returned user %zu with unexpected login (%s)",
                                    id,
                                    i,
                                    login.c_str()
                                );
                                continue;
                            }
                            const auto joinTime = userLookupsByLoginEntry->second;
                            (void)impl.userJoinsByLogin.erase(userLookupsByLoginEntry);
                            impl.userIdsByLogin[login] = userid;
                            auto& user = impl.usersById[userid];
                            if (user.login != login) {
                                if (!user.login.empty()) {
                                    impl.diagnosticsSender.SendDiagnosticInformationFormatted(
                                        3,
                                        "Twitch user %" PRIdMAX " login changed from %s to %s",
                                        userid,
                                        user.login.c_str(),
                                        login.c_str()
                                    );
                                    (void)impl.userIdsByLogin.erase(user.login);
                                }
                                user.login = login;
                            }
                            const auto name = (std::string)userEncoded["display_name"];
                            if (user.name != name) {
                                if (!user.name.empty()) {
                                    impl.diagnosticsSender.SendDiagnosticInformationFormatted(
                                        3,
                                        "Twitch user %" PRIdMAX " display name changed from %s to %s",
                                        userid,
                                        user.name.c_str(),
                                        name.c_str()
                                    );
                                }
                                user.name = name;
                            }
                            user.createdAt = ParseTimestamp((std::string)userEncoded["created_at"]);
                            impl.UserJoined(userid, joinTime);
                        }
                    } else {
                        impl.diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                            "Twitch API call %d returned code %d",
                            id,
                            transaction.response.statusCode
                        );
                    }
                }
            );
        }

        void Worker() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            WorkerBody(lock);
            if (
                (state == State::InsideRoom)
                || (state == State::OutsideRoom)
            ) {
                tmi.LogOut("kthxbye");
                WithoutLock(lock, [&]{ AwaitLogOut(); });
            }
            PostStatus("Stopped");
        }

        void WorkerBody(std::unique_lock< decltype(mutex) >& lock) {
            LoadConfiguration();
            std::string caCerts;
            const auto caCertsPath = SystemAbstractions::File::GetExeParentDirectory() + "/cert.pem";
            if (!LoadFile(caCertsPath, "CA certificates", diagnosticsSender, caCerts)) {
                return;
            }
            const auto diagnosticsPublisher = diagnosticsSender.Chain();
            (void)httpClient->SubscribeToDiagnostics(diagnosticsPublisher);
            Http::Client::MobilizationDependencies httpClientDeps;
            httpClientDeps.timeKeeper = timeKeeper;
            const auto transport = std::make_shared< HttpNetworkTransport::HttpClientNetworkTransport >();
            transport->SubscribeToDiagnostics(diagnosticsPublisher);
            transport->SetConnectionFactory(
                [
                    diagnosticsPublisher,
                    caCerts
                ](
                    const std::string& scheme,
                    const std::string& serverName
                ) -> std::shared_ptr< SystemAbstractions::INetworkConnection > {
                    const auto decorator = std::make_shared< TlsDecorator::TlsDecorator >();
                    const auto connection = std::make_shared< SystemAbstractions::NetworkConnection >();
                    decorator->ConfigureAsClient(connection, caCerts, serverName);
                    return decorator;
                }
            );
            httpClientDeps.transport = transport;
            httpClient->Mobilize(httpClientDeps);
            tmi.SetConnectionFactory(
                [
                    caCerts,
                    diagnosticsPublisher
                ]() -> std::shared_ptr< Twitch::Connection > {
                    auto connection = std::make_shared< TwitchNetworkTransport::Connection >();
                    connection->SubscribeToDiagnostics(diagnosticsPublisher, 0);
                    connection->SetCaCerts(caCerts);
                    return connection;
                }
            );
            (void)tmi.SubscribeToDiagnostics(diagnosticsPublisher);
            tmi.SetTimeKeeper(timeKeeper);
            tmi.SetUser(twitchDelegate);
            PostStatus("Started");
            while (!stopWorker) {
                if (configurationChanged) {
                    configurationChanged = false;
                    HandleConfigurationChanged();
                }
                auto now = timeKeeper->GetCurrentTime();
                if (
                    !apiCallInProgress
                    && (now >= nextApiCallTime)
                ) {
                    NextApiCall();
                }
                if (now >= nextConfigurationAutoSaveTime) {
                    SaveConfiguration();
                }
                auto nextTimeout = nextConfigurationAutoSaveTime;
                if (
                    !apiCallInProgress
                    && (nextApiCallTime != 0.0)
                ) {
                    nextTimeout = std::min(nextTimeout, nextApiCallTime);
                }
                const auto nowClock = std::chrono::system_clock::now();
                now = timeKeeper->GetCurrentTime();
                if (nextTimeout > now) {
                    const auto timeoutMilliseconds = (int)ceil(
                        (nextTimeout - now)
                        * 1000.0
                    );
                    wakeWorker.wait_until(
                        lock,
                        nowClock + std::chrono::milliseconds(timeoutMilliseconds)
                    );
                }
            }
            SaveConfiguration();
        }
    };

    Main::~Main() noexcept {
        impl_->PostStatus("Stopping");
        impl_->StopWorker();
        impl_->StopDiagnosticsWorker();
    }
    Main::Main(Main&&) noexcept = default;
    Main& Main::operator=(Main&&) noexcept = default;

    Main::Main()
        : impl_(new Impl)
    {
        impl_->selfWeak = impl_;
        impl_->twitchDelegate->implWeak = impl_;
    }

    void Main::Start(std::shared_ptr< Host > host) {
        impl_->host = host;
        impl_->PostStatus("Starting");
        impl_->StartDiagnosticsWorker();
        impl_->StartWorker();
    }

    Configuration Main::GetConfiguration() {
        std::unique_lock< decltype(impl_->mutex) > lock(impl_->mutex);
        return impl_->configuration;
    }

    void Main::SetConfiguration(const Configuration& configuration) {
        std::unique_lock< decltype(impl_->mutex) > lock(impl_->mutex);
        impl_->configuration = configuration;
        impl_->configurationChanged = true;
        impl_->SaveConfiguration();
        impl_->wakeWorker.notify_one();
    }

}
