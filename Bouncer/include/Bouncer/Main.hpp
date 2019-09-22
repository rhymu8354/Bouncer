#pragma once

/**
 * @file Main.hpp
 *
 * This module declares the Bouncer::Main class.
 *
 * © 2019 by Richard Walters
 */

#include <Bouncer/Configuration.hpp>
#include <Bouncer/Host.hpp>
#include <memory>

namespace Bouncer {

    /**
     * This represents the overall application and its interface
     * to the framework hosting it.
     */
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

        /**
         * Associate the application with the framework hosting it.
         *
         * @param[in] host
         *     This is the interface to the framework hosting the application.
         */
        void SetHost(std::shared_ptr< Host > host);

        Configuration GetConfiguration();

        void SetConfiguration(const Configuration& configuration);

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
