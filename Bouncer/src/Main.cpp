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
#include <Json/Value.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <Twitch/Messaging.hpp>
#include <TwitchNetworkTransport/Connection.hpp>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <SystemAbstractions/File.hpp>
#include <SystemAbstractions/StringExtensions.hpp>

namespace {

    const auto configurationFilePath = SystemAbstractions::File::GetExeParentDirectory() + "/Bouncer.json";

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

        // Properties

        Configuration configuration;
        bool configurationChanged = false;
        SystemAbstractions::DiagnosticsSender diagnosticsSender;
        std::shared_ptr< Host > host;
        std::promise< void > loggedOut;
        std::recursive_mutex mutex;
        State state = State::Unconfigured;
        AsyncData::MultiProducerSingleConsumerQueue< StatusMessage > statusMessages;
        bool stopWorker = false;
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
        std::shared_ptr< TimeKeeper > twitchTimeKeeper = std::make_shared< TimeKeeper >();

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
                    std::lock_guard< decltype(mutex) > lock(mutex);
                    statusMessages.Add(std::move(statusMessage));
                    wakeWorker.notify_one();
                }
            );
        }

        void AwaitLogOut() {
            auto wasLoggedOut = loggedOut.get_future();
            (void)wasLoggedOut.wait_for(std::chrono::seconds(1));
        }

        void LoadConfiguration() {
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
            SaveConfiguration();
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
                tmi.SendMessage(
                    configuration.channel,
                    "Hello, World!"
                );
            }
        }

        void OnLeave(Twitch::Messaging::MembershipInfo&& membershipInfo) {
            std::lock_guard< decltype(mutex) > lock(mutex);
            if (membershipInfo.user == configuration.account) {
                PostStatus("Left room");
                state = State::OutsideRoom;
            }
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
            auto json = Json::Object({
                {"account", configuration.account},
                {"token", configuration.token},
                {"clientId", configuration.clientId},
                {"channel", configuration.channel},
                {"newAccountAgeThreshold", configuration.newAccountAgeThreshold},
                {"whitelist", Json::Array({})},
            });
            auto& whitelist = json["whitelist"];
            for (const auto& whitelistEntry: configuration.whitelist) {
                whitelist.Add(whitelistEntry);
            }
            Json::EncodingOptions jsonEncodingOptions;
            jsonEncodingOptions.pretty = true;
            jsonEncodingOptions.reencode = true;
            SaveFile(
                configurationFilePath,
                "configuration",
                diagnosticsSender,
                json.ToEncoding(jsonEncodingOptions)
            );
        }

        void StartWorker() {
            if (worker.joinable()) {
                return;
            }
            std::lock_guard< decltype(mutex) > lock(mutex);
            stopWorker = false;
            worker = std::thread(&Impl::Worker, this);
        }

        void StopWorker() {
            if (!worker.joinable()) {
                return;
            }
            NotifyStopWorker();
            worker.join();
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
            WithoutLock(lock, [&]{ PublishMessages(); });
        }

        void WorkerBody(std::unique_lock< decltype(mutex) >& lock) {
            LoadConfiguration();
            std::string caCerts;
            const auto caCertsPath = SystemAbstractions::File::GetExeParentDirectory() + "/cert.pem";
            if (!LoadFile(caCertsPath, "CA certificates", diagnosticsSender, caCerts)) {
                return;
            }
            const auto diagnosticsPublisher = diagnosticsSender.Chain();
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
            tmi.SubscribeToDiagnostics(diagnosticsPublisher);
            tmi.SetTimeKeeper(twitchTimeKeeper);
            tmi.SetUser(twitchDelegate);
            PostStatus("Started");
            while (!stopWorker) {
                if (configurationChanged) {
                    configurationChanged = false;
                    HandleConfigurationChanged();
                }
                WithoutLock(lock, [&]{ PublishMessages(); });
                wakeWorker.wait(
                    lock,
                    [this]{
                        return (
                            stopWorker
                            || !statusMessages.IsEmpty()
                            || configurationChanged
                        );
                    }
                );
            }
        }
    };

    Main::~Main() noexcept {
        impl_->PostStatus("Stopping");
        impl_->StopWorker();
        impl_->PublishMessages();
    }
    Main::Main(Main&&) noexcept = default;
    Main& Main::operator=(Main&&) noexcept = default;

    Main::Main()
        : impl_(new Impl)
    {
        impl_->twitchDelegate->implWeak = impl_;
    }

    void Main::Start(std::shared_ptr< Host > host) {
        impl_->host = host;
        impl_->PostStatus("Starting");
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
        impl_->wakeWorker.notify_one();
    }

}
