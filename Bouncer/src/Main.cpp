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
        std::shared_ptr< Host > host;
        Configuration configuration;
    };

    Main::~Main() noexcept = default;
    Main::Main(Main&&) noexcept = default;
    Main& Main::operator=(Main&&) noexcept = default;

    Main::Main()
        : impl_(new Impl)
    {
    }

    void Main::SetHost(std::shared_ptr< Host > host) {
        impl_->host = host;
    }

    Configuration Main::GetConfiguration() {
        return impl_->configuration;
    }

    void Main::SetConfiguration(const Configuration& configuration) {
        impl_->configuration = configuration;
    }

}
