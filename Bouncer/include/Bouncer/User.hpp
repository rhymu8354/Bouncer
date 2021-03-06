#pragma once

/**
 * @file User.hpp
 *
 * This module declares the Bouncer::User structure.
 *
 * © 2019 by Richard Walters
 */

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace Bouncer {

    /**
     * This holds information about a single Twitch user in the context
     * of the channel monitored by the Bouncer.
     */
    struct User {
        intmax_t id = 0;
        std::string login;
        std::string name;
        double createdAt = 0.0;
        double totalViewTime = 0.0;
        double joinTime = 0.0;
        double partTime = 0.0;
        double firstSeenTime = 0.0;
        double firstMessageTime = 0.0;
        double firstMessageTimeThisInstance = 0.0;
        double lastMessageTime = 0.0;
        size_t numMessages = 0;
        size_t numMessagesThisInstance = 0;
        double timeout = 0.0;
        bool isBanned = false;
        bool isJoined = false;
        bool isRecentChatter = false;
        bool isNewAccount = false;
        bool isWhitelisted = false;
        bool watching = false;
        bool needsGreeting = false;
        std::string note;
        enum class Bot {
            Unknown,
            Yes,
            No,
        } bot = Bot::Unknown;
        enum class Role {
            Unknown,
            Pleb,
            VIP,
            Moderator,
            Broadcaster,
            Admin,
            Staff,
        } role = Role::Unknown;
        std::vector< std::string > lastChat;
    };

}
