cmake_minimum_required(VERSION 3.1)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project(Ares VERSION 0.0.1)

set(PROJECT_DESCRIPTION "A general game playing AI")

# cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")


# Boost
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREAD ON)
find_package(Boost 1.71.0 REQUIRED COMPONENTS system thread regex chrono)
if(Boost_FOUND)
  include_directories(${BOOST_INCLUDE_DIRS})
  link_directories(${BOOST_LIBRARY_DIRS})
  set(BOOST_LIBS ${BOOST_SYSTEM_LIBRARY} ${BOOST_THREAD_LIBRARY} ${BOOST_REGEX_LIBRARY} ${BOOST_CHRONO_LIBRARY})
else()
    message(FATAL_ERROR "Boost not found")
endif()

# Cpprestsdk
add_subdirectory(lib/cpprestsdk)
include_directories(lib/cpprestsdk/Release/include)

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

add_executable(ares ares.cpp)
target_link_libraries(ares PRIVATE ${BOOST_LIBS} cpprest tbb httphandler reasoner strategy gameAnalyzer game gdl thread memory iterator hash)
