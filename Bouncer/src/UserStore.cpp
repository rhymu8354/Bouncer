/**
 * @file UserStore.cpp
 *
 * This module contains the implementation of the Bouncer::UserStore
 * class.
 */

#include <Bouncer/User.hpp>
#include <Bouncer/UserStore.hpp>
#include <Bouncer/UserStoreContainer.hpp>
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace {

    using namespace Bouncer;

    constexpr size_t maxUserChatLines = 10;

}

namespace Bouncer {

    /**
     * This defines the private properties of the Bouncer::UserStore class.
     */
    struct UserStore::Impl {
        User::Bot bot = User::Bot::Unknown;
        std::weak_ptr< UserStoreContainer > containerWeak;
        double createdAt = 0.0;
        double firstMessageTime = 0.0;
        double firstSeenTime = 0.0;
        intmax_t id = 0;
        bool isBanned = false;
        bool isWhitelisted = false;
        std::vector< std::string > lastChat;
        double lastMessageTime = 0.0;
        std::string login;
        std::string name;
        std::string note;
        size_t numMessages = 0;
        User::Role role = User::Role::Unknown;
        double timeout = 0.0;
        double totalViewTime = 0.0;
        bool watching = false;
    };

    UserStore::~UserStore() noexcept = default;
    UserStore::UserStore(UserStore&&) noexcept = default;
    UserStore& UserStore::operator=(UserStore&&) noexcept = default;

    UserStore::UserStore(
        const User& user,
        std::weak_ptr< UserStoreContainer > containerWeak
    )
        : impl_(new Impl())
    {
        // Establish connection to container
        impl_->containerWeak = containerWeak;

        // Copy ephemeral data.
        joinTime = user.joinTime;
        partTime = user.partTime;
        firstMessageTimeThisInstance = user.firstMessageTimeThisInstance;
        numMessagesThisInstance = user.numMessagesThisInstance;
        isJoined = user.isJoined;
        isRecentChatter = user.isRecentChatter;
        isNewAccount = user.isNewAccount;
        needsGreeting = user.needsGreeting;

        // Copy persistent data.
        impl_->bot = user.bot;
        impl_->createdAt = user.createdAt;
        impl_->firstMessageTime = user.firstMessageTime;
        impl_->firstSeenTime = user.firstSeenTime;
        impl_->id = user.id;
        impl_->isBanned = user.isBanned;
        impl_->isWhitelisted = user.isWhitelisted;
        impl_->lastChat = user.lastChat;
        impl_->lastMessageTime = user.lastMessageTime;
        impl_->login = user.login;
        impl_->name = user.name;
        impl_->note = user.note;
        impl_->numMessages = user.numMessages;
        impl_->role = user.role;
        impl_->timeout = user.timeout;
        impl_->totalViewTime = user.totalViewTime;
        impl_->watching = user.watching;
    }

    void UserStore::AddLastChat(std::string&& chat) {
        const auto container = impl_->containerWeak.lock();
        if (container) {
            container->AddChat(impl_->id, chat, maxUserChatLines);
        }
        impl_->lastChat.push_back(std::move(chat));
        while (impl_->lastChat.size() > maxUserChatLines) {
            impl_->lastChat.erase(impl_->lastChat.begin());
        }
    }

    void UserStore::AddTotalViewTime(double time) {
        impl_->totalViewTime += time;
        const auto container = impl_->containerWeak.lock();
        if (container) {
            container->UpdateUserTotalViewTime(impl_->id, impl_->totalViewTime);
        }
    }

    void UserStore::Create() {
        const auto container = impl_->containerWeak.lock();
        if (container) {
            container->CreateUser(
                impl_->bot,
                impl_->createdAt,
                impl_->firstMessageTime,
                impl_->firstSeenTime,
                impl_->id,
                impl_->isBanned,
                impl_->isWhitelisted,
                impl_->lastMessageTime,
                impl_->login,
                impl_->name,
                impl_->note,
                impl_->numMessages,
                impl_->role,
                impl_->timeout,
                impl_->totalViewTime,
                impl_->watching
            );
            for (const auto message: impl_->lastChat) {
                container->AddChat(
                    impl_->id,
                    message,
                    maxUserChatLines
                );
            }
        }
    }

    User::Bot UserStore::GetBot() const {
        return impl_->bot;
    }

    double UserStore::GetCreatedAt() const {
        return impl_->createdAt;
    }

    double UserStore::GetFirstMessageTime() const {
        return impl_->firstMessageTime;
    }

    double UserStore::GetFirstSeenTime() const {
        return impl_->firstSeenTime;
    }

    intmax_t UserStore::GetId() const {
        return impl_->id;
    }

    bool UserStore::GetIsBanned() const {
        return impl_->isBanned;
    }

    bool UserStore::GetIsWhitelisted() const {
        return impl_->isWhitelisted;
    }

    const std::vector< std::string >& UserStore::GetLastChat() const {
        return impl_->lastChat;
    }

    double UserStore::GetLastMessageTime() const {
        return impl_->lastMessageTime;
    }

    const std::string& UserStore::GetLogin() const {
        return impl_->login;
    }

    const std::string& UserStore::GetName() const {
        return impl_->name;
    }

    const std::string& UserStore::GetNote() const {
        return impl_->note;
    }

    size_t UserStore::GetNumMessages() const {
        return impl_->numMessages;
    }

    User::Role UserStore::GetRole() const {
        return impl_->role;
    }

    double UserStore::GetTimeout() const {
        return impl_->timeout;
    }

    double UserStore::GetTotalViewTime() const {
        return impl_->totalViewTime;
    }

    bool UserStore::GetWatching() const {
        return impl_->watching;
    }

    void UserStore::IncrementNumMessages() {
        ++impl_->numMessages;
        const auto container = impl_->containerWeak.lock();
        if (container) {
            container->UpdateUserNumMessages(impl_->id, impl_->numMessages);
        }
    }

    User UserStore::MakeSnapshot() const {
        // Copy ephemeral data.
        User user;
        user.joinTime = joinTime;
        user.partTime = partTime;
        user.firstMessageTimeThisInstance = firstMessageTimeThisInstance;
        user.numMessagesThisInstance = numMessagesThisInstance;
        user.isJoined = isJoined;
        user.isRecentChatter = isRecentChatter;
        user.isNewAccount = isNewAccount;
        user.needsGreeting = needsGreeting;

        // Copy persistent data.
        user.bot = impl_->bot;
        user.createdAt = impl_->createdAt;
        user.firstMessageTime = impl_->firstMessageTime;
        user.firstSeenTime = impl_->firstSeenTime;
        user.id = impl_->id;
        user.isBanned = impl_->isBanned;
        user.isWhitelisted = impl_->isWhitelisted;
        user.lastChat = impl_->lastChat;
        user.lastMessageTime = impl_->lastMessageTime;
        user.login = impl_->login;
        user.name = impl_->name;
        user.note = impl_->note;
        user.numMessages = impl_->numMessages;
        user.role = impl_->role;
        user.timeout = impl_->timeout;
        user.totalViewTime = impl_->totalViewTime;
        user.watching = impl_->watching;
        return user;
    }

    void UserStore::SetBot(User::Bot bot) {
        impl_->bot = bot;
        const auto container = impl_->containerWeak.lock();
        if (container) {
            container->UpdateUserBot(impl_->id, impl_->bot);
        }
    }

    void UserStore::SetCreatedAt(double createdAt) {
        impl_->createdAt = createdAt;
        const auto container = impl_->containerWeak.lock();
        if (container) {
            container->UpdateUserCreatedAt(impl_->id, impl_->createdAt);
        }
    }

    void UserStore::SetFirstMessageTime(double firstMessageTime) {
        impl_->firstMessageTime = firstMessageTime;
        const auto container = impl_->containerWeak.lock();
        if (container) {
            container->UpdateUserFirstMessageTime(impl_->id, impl_->firstMessageTime);
        }
    }

    void UserStore::SetFirstSeenTime(double firstSeenTime) {
        impl_->firstSeenTime = firstSeenTime;
        const auto container = impl_->containerWeak.lock();
        if (container) {
            container->UpdateUserFirstSeenTime(impl_->id, impl_->firstSeenTime);
        }
    }

    void UserStore::SetIsBanned(bool isBanned) {
        impl_->isBanned = isBanned;
        const auto container = impl_->containerWeak.lock();
        if (container) {
            container->UpdateUserIsBanned(impl_->id, impl_->isBanned);
        }
    }

    void UserStore::SetIsWhitelisted(bool isWhitelisted) {
        impl_->isWhitelisted = isWhitelisted;
        const auto container = impl_->containerWeak.lock();
        if (container) {
            container->UpdateUserIsWhitelisted(impl_->id, impl_->isWhitelisted);
        }
    }

    void UserStore::SetLastMessageTime(double lastMessageTime) {
        impl_->lastMessageTime = lastMessageTime;
        const auto container = impl_->containerWeak.lock();
        if (container) {
            container->UpdateUserLastMessageTime(impl_->id, impl_->lastMessageTime);
        }
    }

    void UserStore::SetLogin(const std::string& login) {
        impl_->login = login;
        const auto container = impl_->containerWeak.lock();
        if (container) {
            container->UpdateUserLogin(impl_->id, impl_->login);
        }
    }

    void UserStore::SetName(const std::string& name) {
        impl_->name = name;
        const auto container = impl_->containerWeak.lock();
        if (container) {
            container->UpdateUserName(impl_->id, impl_->name);
        }
    }

    void UserStore::SetNote(const std::string& note) {
        impl_->note = note;
        const auto container = impl_->containerWeak.lock();
        if (container) {
            container->UpdateUserNote(impl_->id, impl_->note);
        }
    }

    void UserStore::SetRole(User::Role role) {
        impl_->role = role;
        const auto container = impl_->containerWeak.lock();
        if (container) {
            container->UpdateUserRole(impl_->id, impl_->role);
        }
    }

    void UserStore::SetTimeout(double timeout) {
        impl_->timeout = timeout;
        const auto container = impl_->containerWeak.lock();
        if (container) {
            container->UpdateUserTimeout(impl_->id, impl_->timeout);
        }
    }

    void UserStore::SetWatching(bool watching) {
        impl_->watching = watching;
        const auto container = impl_->containerWeak.lock();
        if (container) {
            container->UpdateUserWatching(impl_->id, impl_->watching);
        }
    }

}
