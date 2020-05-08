#pragma once

/**
 * @file UsersStore.hpp
 *
 * This module declares the Bouncer::UsersStore class.
 */

#include <Bouncer/UserStore.hpp>
#include <Json/Value.hpp>
#include <memory>
#include <string>
#include <SystemAbstractions/DiagnosticsSender.hpp>

namespace Bouncer {

    /**
     * This represents the collection of persistent information about Twitch
     * users known by the Bouncer.
     */
    class UsersStore : public std::enable_shared_from_this< UsersStore > {
        // Lifecycle
    public:
        ~UsersStore() noexcept;
        UsersStore(const UsersStore& other) = delete;
        UsersStore(UsersStore&&) noexcept;
        UsersStore& operator=(const UsersStore& other) = delete;
        UsersStore& operator=(UsersStore&&) noexcept;

        // Methods
    public:
        UsersStore();
        void Add(const User& user);
        std::shared_ptr< UserStore > FindById(intmax_t id);
        std::shared_ptr< UserStore > FindByLogin(const std::string& login);
        void Migrate(const Json::Value& jsonUsers);
        bool Mobilize(const std::string& dbFilePath);
        void SetUserId(const std::string& login, intmax_t id);
        SystemAbstractions::DiagnosticsSender::UnsubscribeDelegate SubscribeToDiagnostics(
            SystemAbstractions::DiagnosticsSender::DiagnosticMessageDelegate delegate,
            size_t minLevel = 0
        );

        // Properties
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
