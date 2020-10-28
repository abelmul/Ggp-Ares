cmake_minimum_required(VERSION 3.1)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project(Ares VERSION 0.0.1)

set(PROJECT_DESCRIPTION "A general game playing AI")

# cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Boost
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREAD ON)
find_package(Boost 1.71.0 REQUIRED COMPONENTS system thread regex chrono)
if(Boost_FOUND)
    include_directories(${BOOST_INCLUDE_DIRS})
    link_directories(${BOOST_LIBRARY_DIRS})
else()
    message(FATAL_ERROR "Boost not found")
endif()

#######################
# Cpprestsdk
include(ExternalProject)
ExternalProject_add(cpprestsdk
        DOWNLOAD_COMMAND git clone --recursive --shallow-submodules --depth 1 --branch v2.10.16 https://github.com/microsoft/cpprestsdk

        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/lib/cpprestsdk

        CONFIGURE_COMMAND cmake ../cpprestsdk/Release -G ${CMAKE_GENERATOR} -DCMAKE_BUILD_TYPE=Release -DBUILD_SAMPLES=false -DBUILD_TESTS=false -DWERROR=false
        BUILD_COMMAND ${CMAKE_MAKE_PROGRAM}

        INSTALL_COMMAND ""
)


ExternalProject_Get_Property(cpprestsdk SOURCE_DIR)
ExternalProject_Get_Property(cpprestsdk BINARY_DIR)
set(cpprestsdk_SOURCE_DIR ${SOURCE_DIR})
set(cpprestsdk_BINARY_DIR ${BINARY_DIR}/Binaries)

include_directories(${cpprestsdk_SOURCE_DIR}/Release/include)
link_directories(${cpprestsdk_BINARY_DIR})

#######################

## add compile options for own source
add_compile_options(-Wall -Wextra -Ofast -march=native)
# tests
option(BUILD_ARES_TESTS "Make tests set to off if you don't want to test" ON)

include_directories(include)

add_subdirectory(gameAnalyzer)
add_subdirectory(reasoner)
add_subdirectory(strategy)
add_subdirectory(utils)

if(BUILD_ARES_TESTS)
    enable_testing()
    add_subdirectory(tests)
else()
    message(STATUS "Testing OFF")
endif()

# Libraries we need
set(SYSTEM_LIBS ${Boost_LIBRARIES} cpprest tbb ssl crypto)

add_executable(ares ares.cpp)
add_dependencies(ares cpprestsdk)
target_link_libraries(ares PRIVATE ${SYSTEM_LIBS} httphandler reasoner strategy gameAnalyzer game gdl thread memory iterator hash)

#Create a symlink of ares.cfg.json

#add_custom_target(symlink ALL
    #${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/ares.cfg.json ${CMAKE_BINARY_DIR}/ares.cfg.json)
