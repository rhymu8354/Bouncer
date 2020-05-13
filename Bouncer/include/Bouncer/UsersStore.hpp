#pragma once

/**
 * @file UsersStore.hpp
 *
 * This module declares the Bouncer::UsersStore class.
 */

#include "UserStore.hpp"
#include "UserStoreContainer.hpp"

#include <functional>
#include <Json/Value.hpp>
#include <memory>
#include <string>
#include <SystemAbstractions/DiagnosticsSender.hpp>

namespace Bouncer {

    /**
     * This represents the collection of persistent information about Twitch
     * users known by the Bouncer.
     */
    class UsersStore
        : public std::enable_shared_from_this< UsersStore >
        , public UserStoreContainer
    {
        // Lifecycle
    public:
        ~UsersStore() noexcept;
        UsersStore(const UsersStore& other) = delete;
        UsersStore(UsersStore&&) noexcept;
        UsersStore& operator=(const UsersStore& other) = delete;
        UsersStore& operator=(UsersStore&&) noexcept;

        // Constructor
    public:
        UsersStore();

        // UserStoreContainer
    public:
        virtual void CreateUser(
            User::Bot bot,
            double createdAt,
            double firstMessageTime,
            double firstSeenTime,
            intmax_t id,
            bool isBanned,
            bool isWhitelisted,
            double lastMessageTime,
            const std::string& login,
            const std::string& name,
            const std::string& note,
            intmax_t numMessages,
            User::Role role,
            double timeout,
            double totalViewTime,
            bool watching
        ) override;
        virtual void UpdateUserBot(
            intmax_t id,
            User::Bot bot
        ) override;
        virtual void UpdateUserCreatedAt(
            intmax_t id,
            double createdAt
        ) override;
        virtual void UpdateUserFirstMessageTime(
            intmax_t id,
            double firstMessageTime
        ) override;
        virtual void UpdateUserFirstSeenTime(
            intmax_t id,
            double firstSeenTime
        ) override;
        virtual void UpdateUserIsBanned(
            intmax_t id,
            bool isBanned
        ) override;
        virtual void UpdateUserIsWhitelisted(
            intmax_t id,
            bool isWhitelisted
        ) override;
        virtual void UpdateUserLastMessageTime(
            intmax_t id,
            double lastMessageTime
        ) override;
        virtual void UpdateUserLogin(
            intmax_t id,
            const std::string& login
        ) override;
        virtual void UpdateUserName(
            intmax_t id,
            const std::string& name
        ) override;
        virtual void UpdateUserNote(
            intmax_t id,
            const std::string& note
        ) override;
        virtual void UpdateUserNumMessages(
            intmax_t id,
            intmax_t numMessages
        ) override;
        virtual void UpdateUserRole(
            intmax_t id,
            User::Role role
        ) override;
        virtual void UpdateUserTimeout(
            intmax_t id,
            double timeout
        ) override;
        virtual void UpdateUserTotalViewTime(
            intmax_t id,
            double totalViewTime
        ) override;
        virtual void UpdateUserWatching(
            intmax_t id,
            bool watching
        ) override;
        virtual void AddChat(
            intmax_t userId,
            const std::string& message,
            size_t maxUserChatLines
        ) override;

        // Methods
    public:
        void Add(const User& user);
        std::shared_ptr< UserStore > FindById(intmax_t id);
        std::shared_ptr< UserStore > FindByLogin(const std::string& login);
        void Migrate(const Json::Value& jsonUsers);
        bool Mobilize(const std::string& dbFilePath);
        void SetUserId(const std::string& login, intmax_t id);
        SystemAbstractions::DiagnosticsSender::UnsubscribeDelegate SubscribeToDiagnostics(
            SystemAbstractions::DiagnosticsSender::DiagnosticMessageDelegate delegate,
            size_t minLevel = 0
        );
        void WithAll(std::function< void(const std::shared_ptr< UserStore >&) > visitor);

        // Properties
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
