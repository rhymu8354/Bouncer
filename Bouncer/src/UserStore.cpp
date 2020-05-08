/**
 * @file UserStore.cpp
 *
 * This module contains the implementation of the Bouncer::UserStore
 * class.
 */

#include <Bouncer/User.hpp>
#include <Bouncer/UserStore.hpp>
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
        std::weak_ptr< UsersStore > container;
        double createdAt = 0.0;
        double firstMessageTime = 0.0;
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

    UserStore::UserStore()
        : impl_(new Impl())
    {
    }

    void UserStore::Connect(std::weak_ptr< UsersStore > container) {
        impl_->container = container;
    }

    void UserStore::Create(const User& user) {
        // Copy ephemeral data.
        joinTime = user.joinTime;
        partTime = user.partTime;
        firstSeenTime = user.firstSeenTime;
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

        // Create database entry.
    }

    void UserStore::AddLastChat(std::string&& chat) {
        impl_->lastChat.push_back(std::move(chat));
        while (impl_->lastChat.size() > maxUserChatLines) {
            impl_->lastChat.erase(impl_->lastChat.begin());
        }
        // TODO
    }

    void UserStore::AddTotalViewTime(double time) {
        impl_->totalViewTime += time;
        // TODO
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
        // TODO
    }

    void UserStore::SetBot(User::Bot bot) {
        impl_->bot = bot;
        // TODO
    }

    void UserStore::SetCreatedAt(double createdAt) {
        impl_->createdAt = createdAt;
        // TODO
    }

    void UserStore::SetFirstMessageTime(double firstMessageTime) {
        impl_->firstMessageTime = firstMessageTime;
        // TODO
    }

    void UserStore::SetIsBanned(bool isBanned) {
        impl_->isBanned = isBanned;
        // TODO
    }

    void UserStore::SetIsWhitelisted(bool isWhitelisted) {
        impl_->isWhitelisted = isWhitelisted;
        // TODO
    }

    void UserStore::SetLastMessageTime(double lastMessageTime) {
        impl_->lastMessageTime = lastMessageTime;
        // TODO
    }

    void UserStore::SetLogin(const std::string& login) {
        impl_->login = login;
        // TODO
    }

    void UserStore::SetName(const std::string& name) {
        impl_->name = name;
        // TODO
    }

    void UserStore::SetNote(const std::string& note) {
        impl_->note = note;
        // TODO
    }

    void UserStore::SetRole(User::Role role) {
        impl_->role = role;
        // TODO
    }

    void UserStore::SetTimeout(double timeout) {
        impl_->timeout = timeout;
        // TODO
    }

    void UserStore::SetWatching(bool watching) {
        impl_->watching = watching;
        // TODO
    }

}
