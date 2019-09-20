# CMakeLists.txt for Bouncer
#
# © 2019 by Richard Walters

cmake_minimum_required(VERSION 3.8)
set(This Bouncer)

set(Headers
    include/Bouncer/Main.hpp
)

set(Sources
    src/Main.cpp
    src/TimeKeeper.cpp
    src/TimeKeeper.hpp
)

add_library(${This} STATIC ${Sources} ${Headers})
set_target_properties(${This} PROPERTIES
    FOLDER Libraries
)

target_include_directories(${This} PUBLIC include)

target_link_libraries(${This} PUBLIC
    Http
    HttpNetworkTransport
    Json
    SystemAbstractions
    TlsDecorator
    Twitch
    TwitchNetworkTransport
)
