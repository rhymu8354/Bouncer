#pragma once

/**
 * @file Main.hpp
 *
 * This module declares the Bouncer::Main class.
 *
 * © 2019 by Richard Walters
 */

#include <memory>

namespace Bouncer {

    class Main {
        // Lifecycle management
    public:
        ~Main() noexcept;
        Main(const Main& other) = delete;
        Main(Main&&) noexcept;
        Main& operator=(const Main& other) = delete;
        Main& operator=(Main&&) noexcept;

        // Public methods
    public:
        /**
         * This is the default constructor.
         */
        Main();

        // Private properties
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
