# Copyright 2022-present Contributors to the flipbook project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/flipbook

cmake_minimum_required (VERSION 3.27)
set (project_name "flipbook")

project (${project_name})

# packages
set (qt6_modules Core PrintSupport Gui Widgets)
find_package (Qt6 COMPONENTS ${qt6_modules} CONFIG REQUIRED)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)
set (CMAKE_AUTOUIC ON)
set (CMAKE_POSITION_INDEPENDENT_CODE ON)

# app
set (app_name "flipbook")

# sources
set (app_sources
    main.cpp
    flipbook.h
    flipbook.cpp
    qt_reader.h
    qt_reader.mm
    qt_sidecar.h
    qt_sidecar.cpp
    rhi_widget.h
    rhi_widget.cpp
)

# resources
file (GLOB app_resources
    "resources/*.frag" 
    "resources/*.vert"
    "resources/*.qsb"
)

# bundle
set (bundle_sources
    "${CMAKE_SOURCE_DIR}/MacOSXBundle.plist.in"
)

# source groups
source_group("Resouce Files" FILES ${app_resources})

if (APPLE)
    set (MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.mikaelsundell.flipbook")
    set (MACOSX_BUNDLE_EXECUTABLE_NAME ${app_name})
    set (MACOSX_BUNDLE_INFO_STRING ${app_name})
    set (MACOSX_BUNDLE_BUNDLE_NAME ${app_name})
    set (MACOSX_BUNDLE_ICON_FILE AppIcon.icns)
    set (MACOSX_BUNDLE_LONG_VERSION_STRING "0.0.1")
    set (MACOSX_BUNDLE_SHORT_VERSION_STRING "0.0.1")
    set (MACOSX_BUNDLE_BUNDLE_VERSION ${MACOSX_BUNDLE_LONG_VERSION_STRING})
    set (MACOSX_BUNDLE_COPYRIGHT "Copyright 2024-present Contributors to the ${app_name} project")
    set (MACOSX_DEPLOYMENT_TARGET ${DEPLOYMENT_TARGET})
    set (CMAKE_XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "-o linker-signed")
    set_source_files_properties(${app_resources} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    add_executable (${project_name} MACOSX_BUNDLE ${app_sources} ${app_resources})
    # definitions
    add_definitions(-DMACOSX_BUNDLE_BUNDLE_NAME="${MACOSX_BUNDLE_BUNDLE_NAME}")
    add_definitions(-DMACOSX_BUNDLE_GUI_IDENTIFIER="${MACOSX_BUNDLE_GUI_IDENTIFIER}")
    add_definitions(-DMACOSX_BUNDLE_COPYRIGHT="${MACOSX_BUNDLE_COPYRIGHT}")
    add_definitions(-DMACOSX_BUNDLE_LONG_VERSION_STRING="${MACOSX_BUNDLE_LONG_VERSION_STRING}")
    add_definitions(-DGITHUBURL="https://github.com/mikaelsundell/flipbook")
    set_target_properties(${project_name} PROPERTIES
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${MACOSX_BUNDLE_GUI_IDENTIFIER}"
        MACOSX_BUNDLE_INFO_PLIST ${bundle_sources}
    )
    target_link_libraries (${project_name} 
        Qt6::Core Qt6::PrintSupport Qt6::Gui Qt6::GuiPrivate Qt6::Widgets 
        "-framework CoreFoundation"
        "-framework AVFoundation"
        "-framework CoreMedia"
        "-framework CoreVideo"
        "-framework AppKit")
else ()
    message (WARNING "${project_name} is a Mac program, will not be built.")
endif ()