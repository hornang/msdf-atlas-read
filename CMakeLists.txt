cmake_minimum_required(VERSION 3.5)

add_library(msdf-atlas-read INTERFACE
    include/msdf-atlas-read.h
)

target_include_directories(msdf-atlas-read
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(msdf-atlas-read INTERFACE
    nlohmann_json::nlohmann_json
)

