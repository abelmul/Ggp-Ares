include_directories(include)

add_executable(answerListTest answerListTest.cpp)
target_link_libraries(answerListTest PRIVATE ${BOOST_LIBS} cpprest tbb  reasoner gdl memory hash)

add_executable(cacheTest cacheTest.cpp)
target_link_libraries(cacheTest PRIVATE ${BOOST_LIBS} cpprest tbb  reasoner gdl memory hash)

add_executable(monteTest monteTest.cpp mock_reasoner.cpp)
target_link_libraries(monteTest PRIVATE ${BOOST_LIBS} cpprest tbb httphandler reasoner strategy gameAnalyzer game gdl thread memory iterator hash)

add_executable(parserTest parserTest.cpp)
target_link_libraries(parserTest PRIVATE ${BOOST_LIBS} cpprest tbb httphandler reasoner strategy gameAnalyzer game gdl thread memory iterator hash)

add_executable(verifier verifier.cpp)
target_link_libraries(verifier PRIVATE ${BOOST_LIBS} cpprest tbb httphandler reasoner strategy gameAnalyzer game gdl thread memory iterator hash)

add_test(NAME answerListTest COMMAND answerListTest WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_test(NAME cacheTest      COMMAND cacheTest WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_test(NAME monteTest      COMMAND monteTest WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_test(NAME parserTest     COMMAND parserTest WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_test(NAME verifier       COMMAND verifier WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
