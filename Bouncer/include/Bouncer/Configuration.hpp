#pragma once

/**
 * @file Configuration.hpp
 *
 * This module declares the Bouncer::Configuration structure.
 *
 * © 2019 by Richard Walters
 */

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
        std::string channel;
        double newAccountAgeThreshold = 0.0;
        std::vector< std::string > whitelist;
    };

}
