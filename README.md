# Bouncer

This is a stand-alone program which helps moderate a Twitch channel.  It uses
the `Twitch::Messaging` class along with the
`TwitchNetworkTransport::Connection` class in order to connect to the Twitch
chat server and join the channel to moderate.  It uses the `Http::Client` class
along with the `HttpNetworkTransport::HttpClientNetworkTransport` and
`TlsDecorator::TlsDecorator` classes in order to access Twitch APIs to collect
more information about other users in chat.

Most of the time, `Bouncer` merely listens to chat and collects information
such as when accounts are created, what privilege level they have in the
channel, when they first chat in the channel, how often they chat in the
channel, and whether or not the broadcaster has acknowledged them in chat
if they have posted a message since the current stream began.

When `Bouncer` detects certain types of troll and spammer accounts chatting in
the channel, it either times out or bans them according to the configured
rules.  A whitelist can be configured to prevent `Bouncer` from timing out
or banning known trusted accounts.

## Usage

    Usage: BouncerWpf

    Connect to Twitch chat, join a channel, and perform moderation duties.

`Bouncer` itself is a generic C++ program built as a static-link library and
meant to be hosted in a platform-specific application environment.
`BouncerNet` is a Windows project which uses the Simplifed Wrapper and
Interface Generator (`SWIG`) to package `Bouncer` into a pair of .NET
assemblies.  `BouncerWpf` is a Windows application wrapper for `BouncerNet`
which uses the Windows Presentation Foundation `WPF` to implement the user
interface.

`Bouncer` stores configuration and statistics in a JSON file named
`Bouncer.json` which is expected to be in the same directory as the program.
The configuration includes the name of the channel to moderate,
the OAuth token to use to authenticate with Twitch, the Twitch app client ID
to use in API requests, and the rules and whitelist to use for moderation.

## Supported platforms / recommended toolchains

`Bouncer` itself is a portable C++11 application which depends only on the
C++11 compiler, the C and C++ standard libraries, and other C++11 libraries
with similar dependencies, so it should be supported on almost any platform.
The following are recommended toolchains for popular platforms.

* Windows -- [Visual Studio](https://www.visualstudio.com/) (Microsoft Visual
  C++)
* Linux -- clang or gcc
* MacOS -- Xcode (clang)

`BouncerNet` and `BouncerWpf` are Windows .NET projects and so only Windows
with Visual Studio is supported.

## Building

There are two distinct steps in the build process:

1. Generation of the build system, using CMake
2. Compiling, linking, etc., using CMake-compatible toolchain

### Prerequisites

* [CMake](https://cmake.org/) version 3.8 or newer
* C++11 toolchain compatible with CMake for your development platform (e.g.
  [Visual Studio](https://www.visualstudio.com/) on Windows)
* [Http](https://github.com/rhymu8354/Http.git) - a library which implements
  [RFC 7230](https://tools.ietf.org/html/rfc7230), "Hypertext Transfer Protocol
  (HTTP/1.1): Message Syntax and Routing".
* [HttpNetworkTransport](https://github.com/rhymu8354/HttpNetworkTransport.git) -
  a library which implements the transport interfaces needed by the `Http`
  library, in terms of the network endpoint and connection abstractions
  provided by the `SystemAbstractions` library.
* [Json](https://github.com/rhymu8354/Json.git) - a library which implements
  [RFC 7159](https://tools.ietf.org/html/rfc7159), "The JavaScript Object
  Notation (JSON) Data Interchange Format".
* [SWIG](http://swig.org/) - a software development tool that connects programs
  written in C and C++ with a variety of high-level programming languages.
* [SystemAbstractions](https://github.com/rhymu8354/SystemAbstractions.git) - a
  cross-platform adapter library for system services whose APIs vary from one
  operating system to another
* [TlsDecorator](https://github.com/rhymu8354/TlsDecorator.git) - an adapter to
  use `LibreSSL` to encrypt traffic passing through a network connection
  provided by `SystemAbstractions`
* [Twitch](https://github.com/rhymu8354/Twitch.git) - a library for interfacing
  with Twitch chat
* [TwitchNetworkTransport](https://github.com:rhymu8354/TwitchNetworkTransport.git) -
  an adapter to provide the `Twitch` library with the network connection
  facilities from `SystemAbstractions`

### Build system generation

Generate the build system using [CMake](https://cmake.org/) from the solution
root.  For example:

```bash
mkdir build
cd build
cmake -G "Visual Studio 15 2017" -A "x64" ..
```

`SWIG` needs to be installed separately.  When `CMake` is run the first time,
it may not be able to locate `SWIG`.  You can fix this by editing the
`CMakeCache.txt` file in the build directory, locate the `SWIG_EXECUTABLE`
cache entry, and set its value to the absolute path to the `swig.exe`
executable.  Then run `CMake` again to finish generating the build system.

### Compiling, linking, et cetera

Either use [CMake](https://cmake.org/) or your toolchain's IDE to build.
For [CMake](https://cmake.org/):

```bash
cd build
cmake --build . --config Release
```
