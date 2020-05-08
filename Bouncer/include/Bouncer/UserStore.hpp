#pragma once

/**
 * @file UserStore.hpp
 *
 * This module declares the Bouncer::UserStore class.
 */

#include <Bouncer/User.hpp>
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace Bouncer {

    class UsersStore;

    /**
     * This represents a single Twitch user known by the Bouncer.  All public
     * properties are ephemeral, and methods are provided to get/set
     * persistent information about the user.
     */
    class UserStore {
        // Lifecycle
    public:
        ~UserStore() noexcept;
        UserStore(const UserStore& other) = delete;
        UserStore(UserStore&&) noexcept;
        UserStore& operator=(const UserStore& other) = delete;
        UserStore& operator=(UserStore&&) noexcept;

        // Methods
    public:
        UserStore();
        void Connect(std::weak_ptr< UsersStore > container);
        void Create(const User& user);

        // Public Properties
    public:
        double joinTime = 0.0;
        double partTime = 0.0;
        double firstSeenTime = 0.0;
        double firstMessageTimeThisInstance = 0.0;
        size_t numMessagesThisInstance = 0;
        bool isJoined = false;
        bool isRecentChatter = false;
        bool isNewAccount = false;
        bool needsGreeting = false;

        // Methods
    public:
        void AddLastChat(std::string&& chat);
        void AddTotalViewTime(double time);
        User::Bot GetBot() const;
        double GetCreatedAt() const;
        double GetFirstMessageTime() const;
        intmax_t GetId() const;
        bool GetIsBanned() const;
        bool GetIsWhitelisted() const;
        const std::vector< std::string >& GetLastChat() const;
        double GetLastMessageTime() const;
        const std::string& GetLogin() const;
        const std::string& GetName() const;
        const std::string& GetNote() const;
        size_t GetNumMessages() const;
        User::Role GetRole() const;
        double GetTimeout() const;
        double GetTotalViewTime() const;
        bool GetWatching() const;
        void IncrementNumMessages();
        void SetBot(User::Bot bot);
        void SetCreatedAt(double createdAt);
        void SetFirstMessageTime(double firstMessageTime);
        void SetIsBanned(bool isBanned);
        void SetIsWhitelisted(bool isWhitelisted);
        void SetLastMessageTime(double lastMessageTime);
        void SetLogin(const std::string& login);
        void SetName(const std::string& name);
        void SetNote(const std::string& note);
        void SetRole(User::Role role);
        void SetTimeout(double timeout);
        void SetWatching(bool watching);

        // Private Properties
    private:
        /**
         * This is the type of structure that contains the private
         * properties of the instance.  It is defined in the implementation
         * and declared here to ensure that it is scoped inside the class.
         */
        struct Impl;

        /**
         * This contains the private properties of the instance.
         */
        std::unique_ptr< Impl > impl_;
    };

}
