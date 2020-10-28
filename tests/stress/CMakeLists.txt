add_executable(simulator simulator.cpp)
target_link_libraries(simulator PRIVATE ${Boost_LIBRARIES} tbb reasoner strategy gameAnalyzer game gdl thread memory iterator hash)

add_executable(strategy_test strategy_test.cpp)
target_link_libraries(strategy_test PRIVATE ${Boost_LIBRARIES} tbb reasoner strategy gameAnalyzer game gdl thread memory iterator hash)

add_test(NAME simulator     COMMAND simulator     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_test(NAME strategy_test COMMAND strategy_test WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
