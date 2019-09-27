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
#include <Bouncer/Stats.hpp>
#include <Bouncer/User.hpp>
#include <memory>
#include <string>
#include <vector>

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

        void Ban(intmax_t userid);

        Configuration GetConfiguration();

        Stats GetStats();

        std::vector< User > GetUsers();

        void SetConfiguration(const Configuration& configuration);

        /**
         * Begin the background processing of the application.
         *
         * @param[in] host
         *     This is the interface to the framework hosting the application.
         */
        void StartApplication(std::shared_ptr< Host > host);

        void StartViewTimer();

        void StopViewTimer();

        void Unban(intmax_t userid);

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
        std::shared_ptr< Impl > impl_;
    };

}
