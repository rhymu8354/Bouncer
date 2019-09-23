# CMakeLists.txt for BouncerWpf
#
# © 2019 by Richard Walters

if(MSVC)

    cmake_minimum_required(VERSION 3.8)
    set(This BouncerWpf)
    project(${This} CSharp)

    string(REGEX REPLACE "/langversion:[^ ]*"
        /langversion:default CMAKE_CSharp_FLAGS
            ${CMAKE_CSharp_FLAGS}
    )

    set(Sources
        ../Bouncer.ico
        App.config
        App.xaml
        App.xaml.cs
        EventArgs.cs
        ObservableStdVector.cs
        Model/Configuration.cs
        Model/Main.cs
        Model/Message.cs
        Properties/AssemblyInfo.cs
        Properties/Resources.resx
        Properties/Resources.Designer.cs
        View/Commands.cs
        View/ConfigurationWindow.xaml
        View/ConfigurationWindow.xaml.cs
        View/FindDescendant.cs
        View/ListBoxExtensions.cs
        View/MainWindow.xaml
        View/MainWindow.xaml.cs
    )
    set_source_files_properties(App.xaml PROPERTIES
        VS_XAML_TYPE ApplicationDefinition
    )
    set_source_files_properties(
        Bouncer.ico

        PROPERTIES
            VS_TOOL_OVERRIDE Resource
    )

    add_executable(${This} WIN32
        ${Sources}
    )
    set_target_properties(${This} PROPERTIES
        FOLDER Applications
    )
    set(DotNetReferences
        Microsoft.CSharp
        PresentationCore
        PresentationFramework
        System
        System.Core
        System.Data
        System.Data.DataSetExtensions
        System.Net.Http
        System.Xaml
        System.Xml
        System.Xml.Linq
        WindowsBase
    )
    set_target_properties(${This} PROPERTIES
        VS_GLOBAL_ROOTNAMESPACE Bouncer.Wpf
        VS_GLOBAL_AssemblyName Bouncer
        VS_GLOBAL_ApplicationIcon "${CMAKE_CURRENT_SOURCE_DIR}/../Bouncer.ico"
        VS_GLOBAL_PROJECT_TYPES "{60dc8134-eba5-43b8-bcc9-bb4bc16c2548};{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}"
        VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.5"
        VS_DOTNET_REFERENCES "${DotNetReferences}"
    )
    target_link_libraries(${This}
        BouncerNet
    )
    add_custom_command(TARGET ${This} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:BouncerNative> $<TARGET_FILE_DIR:${This}>
    )
    add_custom_command(TARGET ${This} POST_BUILD
        COMMAND if EXIST $<TARGET_PDB_FILE:BouncerNative> ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:BouncerNative> $<TARGET_FILE_DIR:${This}>
    )
    add_custom_command(TARGET ${This} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PROPERTY:tls,SOURCE_DIR>/../apps/openssl/cert.pem $<TARGET_FILE_DIR:${This}>
    )

endif(MSVC)
