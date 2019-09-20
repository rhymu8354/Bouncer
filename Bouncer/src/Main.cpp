/**
 * @file Main.cpp
 *
 * This module contains the implementation of the Bouncer::Main class.
 *
 * © 2019 by Richard Walters
 */

#include <Bouncer/Main.hpp>

namespace Bouncer {

    /**
     * This contains the private properties of a Main instance.
     */
    struct Main::Impl {
    };

    Main::~Main() {
    }
    Main::Main(Main&&) noexcept = default;
    Main& Main::operator=(Main&&) noexcept = default;

    Main::Main()
        : impl_(new Impl)
    {
    }

}
