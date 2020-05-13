# CMakeLists.txt for Bouncer
#
# © 2019 by Richard Walters

cmake_minimum_required(VERSION 3.8)
set(This Bouncer)

set(Headers
    include/Bouncer/Configuration.hpp
    include/Bouncer/Host.hpp
    include/Bouncer/Main.hpp
    include/Bouncer/Stats.hpp
    include/Bouncer/User.hpp
    include/Bouncer/UserStore.hpp
    include/Bouncer/UsersStore.hpp
    include/Bouncer/UserStoreContainer.hpp
)

set(Sources
    src/Main.cpp
    src/TimeKeeper.cpp
    src/TimeKeeper.hpp
    src/UserStore.cpp
    src/UsersStore.cpp
)

add_library(${This} STATIC ${Sources} ${Headers})
set_target_properties(${This} PROPERTIES
    FOLDER Libraries
)

target_include_directories(${This} PUBLIC include)

target_link_libraries(${This} PUBLIC
    AsyncData
    Http
    HttpNetworkTransport
    Json
    StringExtensions
    SQLite
    SystemAbstractions
    TlsDecorator
    Twitch
    TwitchNetworkTransport
)
