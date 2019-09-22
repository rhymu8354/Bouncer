#pragma once

/**
 * @file Host.hpp
 *
 * This module declares the Bouncer::Host interface.
 *
 * © 2019 by Richard Walters
 */

#include <memory>

namespace Bouncer {

    /**
     * This is the interface required for hosting the application
     * in a framework.
     */
    class Host {
    public:
        virtual int Foo() = 0;
    };

}
