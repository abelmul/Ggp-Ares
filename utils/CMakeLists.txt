add_library(utils STATIC httpHandler.cpp
                         hashing.cpp
                         gdl/gdlParser/transformer.cpp
                         gdl/gdlParser/gdlParser.cpp
                         gdl/structuredTerm.cpp
                         game/game.cpp
                         game/visualizer.cpp
                         threading/threading.cpp
                         memory/memCache.cpp
                         memory/memoryPool.cpp
                         utils/iterators.cpp)
