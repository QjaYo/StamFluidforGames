cmake_minimum_required(VERSION 3.10)

# Project name and C++ standard.
project(StamFluidforGames VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json for editor and language-server support.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Executable target name.
set(EXECUTABLE_NAME "StamFluidforGames")

# Root directory for external libraries.
# The project remains portable as long as this directory structure is preserved.
set(EXTERNAL_LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../External/Libraries" CACHE PATH "Root directory for external libraries")

# Source directory.
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)

# Header directory.
set(HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/header)

# Collect source files.
file(GLOB SOURCE_FILES "${SOURCE_DIR}/*.cpp")

# Build GLAD as a static library.
# This assumes glad.c is located under the configured external libraries path.
add_library(glad STATIC ${EXTERNAL_LIBS_DIR}/glad/src/glad.c)
# Add GLAD include paths.
target_include_directories(glad PUBLIC
    ${EXTERNAL_LIBS_DIR}/include
    ${EXTERNAL_LIBS_DIR}/include/glad
)

# Configure GLFW as an imported static library.
add_library(glfw STATIC IMPORTED)

# Set the GLFW library file and include path.
set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION "${EXTERNAL_LIBS_DIR}/lib/libglfw3.a"
    INTERFACE_INCLUDE_DIRECTORIES "${EXTERNAL_LIBS_DIR}/include"
)

# Create the executable.
add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})

# Add project header paths.
target_include_directories(${EXECUTABLE_NAME} PRIVATE 
    ${HEADER_DIR}
)

# Expose the project source directory to C++ code.
# This is used to build paths for shaders and other runtime resources.
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE "PROJECT_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}\"")


# Link libraries.
target_link_libraries(${EXECUTABLE_NAME} PRIVATE
    glad
    glfw
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
