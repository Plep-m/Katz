cmake_minimum_required(VERSION 3.10)

project(katz VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/includes)

# Find and include necessary libraries
find_package(PkgConfig REQUIRED)

# libmicrohttpd
pkg_check_modules(MICROHTTPD REQUIRED libmicrohttpd)
include_directories(${MICROHTTPD_INCLUDE_DIRS})
message(STATUS "Using microhttpd: ${MICROHTTPD_LIBRARIES}")

# libuuid
find_library(UUID_LIBRARY uuid REQUIRED)
message(STATUS "Using libuuid: ${UUID_LIBRARY}")

# Jansson
set(JANSSON_INCLUDE_DIR /usr/include)
set(JANSSON_LIBRARY /usr/lib/x86_64-linux-gnu/libjansson.so)
include_directories(${JANSSON_INCLUDE_DIR})

# b64/cdecode
include_directories(/usr/include)
find_library(B64_LIBRARY NAMES b64 PATHS /usr/lib/x86_64-linux-gnu)
message(STATUS "Using libb64: ${B64_LIBRARY}")

# Collect all source files from the src/ directory
file(GLOB_RECURSE CORE_SOURCES "${CMAKE_SOURCE_DIR}/src/core/*.c")
file(GLOB_RECURSE UTILS_SOURCES "${CMAKE_SOURCE_DIR}/src/utils/*.c")
file(GLOB_RECURSE ROUTES_SOURCES "${CMAKE_SOURCE_DIR}/src/routes/*.c")

# Define the library target
add_library(katz SHARED ${CORE_SOURCES} ${UTILS_SOURCES} ${ROUTES_SOURCES} src/katz.c)

# Link libraries with the library
target_link_libraries(katz PRIVATE 
    ${MICROHTTPD_LIBRARIES} 
    ${UUID_LIBRARY}
    pthread 
    ${JANSSON_LIBRARY} 
    ${B64_LIBRARY}
    dl
)

# Install the library to /usr/local/lib
install(TARGETS katz
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install only the public headers to /usr/local/include/katz
install(DIRECTORY ${CMAKE_SOURCE_DIR}/includes/public/
    DESTINATION include/katz
    FILES_MATCHING PATTERN "*.h"
)