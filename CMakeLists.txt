cmake_minimum_required(VERSION 3.10)

# Project name and C++ standard
project(dbuspp VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(GNUInstallDirs)

# Define options
option(CLANG_TIDY "Enable clang-tidy checks" OFF)
option(BUILD_EXAMPLES "Build examples" ON)

# Enable clang-tidy
if(CLANG_TIDY)
    find_program(CLANG_TIDY_BIN "clang-tidy")
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_BIN} --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy)
endif()

# Find DBus package
find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)

# Add the library
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
# Include DBus headers and link libraries
target_include_directories(${PROJECT_NAME} INTERFACE ${DBUS_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} INTERFACE ${DBUS_LIBRARIES})
target_compile_options(${PROJECT_NAME} INTERFACE ${DBUS_CFLAGS_OTHER})

# Include examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Install the header files
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install the header-only library target
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Config
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install the config files for find_package
install(EXPORT ${PROJECT_NAME}Config
        FILE ${PROJECT_NAME}Config.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
