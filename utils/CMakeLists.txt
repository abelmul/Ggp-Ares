add_library(game        STATIC game/game.cpp
                               game/visualizer.cpp)

add_library(gdl         STATIC gdl/gdlParser/transformer.cpp
                               gdl/gdlParser/gdlParser.cpp
                               gdl/structuredTerm.cpp)

add_library(thread      STATIC threading/threading.cpp)

add_library(memory      STATIC memory/memCache.cpp
                               memory/memoryPool.cpp)

add_library(iterator    STATIC utils/iterators.cpp)

add_library(httphandler STATIC httpHandler.cpp)
add_dependencies(httphandler cpprestsdk)

add_library(hash        STATIC hashing.cpp)
