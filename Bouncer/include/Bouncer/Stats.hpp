#pragma once

/**
 * @file Stats.hpp
 *
 * This module declares the Bouncer::Stats structure.
 *
 * © 2019 by Richard Walters
 */

#include <stddef.h>

namespace Bouncer {

    /**
     * This holds statistics gathered by the application.
     */
    struct Stats {
        double totalViewTimeRecorded = 0.0;
        double totalViewTimeRecordedThisInstance = 0.0;
        size_t currentViewerCount = 0;
        size_t maxViewerCount = 0;
        size_t maxViewerCountThisInstance = 0;
        size_t numViewersKnown = 0;
    };

}
