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

# Define the executable target
add_executable(katz ${CORE_SOURCES} ${UTILS_SOURCES} ${ROUTES_SOURCES} src/main.c) # Add main.c here

# Link libraries with the executable
target_link_libraries(katz PRIVATE 
    ${MICROHTTPD_LIBRARIES} 
    ${UUID_LIBRARY}
    pthread 
    ${JANSSON_LIBRARY} 
    ${B64_LIBRARY}
    dl
)

# Set the runtime output directory for the executable
set_target_properties(katz PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Compile route .c files into shared libraries
foreach(route_source ${ROUTES_SOURCES})
    get_filename_component(route_name ${route_source} NAME_WE)

    # Create the shared library for each route
    add_library(${route_name} SHARED
        ${route_source}
    )

    # Include directories and link the required libraries
    target_include_directories(${route_name} PRIVATE 
        ${CMAKE_SOURCE_DIR}/includes 
        ${JANSSON_INCLUDE_DIR} 
        ${B64_INCLUDE_DIRS}
    )
    target_link_libraries(${route_name} PRIVATE 
        pthread 
        dl 
        ${JANSSON_LIBRARY} 
        ${B64_LIBRARY}
    )

    # Set the output directory for shared libraries
    set_target_properties(${route_name} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/routes)
endforeach()