#pragma once

/**
 * @file Configuration.hpp
 *
 * This module declares the Bouncer::Configuration structure.
 *
 * © 2019 by Richard Walters
 */

#include <stdint.h>
#include <string>
#include <vector>

namespace Bouncer {

    /**
     * This holds the settings that can be adjusted for the application
     * at runtime and are persisted on the filesystem.
     */
    struct Configuration {
        std::string account;
        std::string token;
        std::string clientId;
        std::string channel;
        std::string greetingPattern;
        std::string newAccountChatterTimeoutExplanation;
        double newAccountAgeThreshold = 604800.0;
        double recentChatThreshold = 1800.0;
        size_t minDiagnosticsLevel = 0;
        bool autoTimeOutNewAccountChatters = false;
        bool autoBanTitleScammers = false;
        bool autoBanForbiddenWords = false;
        std::vector< std::string > forbiddenWords;
        std::string buddyHost;
        uint16_t buddyPort;
    };

}
