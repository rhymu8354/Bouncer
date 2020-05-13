#pragma once

/**
 * @file UserStoreContainer.hpp
 *
 * This module declares the UserStoreContainer interface.
 */

#include "User.hpp"

#include <stdint.h>
#include <string>

namespace Bouncer {

    /**
     * This is the interface required by the UserStore class in order
     * to request updates in the database.
     */
    class UserStoreContainer {
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
        ) = 0;
        virtual void UpdateUserBot(
            intmax_t id,
            User::Bot bot
        ) = 0;
        virtual void UpdateUserCreatedAt(
            intmax_t id,
            double createdAt
        ) = 0;
        virtual void UpdateUserFirstMessageTime(
            intmax_t id,
            double firstMessageTime
        ) = 0;
        virtual void UpdateUserFirstSeenTime(
            intmax_t id,
            double firstSeenTime
        ) = 0;
        virtual void UpdateUserIsBanned(
            intmax_t id,
            bool isBanned
        ) = 0;
        virtual void UpdateUserIsWhitelisted(
            intmax_t id,
            bool isWhitelisted
        ) = 0;
        virtual void UpdateUserLastMessageTime(
            intmax_t id,
            double lastMessageTime
        ) = 0;
        virtual void UpdateUserLogin(
            intmax_t id,
            const std::string& login
        ) = 0;
        virtual void UpdateUserName(
            intmax_t id,
            const std::string& name
        ) = 0;
        virtual void UpdateUserNote(
            intmax_t id,
            const std::string& note
        ) = 0;
        virtual void UpdateUserNumMessages(
            intmax_t id,
            intmax_t numMessages
        ) = 0;
        virtual void UpdateUserRole(
            intmax_t id,
            User::Role role
        ) = 0;
        virtual void UpdateUserTimeout(
            intmax_t id,
            double timeout
        ) = 0;
        virtual void UpdateUserTotalViewTime(
            intmax_t id,
            double totalViewTime
        ) = 0;
        virtual void UpdateUserWatching(
            intmax_t id,
            bool watching
        ) = 0;
        virtual void AddChat(
            intmax_t userId,
            const std::string& message,
            size_t maxUserChatLines
        ) = 0;
    };

}
