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

        void MarkGreeted(intmax_t userid);

        void SetBotStatus(intmax_t userid, User::Bot bot);

        void SetConfiguration(const Configuration& configuration);

        void SetNote(intmax_t userid, const std::string& note);

        /**
         * Begin the background processing of the application.
         *
         * @param[in] host
         *     This is the interface to the framework hosting the application.
         */
        void StartApplication(std::shared_ptr< Host > host);

        void StartViewTimer();

        void StartWatching(intmax_t userid);

        void StopViewTimer();

        void StopWatching(intmax_t userid);

        void TimeOut(intmax_t userid, int seconds);

        void Unban(intmax_t userid);

        void Unwhitelist(intmax_t userid);

        void Whitelist(intmax_t userid);

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
