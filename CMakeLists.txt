cmake_minimum_required(VERSION 3.10)

# Project name and C++ standard
project(dbuspp VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find DBus package
find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)

# Add the executable
add_executable(${PROJECT_NAME} examples/networkmanager.cpp)

# Include DBus headers and link libraries
target_include_directories(${PROJECT_NAME} PRIVATE ${DBUS_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${DBUS_LIBRARIES})
target_compile_options(${PROJECT_NAME} PRIVATE ${DBUS_CFLAGS_OTHER})

# Enable clang-tidy
find_program(CLANG_TIDY "clang-tidy")
if(CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY}")
endif()
