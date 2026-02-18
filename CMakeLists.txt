cmake_minimum_required(VERSION 3.16)
project(AltDenoiser VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# compiler options
if(MSVC)
    add_compile_options(/utf-8)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-fPIC)
endif()

# path settings
set(JUCE_DIR          ${CMAKE_SOURCE_DIR}/modules/JUCE)
set(RESAMPLER_DIR     ${CMAKE_SOURCE_DIR}/modules/Resampler)
set(DFNET_DIR         ${CMAKE_SOURCE_DIR}/modules/DeepFilterNet)
set(VST2_SDK_DIR      ${CMAKE_SOURCE_DIR}/modules/plugin_sdk/vstsdk2.4)
set(LIBDF_CARGO_DIR   ${DFNET_DIR}/libDF)
set(TARGET_RELEASE_DIR ${DFNET_DIR}/target/release)
set(LIBS_DIR          ${CMAKE_SOURCE_DIR}/libs)
set(LIBS_INCLUDE_DIR  ${LIBS_DIR}/Include) # df.h

find_program(CARGO_CMD cargo REQUIRED)
if(WIN32)
    set(RUST_LIB_NAME "df.lib")
else()
    set(RUST_LIB_NAME "libdf.a")
endif()

add_custom_target(build_libdf ALL
    COMMAND ${CARGO_CMD} build --release --manifest-path ${LIBDF_CARGO_DIR}/Cargo.toml --features capi
    WORKING_DIRECTORY ${LIBDF_CARGO_DIR}
    BYPRODUCTS ${TARGET_RELEASE_DIR}/${RUST_LIB_NAME}
    COMMENT "###### Building libDF with Cargo... ######"
    VERBATIM
)

# STATIC
add_library(df STATIC IMPORTED GLOBAL)
add_dependencies(df build_libdf)

if(WIN32)
    # Windows
    set_target_properties(df PROPERTIES IMPORTED_LOCATION "${TARGET_RELEASE_DIR}/df.lib")
    set(RUST_SYSTEM_LIBS
        Bcrypt.lib
        Userenv.lib
        Ntdll.lib
        Advapi32.lib
    )
elseif(APPLE)
    # macOS
    set_target_properties(df PROPERTIES IMPORTED_LOCATION "${TARGET_RELEASE_DIR}/libdf.a")
    find_library(SECURITY_FW Security)
    find_library(COREFOUNDATION_FW CoreFoundation)
    find_library(SYSTEMCONFIGURATION_FW SystemConfiguration)
    
    set(RUST_SYSTEM_LIBS
        ${SECURITY_FW}
        ${COREFOUNDATION_FW}
        ${SYSTEMCONFIGURATION_FW}
        resolv
    )
else()
    # Linux
    set_target_properties(df PROPERTIES IMPORTED_LOCATION "${TARGET_RELEASE_DIR}/libdf.a")
    set(RUST_SYSTEM_LIBS
        pthread
        dl
        m
    )
endif()

# check if Juce submodule exists
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE submodule not found! Please run: git submodule update --init --recursive")
endif()

add_subdirectory(${JUCE_DIR})

# binary data
juce_add_binary_data(AltDenoiserAssets
    NAMESPACE AltDenoiserBinaryData
    SOURCES
        ${DFNET_DIR}/models/DeepFilterNet3_onnx.tar.gz
)

# format
juce_set_vst2_sdk_path(${VST2_SDK_DIR})
set(PLUGIN_FORMATS VST3 Standalone VST)
if(APPLE)
    list(APPEND PLUGIN_FORMATS AU)
elseif(UNIX AND NOT APPLE)
    list(APPEND PLUGIN_FORMATS LV2)
endif()

juce_add_plugin(AltDenoiserPlugin
    PRODUCT_NAME        "Alt Denoiser"
    COMPANY_NAME        "Altinus"
    VERSION             1.0.0
    PLUGIN_MANUFACTURER_CODE "AltA" 
    PLUGIN_CODE              "AltD"
    FORMATS             ${PLUGIN_FORMATS}
)

target_compile_definitions(AltDenoiserPlugin PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JucePlugin_LV2URI="https://github.com/Altinus/Alt-Denoiser"
)

# /bigogj for MSVC to avoid "fatal error C1128: number of sections exceeded object file format limit: compile with /bigobj"
if(MSVC)
    target_compile_options(AltDenoiserAssets PRIVATE /bigobj)
    target_compile_options(AltDenoiserPlugin PRIVATE /bigobj)
endif()

target_sources(AltDenoiserPlugin PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/DeepFilterNetProcessor.cpp
    Source/DeepFilterNetProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    ${RESAMPLER_DIR}/Resampler.hpp
)

target_include_directories(AltDenoiserPlugin PRIVATE
    ${LIBS_INCLUDE_DIR}
    ${RESAMPLER_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_link_libraries(AltDenoiserPlugin PRIVATE
    df
    AltDenoiserAssets
    ${RUST_SYSTEM_LIBS}
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)
add_dependencies(AltDenoiserPlugin build_libdf)
