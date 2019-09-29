#pragma once

/**
 * @file Configuration.hpp
 *
 * This module declares the Bouncer::Configuration structure.
 *
 * © 2019 by Richard Walters
 */

#include <string>

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
        double newAccountAgeThreshold = 604800.0;
        double recentChatThreshold = 1800.0;
        size_t minDiagnosticsLevel = 0;
        bool autoTimeOutNewAccountChatters = false;
        bool autoBanTitleScammers = false;
    };

}
