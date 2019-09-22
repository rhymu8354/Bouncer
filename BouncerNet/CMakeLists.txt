# CMakeLists.txt for BouncerNet
#
# © 2019 by Richard Walters

if(MSVC)

    cmake_minimum_required(VERSION 3.8)
    set(This BouncerNet)
    project(${This} CSharp)
    find_package(SWIG REQUIRED)
    include(${SWIG_USE_FILE})

    set_source_files_properties(../Bouncer.i PROPERTIES
        CPLUSPLUS ON
    )
    include_directories($<TARGET_PROPERTY:SystemAbstractions,INCLUDE_DIRECTORIES>)
    swig_add_library(BouncerNative
        LANGUAGE csharp
        SOURCES
            ../Bouncer.i
    )
    set_target_properties(BouncerNative PROPERTIES
        FOLDER Libraries
    )
    set_property(TARGET BouncerNative PROPERTY SWIG_COMPILE_OPTIONS -namespace Bouncer)
    swig_link_libraries(BouncerNative
        Bouncer
    )

    set(Source
        ${CMAKE_CURRENT_BINARY_DIR}/Configuration.cs
        ${CMAKE_CURRENT_BINARY_DIR}/Host.cs
        ${CMAKE_CURRENT_BINARY_DIR}/Main.cs
        ${CMAKE_CURRENT_BINARY_DIR}/Module.cs
        ${CMAKE_CURRENT_BINARY_DIR}/ModulePINVOKE.cs
        ${CMAKE_CURRENT_BINARY_DIR}/StdVectorString.cs
    )
    set_source_files_properties(${Source} PROPERTIES
        GENERATED 1
    )

    add_library(BouncerNet SHARED
        ${Source}
        Properties/AssemblyInfo.cs
    )
    set_target_properties(${This} PROPERTIES
        FOLDER Libraries
    )
    set(DotNetReferences
        System
        System.Core
        System.Data
        System.Data.DataSetExtensions
        System.Net.Http
        System.Xml
        System.Xml.Linq
    )
    set_target_properties(BouncerNet PROPERTIES
        VS_GLOBAL_ROOTNAMESPACE Bouncer
        VS_GLOBAL_AssemblyName BouncerNet
        VS_GLOBAL_PROJECT_TYPES "{60dc8134-eba5-43b8-bcc9-bb4bc16c2548};{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}"
        VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.5"
        VS_DOTNET_REFERENCES "${DotNetReferences}"
    )
    add_dependencies(BouncerNet BouncerNative)

endif(MSVC)
