/**
 * @file Main.cpp
 *
 * This module contains the implementation of the Bouncer::Main class.
 *
 * © 2019 by Richard Walters
 */

#include "TimeKeeper.hpp"

#include <algorithm>
#include <AsyncData/MultiProducerSingleConsumerQueue.hpp>
#include <Bouncer/Main.hpp>
#include <Bouncer/UsersStore.hpp>
#include <condition_variable>
#include <functional>
#include <future>
#include <Http/Client.hpp>
#include <HttpNetworkTransport/HttpClientNetworkTransport.hpp>
#include <inttypes.h>
#include <Json/Value.hpp>
#include <math.h>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <StringExtensions/StringExtensions.hpp>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <SystemAbstractions/DiagnosticsStreamReporter.hpp>
#include <SystemAbstractions/File.hpp>
#include <SystemAbstractions/NetworkConnection.hpp>
#include <thread>
#include <time.h>
#include <TlsDecorator/TlsDecorator.hpp>
#include <Twitch/Messaging.hpp>
#include <TwitchNetworkTransport/Connection.hpp>
#include <unordered_map>
#include <vector>

namespace {

    constexpr double configurationAutoSaveCooldown = 60.0;
    const auto configurationFilePath = SystemAbstractions::File::GetExeParentDirectory() + "/Bouncer.json";
    const auto usersStoreFilePath = SystemAbstractions::File::GetExeParentDirectory() + "/users.db";
    constexpr int maxTimeoutSeconds = 1209600;
    constexpr size_t maxTwitchUserLookupsByLogin = 100;
    constexpr double reconnectCooldown = 5.0;
    constexpr double streamCheckCooldown = 60.0;
    constexpr double twitchApiLookupCooldown = 1.0;

    std::string InstantiateTemplate(
        const std::string& templateText,
        const std::unordered_map< std::string, std::string >& variables
    ) {
        std::ostringstream builder;
        enum class State {
            Normal,
            Escape,
            TokenStart,
            Token,
        } state = State::Normal;
        std::string token;
        for (auto c: templateText) {
            switch (state) {
                case State::Normal: {
                    if (c == '\\') {
                        state = State::Escape;
                    } else if (c == '$') {
                        state = State::TokenStart;
                    } else {
                        builder << c;
                    }
                } break;

                case State::Escape: {
                    state = State::Normal;
                    builder << c;
                } break;

                case State::TokenStart: {
                    if (c == '{') {
                        state = State::Token;
                        token.clear();
                    } else {
                        state = State::Normal;
                        builder << '$' << c;
                    }
                } break;

                case State::Token: {
                    if (c == '}') {
                        const auto variablesEntry = variables.find(token);
                        if (variablesEntry != variables.end()) {
                            builder << variablesEntry->second;
                        }
                        state = State::Normal;
                    } else {
                        token += c;
                    }
                } break;

                default: break;
            }
        }
        return builder.str();
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
        if (file.OpenReadOnly()) {
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
     *     The equivalent timestamp in seconds since the UNIX epoch
     *     is returned.
     */
    double ParseTimestamp(const std::string& timestamp) {
        int years, months, days, hours, minutes;
        double seconds;
        (void)sscanf(
            timestamp.c_str(),
            "%d-%d-%dT%d:%d:%lfZ",
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
        intmax_t total = 0;
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
        return (
            (double)total
            + seconds
        );
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
        if (file.OpenReadWrite()) {
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

    template< typename T > void WithoutLock(
        T& lock,
        std::function< void() > f
    ) {
        lock.unlock();
        f();
        lock.lock();
    }

}

namespace Bouncer {

    /**
     * This contains the private properties of a Main instance.
     */
    struct Main::Impl {
        // Types

        struct MessageAwaitingProcessing {
            Twitch::Messaging::MessageInfo messageInfo;
            double messageTime = 0.0;
        };

        struct WhisperAwaitingProcessing {
            Twitch::Messaging::WhisperInfo whisperInfo;
            double messageTime = 0.0;
        };

        enum class State {
            Unconfigured,
            Unconnected,
            Connecting,
            OutsideRoom,
            InsideRoom,
        };

        struct StatusMessage {
            size_t level = 0;
            std::string message;
            intmax_t userid = 0;
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

            virtual void Doom() override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
                impl->OnDoom();
            }

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
                impl->OnMessage(std::move(messageInfo));
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
                impl->OnWhisper(std::move(whisperInfo));
            }

            virtual void Notice(Twitch::Messaging::NoticeInfo&& noticeInfo) override {
                const auto impl = implWeak.lock();
                if (impl == nullptr) {
                    return;
                }
                impl->OnNotice(std::move(noticeInfo));
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
                impl->OnClear(std::move(clearInfo));
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

        // Properties

        bool apiCallInProgress = false;
        AsyncData::MultiProducerSingleConsumerQueue< std::function< void() > > apiCalls;
        Configuration configuration;
        bool configurationChanged = false;
        std::mutex diagnosticsMutex;
        SystemAbstractions::DiagnosticsSender diagnosticsSender;
        std::thread diagnosticsWorker;
        FILE* logFileWriter = NULL;
        std::shared_ptr< Host > host;
        std::shared_ptr< Http::Client > httpClient = std::make_shared< Http::Client >();

        /**
         * This holds onto pending HTTP request transactions being made.
         *
         * The keys are unique identifiers.  The values are the transactions.
         */
        std::unordered_map< int, std::shared_ptr< Http::IClient::Transaction > > httpClientTransactions;

        std::promise< void > loggedOut;
        std::unordered_map< intmax_t, std::queue< MessageAwaitingProcessing > > messagesAwaitingProcessing;
        std::unordered_map< intmax_t, std::queue< WhisperAwaitingProcessing > > whispersAwaitingProcessing;
        std::recursive_mutex mutex;
        double nextApiCallTime = 0.0;
        double nextConfigurationAutoSaveTime = 0.0;

        /**
         * This is used to select unique identifiers as keys for the
         * httpClientTransactions collection.  It is incremented each
         * time a key is selected.
         */
        int nextHttpClientTransactionId = 1;

        double nextStreamCheck = 0.0;
        double reconnectTime = 0.0;
        std::weak_ptr< Impl > selfWeak;
        State state = State::Unconfigured;
        Stats stats;
        AsyncData::MultiProducerSingleConsumerQueue< StatusMessage > statusMessages;
        bool stopDiagnosticsWorker = false;
        bool stopWorker = false;
        bool firstStreamTitleCheck = true;
        std::string streamTitle;
        SystemAbstractions::DiagnosticsSender::UnsubscribeDelegate unsubscribeLogFileWriter;
        SystemAbstractions::DiagnosticsSender::UnsubscribeDelegate unsubscribeStatusMessages;
        std::unordered_map< std::string, double > userJoinsByLogin;
        AsyncData::MultiProducerSingleConsumerQueue< std::string > userLookupsByLogin;
        bool userLookupsPending = false;
        bool viewTimerRunning = false;
        double viewTimerStart = 0.0;
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

        /**
         * This is used to manage user data.
         */
        std::shared_ptr< UsersStore > users = std::make_shared< UsersStore >();

        // Methods

        Impl()
            : diagnosticsSender("Bouncer")
        {
            HookDiagnostics();
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

        std::string FormatDateTime(double time) {
            char buffer[20];
            auto timeSeconds = (time_t)time;
            (void)strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", gmtime(&timeSeconds));
            return buffer;
        }

        std::string FormatTime(double time) {
            char buffer[9];
            auto timeSeconds = (time_t)time;
            (void)strftime(buffer, sizeof(buffer), "%H:%M:%S", gmtime(&timeSeconds));
            return buffer;
        }

        void HookDiagnostics() {
            if (unsubscribeStatusMessages != nullptr) {
                unsubscribeStatusMessages();
            }
            unsubscribeStatusMessages = diagnosticsSender.SubscribeToDiagnostics(
                [this](
                    std::string senderName,
                    size_t level,
                    std::string message
                ){
                    QueueStatus(level, message, 0);
                },
                configuration.minDiagnosticsLevel
            );
        }

        void QueueStatus(
            size_t level,
            const std::string& message,
            intmax_t userid
        ) {
            StatusMessage statusMessage;
            statusMessage.level = level;
            statusMessage.message = message;
            statusMessage.userid = userid;
            std::lock_guard< decltype(diagnosticsMutex) > lock(diagnosticsMutex);
            statusMessages.Add(std::move(statusMessage));
            wakeDiagnosticsWorker.notify_one();
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
            configuration.greetingPattern = (std::string)json["greetingPattern"];
            configuration.newAccountChatterTimeoutExplanation = (std::string)json["newAccountChatterTimeoutExplanation"];
            configuration.newAccountAgeThreshold = (double)json["newAccountAgeThreshold"];
            configuration.recentChatThreshold = (double)json["recentChatThreshold"];
            configuration.minDiagnosticsLevel = (size_t)json["minDiagnosticsLevel"];
            configuration.autoTimeOutNewAccountChatters = (bool)json["autoTimeOutNewAccountChatters"];
            configuration.autoBanTitleScammers = (bool)json["autoBanTitleScammers"];
            configuration.autoBanForbiddenWords = (bool)json["autoBanForbiddenWords"];
            stats.maxViewerCount = (size_t)json["maxViewerCount"];
            stats.totalViewTimeRecorded = (double)json["totalViewTimeRecorded"];
            userJoinsByLogin.clear();
            if (json.Has("users")) {
                users->Migrate(json["users"]);
                SaveConfiguration();
            }
            const auto& forbiddenWords = json["forbiddenWords"];
            const auto numForbiddenWords = forbiddenWords.GetSize();
            configuration.forbiddenWords.clear();
            configuration.forbiddenWords.reserve(numForbiddenWords);
            for (size_t i = 0; i < numForbiddenWords; ++i) {
                configuration.forbiddenWords.push_back(forbiddenWords[i]);
            }
            configurationChanged = true;
        }

        void LogIn() {
            state = State::Connecting;
            loggedOut = decltype(loggedOut)();
            tmi.LogIn(
                configuration.account,
                configuration.token
            );
        }

        void HandleClear(
            Twitch::Messaging::ClearInfo&& clearInfo,
            double clearTime
        ) {
            intmax_t targetUserId = 0;
            const auto targetUserIdTag = clearInfo.tags.allTags.find("target-user-id");
            if (
                (targetUserIdTag == clearInfo.tags.allTags.end())
                || (sscanf(targetUserIdTag->second.c_str(), "%" SCNdMAX, &targetUserId) != 1)
            ) {
                return;
            }
            auto user = users->FindById(targetUserId);
            if (user) {
                if (clearInfo.type == Twitch::Messaging::ClearInfo::Type::Ban) {
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        3,
                        "Twitch user %" PRIdMAX " (%s) has been banned",
                        targetUserId,
                        clearInfo.user.c_str()
                    );
                    user->SetIsBanned(true);
                } else {
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        3,
                        "Twitch user %" PRIdMAX " (%s) has been timed out for %zu seconds",
                        targetUserId,
                        clearInfo.user.c_str(),
                        clearInfo.duration
                    );
                    user->SetTimeout(clearTime + clearInfo.duration);
                }
            } else {
                LookupUserById(
                    targetUserId,
                    [
                        clearInfo,
                        clearTime,
                        targetUserId
                    ](Impl& impl){
                        auto clearInfoCopy = clearInfo;
                        impl.HandleClear(std::move(clearInfoCopy), clearTime);
                    }
                );
            }
        }

        void HandleConfigurationChanged() {
            HookDiagnostics();
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

        void HandleMessage(
            const Twitch::Messaging::MessageInfo&& messageInfo,
            double messageTime
        ) {
            const auto userid = messageInfo.tags.userId;
            if (userid != 0) {
                auto user = users->FindById(userid);
                if (user) {
                    UpdateLoginAndName(user, messageInfo.user, messageInfo.tags);
                    user->SetLastMessageTime(messageTime);
                    if (
                        (user->numMessagesThisInstance == 0)
                        && (user->GetBot() != User::Bot::Yes)
                        && (user->GetLogin() != configuration.channel)
                    ) {
                        user->needsGreeting = true;
                    }
                    user->IncrementNumMessages();
                    ++user->numMessagesThisInstance;
                    UserSeen(user, messageTime);
                    if (user->GetFirstMessageTime() == 0.0) {
                        user->SetFirstMessageTime(messageTime);
                    }
                    if (user->firstMessageTimeThisInstance == 0.0) {
                        user->firstMessageTimeThisInstance = messageTime;
                    }
                    UpdateRole(user, messageInfo.tags.badges);
                    user->AddLastChat(
                        StringExtensions::sprintf(
                            "%06zu - %s - %s",
                            user->GetNumMessages(),
                            FormatDateTime(messageTime).c_str(),
                            messageInfo.messageContent.c_str()
                        )
                    );
                    if (configuration.minDiagnosticsLevel <= 3) {
                        if (messageInfo.isAction) {
                            QueueStatus(
                                3,
                                StringExtensions::sprintf(
                                    "[%s] %s %s",
                                    FormatTime(messageTime).c_str(),
                                    user->GetLogin().c_str(),
                                    messageInfo.messageContent.c_str()
                                ),
                                userid
                            );
                        } else {
                            QueueStatus(
                                3,
                                StringExtensions::sprintf(
                                    "[%s] %s: %s",
                                    FormatTime(messageTime).c_str(),
                                    user->GetLogin().c_str(),
                                    messageInfo.messageContent.c_str()
                                ),
                                userid
                            );
                        }
                    }
                    if (
                        (user->GetRole() == User::Role::Broadcaster)
                        && !configuration.greetingPattern.empty()
                    ) {
                        const auto greetingPatternLength = configuration.greetingPattern.length();
                        if (
                            messageInfo.messageContent.substr(0, greetingPatternLength)
                            == configuration.greetingPattern
                        ) {
                            const auto target = StringExtensions::Trim(
                                messageInfo.messageContent.substr(greetingPatternLength)
                            );
                            const auto greetedUser = users->FindByLogin(target);
                            if (greetedUser) {
                                diagnosticsSender.SendDiagnosticInformationFormatted(
                                    2,
                                    "Broadcaster greeted user %" PRIdMAX " (%s)",
                                    userid,
                                    greetedUser->GetLogin().c_str()
                                );
                                greetedUser->needsGreeting = false;
                            }
                        }
                    }
                    if (
                        configuration.autoTimeOutNewAccountChatters
                        && (user->GetRole() == User::Role::Pleb)
                        && !user->GetIsWhitelisted()
                        && (messageTime - user->GetCreatedAt() < configuration.newAccountAgeThreshold)
                    ) {
                        user->needsGreeting = false;
                        const auto seconds = std::min(
                            (int)ceil(
                                configuration.newAccountAgeThreshold
                                - (user->GetCreatedAt() - messageTime)
                            ),
                            maxTimeoutSeconds
                        );
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            3,
                            "New account chatter %" PRIdMAX " (%s) -- timing out user for %d seconds",
                            user->GetId(),
                            user->GetLogin().c_str(),
                            seconds
                        );
                        if (!configuration.newAccountChatterTimeoutExplanation.empty()) {
                            std::unordered_map< std::string, std::string > variables;
                            variables["login"] = user->GetLogin();
                            variables["name"] = user->GetName();
                            const auto explanation = InstantiateTemplate(
                                configuration.newAccountChatterTimeoutExplanation,
                                variables
                            );
                            tmi.SendWhisper(
                                user->GetLogin(),
                                explanation
                            );
                        }
                        tmi.SendMessage(
                            configuration.channel,
                            StringExtensions::sprintf(
                                "/timeout %s %d",
                                user->GetLogin().c_str(),
                                seconds
                            )
                        );
                    }
                    if (
                        configuration.autoBanTitleScammers
                        && !streamTitle.empty()
                        && (user->GetRole() == User::Role::Pleb)
                        && !user->GetIsWhitelisted()
                        && (messageInfo.messageContent.find(streamTitle) != std::string::npos)
                    ) {
                        user->needsGreeting = false;
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            3,
                            "Low-effort spam bot %" PRIdMAX " (%s) detected -- applying ban hammer",
                            user->GetId(),
                            user->GetLogin().c_str()
                        );
                        tmi.SendMessage(
                            configuration.channel,
                            StringExtensions::sprintf(
                                "/ban %s",
                                user->GetLogin().c_str()
                            )
                        );
                    }
                    if (
                        configuration.autoBanForbiddenWords
                        && (user->GetRole() == User::Role::Pleb)
                        && !user->GetIsWhitelisted()
                    ) {
                        std::string forbiddenWordFound;
                        for (const auto& forbiddenWord: configuration.forbiddenWords) {
                            if (messageInfo.messageContent.find(forbiddenWord) != std::string::npos) {
                                forbiddenWordFound = forbiddenWord;
                                break;
                            }
                        }
                        if (!forbiddenWordFound.empty()) {
                            diagnosticsSender.SendDiagnosticInformationFormatted(
                                3,
                                "Forbiden word '%s' spoken by user %" PRIdMAX " (%s) -- applying ban hammer",
                                forbiddenWordFound.c_str(),
                                user->GetId(),
                                user->GetLogin().c_str()
                            );
                            tmi.SendMessage(
                                configuration.channel,
                                StringExtensions::sprintf(
                                    "/ban %s",
                                    user->GetLogin().c_str()
                                )
                            );
                        }
                    }
                } else {
                    auto& messagesAwaitingProcessingForUser = messagesAwaitingProcessing[userid];
                    const auto noMessagesOrWhispersWereAlreadyAwaitingProcessing = (
                        messagesAwaitingProcessingForUser.empty()
                        && (whispersAwaitingProcessing.find(userid) == whispersAwaitingProcessing.end())
                    );
                    MessageAwaitingProcessing messageAwaitingProcessing;
                    messageAwaitingProcessing.messageInfo = std::move(messageInfo);
                    messageAwaitingProcessing.messageTime = messageTime;
                    messagesAwaitingProcessingForUser.push(std::move(messageAwaitingProcessing));
                    if (noMessagesOrWhispersWereAlreadyAwaitingProcessing) {
                        LookupUserById(
                            userid,
                            std::bind(&Impl::ProcessMessagesAndWhispersAwaitingProcessing, this, userid)
                        );
                    }
                }
            }
        }

        void HandleWhisper(
            const Twitch::Messaging::WhisperInfo&& whisperInfo,
            double messageTime
        ) {
            const auto userid = whisperInfo.tags.userId;
            if (userid != 0) {
                auto user = users->FindById(userid);
                if (user) {
                    UpdateLoginAndName(user, whisperInfo.user, whisperInfo.tags);
                    UserSeen(user, messageTime);
                    if (configuration.minDiagnosticsLevel <= 3) {
                        QueueStatus(
                            3,
                            StringExtensions::sprintf(
                                "[%s] %s whispered: %s",
                                FormatTime(messageTime).c_str(),
                                user->GetLogin().c_str(),
                                whisperInfo.message.c_str()
                            ),
                            userid
                        );
                    }
                } else {
                    auto& whispersAwaitingProcessingForUser = whispersAwaitingProcessing[userid];
                    const auto noMessagesOrWhispersWereAlreadyAwaitingProcessing = (
                        whispersAwaitingProcessingForUser.empty()
                        && (messagesAwaitingProcessing.find(userid) == messagesAwaitingProcessing.end())
                    );
                    WhisperAwaitingProcessing whisperAwaitingProcessing;
                    whisperAwaitingProcessing.whisperInfo = std::move(whisperInfo);
                    whisperAwaitingProcessing.messageTime = messageTime;
                    whispersAwaitingProcessingForUser.push(std::move(whisperAwaitingProcessing));
                    if (noMessagesOrWhispersWereAlreadyAwaitingProcessing) {
                        LookupUserById(
                            userid,
                            std::bind(&Impl::ProcessMessagesAndWhispersAwaitingProcessing, this, userid)
                        );
                    }
                }
            }
        }

        void LookupUserById(
            intmax_t userid,
            std::function< void(Impl& impl) > after
        ) {
            const auto targetUriString = StringExtensions::sprintf(
                "https://api.twitch.tv/kraken/users/%" PRIdMAX,
                userid
            );
            PostApiCall(
                targetUriString,
                true,
                [
                    after,
                    userid
                ](
                    Impl& impl,
                    int id,
                    const Http::IClient::Transaction& transaction
                ){
                    if (transaction.response.statusCode == 200) {
                        const auto userEncoded = Json::Value::FromEncoding(transaction.response.body);
                        User user;
                        user.id = userid;
                        user.login = (std::string)userEncoded["name"];
                        user.name = (std::string)userEncoded["display_name"];
                        user.createdAt = ParseTimestamp((std::string)userEncoded["created_at"]);
                        impl.users->Add(user);
                        after(impl);
                    } else {
                        impl.diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                            "Twitch API call %d returned code %u",
                            id,
                            transaction.response.statusCode
                        );
                    }
                }
            );
        }

        void LookupUserByName(
            const std::string& name,
            std::function< void(Impl& impl) > after
        ) {
            const auto targetUriString = StringExtensions::sprintf(
                "https://api.twitch.tv/kraken/users?login=%s",
                name.c_str()
            );
            PostApiCall(
                targetUriString,
                true,
                [
                    after,
                    name
                ](
                    Impl& impl,
                    int id,
                    const Http::IClient::Transaction& transaction
                ){
                    impl.OnLookupUsersByNamesResponse(id, transaction.response, after);
                }
            );
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

        void OnClear(Twitch::Messaging::ClearInfo&& clearInfo) {
            std::lock_guard< decltype(mutex) > lock(mutex);
            HandleClear(std::move(clearInfo), timeKeeper->GetCurrentTime());
        }

        void OnDoom() {
            std::lock_guard< decltype(mutex) > lock(mutex);
            PostStatus("Reconnect requested");
        }

        void OnJoin(Twitch::Messaging::MembershipInfo&& membershipInfo) {
            std::lock_guard< decltype(mutex) > lock(mutex);
            if (membershipInfo.user == configuration.account) {
                PostStatus("Joined room");
                state = State::InsideRoom;
                stats.currentViewerCount = 0;
            } else {
                UsersJoined({std::move(membershipInfo.user)});
            }
        }

        void OnLeave(Twitch::Messaging::MembershipInfo&& membershipInfo) {
            std::lock_guard< decltype(mutex) > lock(mutex);
            if (membershipInfo.user == configuration.account) {
                PostStatus("Left room");
                state = State::OutsideRoom;
                stats.currentViewerCount = 0;
            } else {
                auto user = users->FindByLogin(membershipInfo.user);
                if (
                    user
                    && user->isJoined
                ) {
                    UserParted(
                        user,
                        timeKeeper->GetCurrentTime()
                    );
                }
            }
        }

        void OnLogIn() {
            std::lock_guard< decltype(mutex) > lock(mutex);
            PostStatus("Logged in");
            if (state == State::Connecting) {
                state = State::OutsideRoom;
                tmi.Join(configuration.channel);
            }
        }

        void OnLogOut() {
            std::lock_guard< decltype(mutex) > lock(mutex);
            switch (state) {
                case State::Connecting:
                case State::OutsideRoom:
                case State::InsideRoom: {
                    loggedOut.set_value();
                    PostStatus("Logged out");
                    state = State::Unconnected;
                } break;

                default: break;
            }
            const auto now = timeKeeper->GetCurrentTime();
            users->WithAll(
                [&](const std::shared_ptr< UserStore >& user){
                    if (user->isJoined) {
                        user->isJoined = false;
                        user->AddTotalViewTime(now - user->joinTime);
                    }
                }
            );
            stats.currentViewerCount = 0;
            reconnectTime = now + reconnectCooldown;
            wakeWorker.notify_one();
        }

        void OnLookupUsersByNamesResponse(
            int id,
            const Http::Response& response,
            std::function< void(Impl& impl) > after
        ) {
            if (response.statusCode == 200) {
                const auto usersEncoded = Json::Value::FromEncoding(response.body)["users"];
                for (size_t i = 0; i < usersEncoded.GetSize(); ++i) {
                    const auto& userEncoded = usersEncoded[i];
                    intmax_t userid;
                    if (
                        sscanf(
                            ((std::string)userEncoded["_id"]).c_str(), "%" SCNdMAX,
                            &userid
                        ) != 1
                    ) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                            "Twitch API call %d returned user %zu with invalid ID",
                            id,
                            i
                        );
                        continue;
                    }
                    const auto login = (std::string)userEncoded["name"];
                    if (login.empty()) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                            "Twitch API call %d returned user %zu with missing login",
                            id,
                            i
                        );
                        continue;
                    }
                    double joinTime = 0.0;
                    auto userJoinsByLoginEntry = userJoinsByLogin.find(login);
                    if (userJoinsByLoginEntry != userJoinsByLogin.end()) {
                        joinTime = userJoinsByLoginEntry->second;
                        (void)userJoinsByLogin.erase(userJoinsByLoginEntry);
                    }
                    users->SetUserId(login, userid);
                    auto user = users->FindById(userid);
                    const auto name = (std::string)userEncoded["display_name"];
                    if (user->GetName() != name) {
                        if (!user->GetName().empty()) {
                            diagnosticsSender.SendDiagnosticInformationFormatted(
                                3,
                                "Twitch user %" PRIdMAX " display name changed from %s to %s",
                                userid,
                                user->GetName().c_str(),
                                name.c_str()
                            );
                        }
                        user->SetName(name);
                    }
                    user->SetCreatedAt(ParseTimestamp((std::string)userEncoded["created_at"]));
                    if (joinTime != 0.0) {
                        UserSeen(user, joinTime);
                        UserJoined(user, joinTime);
                    }
                }
                after(*this);
            } else {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                    "Twitch API call %d returned code %u",
                    id,
                    response.statusCode
                );
            }
        }

        void OnMessage(Twitch::Messaging::MessageInfo&& messageInfo) {
            std::lock_guard< decltype(mutex) > lock(mutex);
            HandleMessage(std::move(messageInfo), timeKeeper->GetCurrentTime());
        }

        void OnNameList(Twitch::Messaging::NameListInfo&& nameListInfo) {
            std::lock_guard< decltype(mutex) > lock(mutex);
            UsersJoined(std::move(nameListInfo.names));
        }

        void OnNotice(Twitch::Messaging::NoticeInfo&& noticeInfo) {
            std::lock_guard< decltype(mutex) > lock(mutex);
            diagnosticsSender.SendDiagnosticInformationFormatted(
                3,
                "Received notice (id=\"%s\"): %s",
                noticeInfo.id.c_str(),
                noticeInfo.message.c_str()
            );
        }

        void OnWhisper(Twitch::Messaging::WhisperInfo&& whisperInfo) {
            std::lock_guard< decltype(mutex) > lock(mutex);
            HandleWhisper(std::move(whisperInfo), timeKeeper->GetCurrentTime());
        }

        void PostApiCall(std::function< void() > apiCall) {
            apiCalls.Add(apiCall);
            wakeWorker.notify_one();
        }

        void PostApiCall(
            const std::string& targetUriString,
            bool isKraken,
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
                    isKraken,
                    onCompletion,
                    targetUriString,
                    this
                ]{
                    apiCallInProgress = true;
                    const auto id = nextHttpClientTransactionId++;
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        0,
                        "Twitch API call %d: %s",
                        id,
                        targetUriString.c_str()
                    );
                    Http::Request request;
                    request.method = "GET";
                    request.target.ParseFromString(targetUriString);
                    request.target.SetPort(443);
                    request.headers.SetHeader("Client-ID", configuration.clientId);
                    if (isKraken) {
                        request.headers.SetHeader("Accept", "application/vnd.twitchtv.v5+json");
                        request.headers.SetHeader(
                            "Authorization",
                            std::string("OAuth ") + configuration.token
                        );
                    } else {
                        request.headers.SetHeader(
                            "Authorization",
                            std::string("Bearer ") + configuration.token
                        );
                    }
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
                            if (httpClientTransaction->response.headers.HasHeader("Ratelimit-Remaining")) {
                                size_t apiPointsRemaining;
                                if (
                                    sscanf(
                                        httpClientTransaction->response.headers.GetHeaderValue("Ratelimit-Remaining").c_str(),
                                        "%zu",
                                        &apiPointsRemaining
                                    ) == 1
                                ) {
                                    impl->diagnosticsSender.SendDiagnosticInformationFormatted(
                                        0,
                                        "Twitch API points remaining: %zu",
                                        apiPointsRemaining
                                    );
                                }
                            }
                            if (httpClientTransaction->response.headers.HasHeader("Ratelimit-Reset")) {
                                size_t rateLimitReset = 0;
                                if (
                                    sscanf(
                                        httpClientTransaction->response.headers.GetHeaderValue("Ratelimit-Reset").c_str(),
                                        "%zu",
                                        &rateLimitReset
                                    ) == 1
                                ) {
                                    const auto now = impl->timeKeeper->GetCurrentTime();
                                    impl->diagnosticsSender.SendDiagnosticInformationFormatted(
                                        0,
                                        "Twitch API points will reset in %lg seconds (%zu - %zu)",
                                        ((double)rateLimitReset - now),
                                        rateLimitReset,
                                        (size_t)now
                                    );
                                }
                            }
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
                true,
                [](
                    Impl& impl,
                    int id,
                    const Http::IClient::Transaction& transaction
                ){
                    impl.OnLookupUsersByNamesResponse(id, transaction.response, [](Impl&){});
                }
            );
        }

        void ProcessMessagesAndWhispersAwaitingProcessing(intmax_t userid) {
            auto& messagesAwaitingProcessingForUser = messagesAwaitingProcessing[userid];
            while (!messagesAwaitingProcessingForUser.empty()) {
                auto& messageAwaitingProcessing = messagesAwaitingProcessingForUser.front();
                HandleMessage(
                    std::move(messageAwaitingProcessing.messageInfo),
                    messageAwaitingProcessing.messageTime
                );
                messagesAwaitingProcessingForUser.pop();
            }
            messagesAwaitingProcessing.erase(userid);
            auto& whispersAwaitingProcessingForUser = whispersAwaitingProcessing[userid];
            while (!whispersAwaitingProcessingForUser.empty()) {
                auto& whisperAwaitingProcessing = whispersAwaitingProcessingForUser.front();
                HandleWhisper(
                    std::move(whisperAwaitingProcessing.whisperInfo),
                    whisperAwaitingProcessing.messageTime
                );
                whispersAwaitingProcessingForUser.pop();
            }
            whispersAwaitingProcessing.erase(userid);
        }

        void PublishMessages() {
            while (!statusMessages.IsEmpty()) {
                const auto statusMessage = statusMessages.Remove();
                host->StatusMessage(
                    statusMessage.level,
                    statusMessage.message,
                    statusMessage.userid
                );
            }
        }

        void QueryChannelStats() {
            const auto user = users->FindByLogin(configuration.channel);
            if (!user) {
                LookupUserByName(
                    configuration.channel,
                    [](Impl& impl){ impl.QueryChannelStats(); }
                );
                return;
            }
            const auto channelId = user->GetId();
            PostApiCall(
                StringExtensions::sprintf(
                    "https://api.twitch.tv/kraken/channels/%" PRIdMAX,
                    channelId
                ),
                true,
                [channelId](
                    Impl& impl,
                    int id,
                    const Http::IClient::Transaction& transaction
                ){
                    if (transaction.response.statusCode == 200) {
                        const auto responseDecoded = Json::Value::FromEncoding(transaction.response.body);
                        const intmax_t views = responseDecoded["views"];
                        const intmax_t followers = responseDecoded["followers"];
                        impl.diagnosticsSender.SendDiagnosticInformationFormatted(
                            3,
                            "Twitch channel %" PRIdMAX " (%s) now has %" PRIdMAX " views and %" PRIdMAX " followers",
                            channelId,
                            impl.configuration.channel.c_str(),
                            views,
                            followers
                        );
                    } else {
                        impl.diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                            "Twitch API call %d returned code %u",
                            id,
                            transaction.response.statusCode
                        );
                    }
                }
            );
        }

        void SaveConfiguration() {
            const auto now = timeKeeper->GetCurrentTime();
            nextConfigurationAutoSaveTime = now + configurationAutoSaveCooldown;
            const auto viewTimerTotalTime = (
                viewTimerRunning
                ? now - viewTimerStart
                : 0.0
            );
            const auto totalViewTimeRecorded = stats.totalViewTimeRecorded + viewTimerTotalTime;
            auto json = Json::Object({
                {"account", configuration.account},
                {"token", configuration.token},
                {"clientId", configuration.clientId},
                {"channel", configuration.channel},
                {"greetingPattern", configuration.greetingPattern},
                {"newAccountChatterTimeoutExplanation", configuration.newAccountChatterTimeoutExplanation},
                {"newAccountAgeThreshold", configuration.newAccountAgeThreshold},
                {"recentChatThreshold", configuration.recentChatThreshold},
                {"minDiagnosticsLevel", configuration.minDiagnosticsLevel},
                {"autoTimeOutNewAccountChatters", configuration.autoTimeOutNewAccountChatters},
                {"autoBanTitleScammers", configuration.autoBanTitleScammers},
                {"autoBanForbiddenWords", configuration.autoBanForbiddenWords},
                {"maxViewerCount", stats.maxViewerCount},
                {"totalViewTimeRecorded", totalViewTimeRecorded},
                {"forbiddenWords", Json::Array({})},
            });
            auto& forbiddenWords = json["forbiddenWords"];
            for (const auto& forbiddenWord: configuration.forbiddenWords) {
                forbiddenWords.Add(forbiddenWord);
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

        void StreamCheck() {
            const auto now = timeKeeper->GetCurrentTime();
            nextStreamCheck = now + streamCheckCooldown;
            if (configuration.channel.empty()) {
                return;
            }
            const auto targetUriString = (
                std::string("https://api.twitch.tv/helix/streams?user_login=")
                + configuration.channel
            );
            PostApiCall(
                targetUriString,
                false,
                [](
                    Impl& impl,
                    int id,
                    const Http::IClient::Transaction& transaction
                ){
                    if (transaction.response.statusCode == 200) {
                        const auto data = Json::Value::FromEncoding(transaction.response.body)["data"];
                        if (data.GetSize() == 0) {
                            if (impl.streamTitle.empty()) {
                                if (impl.firstStreamTitleCheck) {
                                    impl.diagnosticsSender.SendDiagnosticInformationString(
                                        3,
                                        "The stream is offline"
                                    );
                                }
                            } else {
                                impl.streamTitle.clear();
                                impl.diagnosticsSender.SendDiagnosticInformationString(
                                    3,
                                    "The stream has ended"
                                );
                            }
                        } else {
                            const auto title = (std::string)data[0]["title"];
                            if (impl.streamTitle.empty()) {
                                impl.diagnosticsSender.SendDiagnosticInformationString(
                                    3,
                                    "The stream has started"
                                );
                            }
                            if (impl.streamTitle != title) {
                                impl.diagnosticsSender.SendDiagnosticInformationFormatted(
                                    3,
                                    "Stream title: %s",
                                    title.c_str()
                                );
                                impl.streamTitle = title;
                            }
                        }
                    } else {
                        impl.diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                            "Twitch API call %d returned code %u",
                            id,
                            transaction.response.statusCode
                        );
                    }
                    impl.firstStreamTitleCheck = false;
                }
            );
        }

        void UpdateRole(
            std::shared_ptr< UserStore >& user,
            const std::set< std::string >& badges
        ) {
            const auto setWhitelistedRole = [&](User::Role role) {
                user->SetRole(role);
                user->SetIsWhitelisted(true);
            };
            for (const auto& badge: badges) {
                const auto badgeParts = StringExtensions::Split(badge, '/');
                if (badgeParts.size() >= 1) {
                    if (badgeParts[0] == "vip") {
                        setWhitelistedRole(User::Role::VIP);
                        return;
                    } else if (badgeParts[0] == "moderator") {
                        setWhitelistedRole(User::Role::Moderator);
                        return;
                    } else if (badgeParts[0] == "broadcaster") {
                        setWhitelistedRole(User::Role::Broadcaster);
                        return;
                    } else if (badgeParts[0] == "admin") {
                        setWhitelistedRole(User::Role::Admin);
                        return;
                    } else if (badgeParts[0] == "staff") {
                        setWhitelistedRole(User::Role::Staff);
                        return;
                    }
                }
            }
            user->SetRole(User::Role::Pleb);
        }

        void UpdateLoginAndName(
            std::shared_ptr< UserStore >& user,
            const std::string& login,
            const Twitch::Messaging::TagsInfo& tags
        ) {
            users->SetUserId(login, user->GetId());
            if (user->GetName() != tags.displayName) {
                if (!user->GetName().empty()) {
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        3,
                        "Twitch user %" PRIdMAX " display name changed from %s to %s",
                        user->GetId(),
                        user->GetName().c_str(),
                        tags.displayName.c_str()
                    );
                }
                user->SetName(tags.displayName);
            }
        }

        void UserParted(
            std::shared_ptr< UserStore >& user,
            double partTime
        ) {
            if (
                user->isJoined
                && viewTimerRunning
            ) {
                user->AddTotalViewTime(partTime - user->joinTime);
            }
            if (
                user->isJoined
                && (user->GetBot() != User::Bot::Yes)
            ) {
                ViewerCountDown();
            }
            user->isJoined = false;
            user->partTime = partTime;
            diagnosticsSender.SendDiagnosticInformationFormatted(
                1,
                "User %" PRIdMAX " (%s) has parted",
                user->GetId(),
                user->GetLogin().c_str()
            );
        }

        void UserJoined(
            std::shared_ptr< UserStore >& user,
            double joinTime
        ) {
            if (
                !user->isJoined
                && (user->GetBot() != User::Bot::Yes)
            ) {
                ViewerCountUp();
            }
            user->isJoined = true;
            user->joinTime = joinTime;
            diagnosticsSender.SendDiagnosticInformationFormatted(
                1,
                "User %" PRIdMAX " (%s) has joined (account age: %lf)",
                user->GetId(),
                user->GetLogin().c_str(),
                timeKeeper->GetCurrentTime() - user->GetCreatedAt()
            );
        }

        void UsersJoined(const std::vector< std::string >& logins) {
            const auto joinTime = timeKeeper->GetCurrentTime();
            for (const auto& login: logins) {
                auto user = users->FindByLogin(login);
                if (user) {
                    UserSeen(user, joinTime);
                    if (!user->isJoined) {
                        UserJoined(user, joinTime);
                    }
                } else {
                    userLookupsByLogin.Add(login);
                    userJoinsByLogin[login] = joinTime;
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

        void UserSeen(
            std::shared_ptr< UserStore >& user,
            double time
        ) {
            if (user->GetFirstSeenTime() == 0.0) {
                user->SetFirstSeenTime(time);
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    2,
                    "User %" PRIdMAX " (%s) seen for the first time (%lf)",
                    user->GetId(),
                    user->GetLogin().c_str(),
                    time
                );
            }
        }

        void ViewerCountDown() {
            --stats.currentViewerCount;
        }

        void ViewerCountUp() {
            ++stats.currentViewerCount;
            stats.maxViewerCount = std::max(
                stats.maxViewerCount,
                stats.currentViewerCount
            );
            stats.maxViewerCountThisInstance = std::max(
                stats.maxViewerCountThisInstance,
                stats.currentViewerCount
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
            const auto diagnosticsPublisher = diagnosticsSender.Chain();
            users->SubscribeToDiagnostics(diagnosticsPublisher);
            if (!users->Mobilize(usersStoreFilePath)) {
                return;
            }
            LoadConfiguration();
            std::string caCerts;
            const auto caCertsPath = SystemAbstractions::File::GetExeParentDirectory() + "/cert.pem";
            if (!LoadFile(caCertsPath, "CA certificates", diagnosticsSender, caCerts)) {
                return;
            }
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
                if (now >= nextConfigurationAutoSaveTime) {
                    SaveConfiguration();
                }
                if (now >= nextStreamCheck) {
                    StreamCheck();
                }
                if (
                    !apiCallInProgress
                    && (now >= nextApiCallTime)
                ) {
                    NextApiCall();
                }
                if (
                    (state == State::Unconnected)
                    && (now >= reconnectTime)
                ) {
                    PostStatus("Reconnecting");
                    LogIn();
                }
                auto nextTimeout = std::min(
                    nextConfigurationAutoSaveTime,
                    nextStreamCheck
                );
                if (
                    !apiCallInProgress
                    && (nextApiCallTime != 0.0)
                ) {
                    nextTimeout = std::min(nextTimeout, nextApiCallTime);
                }
                if (state == State::Unconnected) {
                    nextTimeout = std::min(nextTimeout, reconnectTime);
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
        if (impl_->unsubscribeLogFileWriter != nullptr) {
            impl_->unsubscribeLogFileWriter();
        }
        if (impl_->logFileWriter != NULL) {
            (void)fclose(impl_->logFileWriter);
        }
    }
    Main::Main(Main&&) noexcept = default;
    Main& Main::operator=(Main&&) noexcept = default;

    Main::Main()
        : impl_(new Impl)
    {
        impl_->selfWeak = impl_;
        impl_->twitchDelegate->implWeak = impl_;
    }

    void Main::Ban(intmax_t userid) {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        auto user = impl_->users->FindById(userid);
        if (!user) {
            return;
        }
        if (impl_->state != Impl::State::InsideRoom) {
            impl_->diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                "Unable to ban user %" PRIdMAX " (%s) because we're not in the room",
                user->GetId(),
                user->GetLogin().c_str()
            );
            return;
        }
        impl_->diagnosticsSender.SendDiagnosticInformationFormatted(
            3,
            "Banning user %" PRIdMAX " (%s)",
            user->GetId(),
            user->GetLogin().c_str()
        );
        impl_->tmi.SendMessage(
            impl_->configuration.channel,
            StringExtensions::sprintf(
                "/ban %s",
                user->GetLogin().c_str()
            )
        );
    }

    Configuration Main::GetConfiguration() {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        return impl_->configuration;
    }

    Stats Main::GetStats() {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        const auto now = impl_->timeKeeper->GetCurrentTime();
        auto statsSnapshot = impl_->stats;
        const auto viewTimerTotalTime = (
            impl_->viewTimerRunning
            ? now - impl_->viewTimerStart
            : 0.0
        );
        statsSnapshot.totalViewTimeRecorded += viewTimerTotalTime;
        statsSnapshot.totalViewTimeRecordedThisInstance += viewTimerTotalTime;
        statsSnapshot.numViewersKnown = 0;
        impl_->users->WithAll(
            [&](const std::shared_ptr< UserStore >& user){
                ++statsSnapshot.numViewersKnown;
            }
        );
        return statsSnapshot;
    }

    std::vector< User > Main::GetUsers() {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        const auto now = impl_->timeKeeper->GetCurrentTime();
        std::vector< User > users;
        impl_->users->WithAll(
            [&](const std::shared_ptr< UserStore >& user){
                auto userSnapshot = user->MakeSnapshot();
                if (userSnapshot.isJoined) {
                    const auto viewTimerTotalTime = (
                        impl_->viewTimerRunning
                        ? now - userSnapshot.joinTime
                        : 0.0
                    );
                    userSnapshot.totalViewTime += viewTimerTotalTime;
                }
                if (
                    userSnapshot.isJoined
                    && (now - userSnapshot.lastMessageTime < impl_->configuration.recentChatThreshold)
                ) {
                    userSnapshot.isRecentChatter = true;
                }
                if (now - userSnapshot.createdAt < impl_->configuration.newAccountAgeThreshold) {
                    userSnapshot.isNewAccount = true;
                }
                if (now > userSnapshot.timeout) {
                    userSnapshot.timeout = 0.0;
                } else {
                    userSnapshot.timeout -= now;
                }
                users.push_back(std::move(userSnapshot));
            }
        );
        return users;
    }

    void Main::MarkGreeted(intmax_t userid) {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        auto user = impl_->users->FindById(userid);
        if (!user) {
            return;
        }
        user->needsGreeting = false;
    }

    void Main::QueryChannelStats() {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        impl_->QueryChannelStats();
    }

    void Main::SetBotStatus(intmax_t userid, User::Bot bot) {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        auto user = impl_->users->FindById(userid);
        if (!user) {
            return;
        }
        if (user->isJoined) {
            if (
                (user->GetBot() == User::Bot::Yes)
                && (bot != User::Bot::Yes)
            ) {
                impl_->ViewerCountUp();
            } else if (
                (user->GetBot() != User::Bot::Yes)
                && (bot == User::Bot::Yes)
            ) {
                impl_->ViewerCountDown();
            }
        }
        user->SetBot(bot);
    }

    void Main::SetConfiguration(const Configuration& configuration) {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        impl_->configuration = configuration;
        impl_->configurationChanged = true;
        impl_->SaveConfiguration();
        impl_->wakeWorker.notify_one();
    }

    void Main::SetNote(intmax_t userid, const std::string& note) {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        auto user = impl_->users->FindById(userid);
        if (!user) {
            return;
        }
        user->SetNote(note);
    }

    void Main::StartApplication(std::shared_ptr< Host > host) {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        if (impl_->worker.joinable()) {
            return;
        }
        time_t startTime;
        (void)time(&startTime);
        char buffer[28];
        (void)strftime(buffer, sizeof(buffer), "/Bouncer-%Y%m%d%H%M%S.log", gmtime(&startTime));
        const auto logFilePath = SystemAbstractions::File::GetExeParentDirectory() + buffer;
        impl_->logFileWriter = fopen(logFilePath.c_str(), "wt");
        if (impl_->logFileWriter != NULL) {
            setbuf(impl_->logFileWriter, NULL);
            impl_->unsubscribeLogFileWriter = impl_->diagnosticsSender.SubscribeToDiagnostics(
                SystemAbstractions::DiagnosticsStreamReporter(
                    impl_->logFileWriter,
                    impl_->logFileWriter
                )
            );
        }
        impl_->host = host;
        impl_->PostStatus("Starting");
        impl_->StartDiagnosticsWorker();
        impl_->StartWorker();
    }

    void Main::StartViewTimer() {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        if (impl_->viewTimerRunning) {
            return;
        }
        impl_->viewTimerRunning = true;
        impl_->viewTimerStart = impl_->timeKeeper->GetCurrentTime();
        impl_->PostStatus("View timer has started");
        impl_->users->WithAll(
            [&](const std::shared_ptr< UserStore >& user){
                if (user->isJoined) {
                    user->joinTime = impl_->viewTimerStart;
                }
            }
        );
    }

    void Main::StartWatching(intmax_t userid) {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        auto user = impl_->users->FindById(userid);
        if (!user) {
            return;
        }
        user->SetWatching(true);
    }

    void Main::StopViewTimer() {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        if (!impl_->viewTimerRunning) {
            return;
        }
        impl_->viewTimerRunning = false;
        const auto now = impl_->timeKeeper->GetCurrentTime();
        const auto viewTimerTotalTime = now - impl_->viewTimerStart;
        impl_->stats.totalViewTimeRecordedThisInstance += viewTimerTotalTime;
        impl_->stats.totalViewTimeRecorded += viewTimerTotalTime;
        impl_->PostStatus("View timer has stopped");
        impl_->users->WithAll(
            [&](const std::shared_ptr< UserStore >& user){
                if (user->isJoined) {
                    user->AddTotalViewTime(now - user->joinTime);
                }
            }
        );
    }

    void Main::StopWatching(intmax_t userid) {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        auto user = impl_->users->FindById(userid);
        if (!user) {
            return;
        }
        user->SetWatching(false);
    }

    void Main::TimeOut(intmax_t userid, int seconds) {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        auto user = impl_->users->FindById(userid);
        if (!user) {
            return;
        }
        if (impl_->state != Impl::State::InsideRoom) {
            impl_->diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                "Unable to time out user %" PRIdMAX " (%s) because we're not in the room",
                user->GetId(),
                user->GetLogin().c_str()
            );
            return;
        }
        impl_->diagnosticsSender.SendDiagnosticInformationFormatted(
            3,
            "Timing out user %" PRIdMAX " (%s) for %d seconds",
            user->GetId(),
            user->GetLogin().c_str(),
            seconds
        );
        impl_->tmi.SendMessage(
            impl_->configuration.channel,
            StringExtensions::sprintf(
                "/timeout %s %d",
                user->GetLogin().c_str(),
                seconds
            )
        );
    }

    void Main::Unban(intmax_t userid) {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        auto user = impl_->users->FindById(userid);
        if (!user) {
            return;
        }
        user->SetIsBanned(false);
        user->SetTimeout(0.0);
        if (impl_->state != Impl::State::InsideRoom) {
            impl_->diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                "Unable to unban user %" PRIdMAX " (%s) because we're not in the room",
                user->GetId(),
                user->GetLogin().c_str()
            );
            return;
        }
        impl_->diagnosticsSender.SendDiagnosticInformationFormatted(
            3,
            "Unbanning user %" PRIdMAX " (%s)",
            user->GetId(),
            user->GetLogin().c_str()
        );
        impl_->tmi.SendMessage(
            impl_->configuration.channel,
            StringExtensions::sprintf(
                "/unban %s",
                user->GetLogin().c_str()
            )
        );
    }

    void Main::Unwhitelist(intmax_t userid) {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        auto user = impl_->users->FindById(userid);
        if (!user) {
            return;
        }
        user->SetIsWhitelisted(false);
    }

    void Main::Whitelist(intmax_t userid) {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        auto user = impl_->users->FindById(userid);
        if (!user) {
            return;
        }
        user->SetIsWhitelisted(true);
    }

}
