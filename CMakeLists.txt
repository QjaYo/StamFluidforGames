cmake_minimum_required(VERSION 3.10)

# Project name and C++ standard.
project(StamFluidforGames VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Generate compile_commands.json for editor and language-server support.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Executable target name.
set(EXECUTABLE_NAME "StamFluidforGames")

# Optional dependency roots. Prefer package targets over hard-coded library files.
set(VCPKG_PACKAGES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg/packages" CACHE PATH "vcpkg packages directory")
set(EXTERNAL_LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../External/Libraries" CACHE PATH "Legacy external libraries directory")

list(APPEND CMAKE_PREFIX_PATH
    "${VCPKG_PACKAGES_DIR}/glad_x64-linux"
)

# Source directory.
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)

# Header directory.
set(HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/header)

# Collect source files.
file(GLOB SOURCE_FILES "${SOURCE_DIR}/*.cpp")

# Dependencies.
find_package(glad CONFIG QUIET)
if(NOT TARGET glad::glad)
    set(GLAD_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/externel/glad.c")
    find_path(GLAD_INCLUDE_DIR
        NAMES glad/glad.h
        HINTS
            "${VCPKG_PACKAGES_DIR}/glad_x64-linux/include"
            "${EXTERNAL_LIBS_DIR}/include"
    )

    if(NOT EXISTS "${GLAD_SOURCE}" OR NOT GLAD_INCLUDE_DIR)
        message(FATAL_ERROR
            "Could not find GLAD. Install glad with vcpkg or provide glad.c/glad.h. "
            "Checked source: ${GLAD_SOURCE}"
        )
    endif()

    add_library(glad_local STATIC "${GLAD_SOURCE}")
    target_include_directories(glad_local PUBLIC "${GLAD_INCLUDE_DIR}")
    add_library(glad::glad ALIAS glad_local)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW3 REQUIRED IMPORTED_TARGET glfw3)

find_path(STB_INCLUDE_DIR
    NAMES stb_image.h
    HINTS
        "${VCPKG_PACKAGES_DIR}/stb_x64-linux/include"
        "${EXTERNAL_LIBS_DIR}/include"
)
if(NOT STB_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find stb_image.h. Install stb with vcpkg or set STB_INCLUDE_DIR.")
endif()

# Create the executable.
add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})

# Add project header paths.
target_include_directories(${EXECUTABLE_NAME} PRIVATE 
    ${HEADER_DIR}
    ${STB_INCLUDE_DIR}
)

# Expose the project source directory to C++ code.
# This is used to build paths for shaders and other runtime resources.
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
    "PROJECT_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}\""
    "CMAKE_BUILD_TYPE=\"${CMAKE_BUILD_TYPE}\""
)


# Link libraries.
target_link_libraries(${EXECUTABLE_NAME} PRIVATE
    glad::glad
    PkgConfig::GLFW3
)

# Link macOS frameworks required by GLFW.
if (APPLE)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework IOKit"
        "-framework CoreVideo"
    )
endif()

# Add a custom target for building and running the executable with `make run`.
add_custom_target(run
    # Ensure the executable is built before running it.
    DEPENDS ${EXECUTABLE_NAME}
    # Run the executable.
    COMMAND ${EXECUTABLE_NAME}
    # Use the project root as the working directory for relative resource paths.
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
