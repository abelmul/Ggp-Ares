RESNR_DIR =reasoner
STRATEGY_DIR = strategy
UTILS_DIR = utils
GDL_DIR =$(UTILS_DIR)/gdl
GDLP_DIR=$(GDL_DIR)/gdlParser
MEMORY_DIR = $(UTILS_DIR)/memory
THREADING = $(UTILS_DIR)/threading
GAME = $(UTILS_DIR)/game
TESTS = tests
TESTS_UNIT = $(TESTS)/unit
TESTS_UNIT_INC = $(TESTS_UNIT)/include
UNIT_BIN = $(TESTS_UNIT)/bin

#create unit tests binary directory
UNIT_BIN_DIR := $(shell mkdir -p $(UNIT_BIN))

CPPREST_INC = ./lib/cpprestsdk/Release/include
IDIR = ./include
UTILS_IDIR =$(IDIR)/$(UTILS_DIR)
UTILS_UTILS_IDIR = $(UTILS_IDIR)/utils
REASONER_IDIR = $(IDIR)/$(RESNR_DIR)
GDL_IDIR = $(IDIR)/$(GDL_DIR)
MEMORY_IDIR = $(IDIR)/$(MEMORY_DIR)
THREADING_IDIR = $(IDIR)/$(THREADING)
GDLP_IDIR = $(IDIR)/$(GDLP_DIR)
STRATEGY_IDIR = $(IDIR)/$(STRATEGY_DIR)
GAME_IDIR = $(IDIR)/$(GAME)

OBJS_DIR = ../objs
#-fno-strict-aliasing -fsanitize=address -fsanitize=undefined -Wextra
CC = g++
FLAGS = -Wall -std=c++17 -I$(IDIR) -I$(CPPREST_INC) -fno-strict-aliasing
LIBS =  -lboost_regex -lboost_thread -lboost_chrono -lpthread -ltbb  -lboost_system -lcrypto -lssl 
ifdef DEBUG_ARES
FLAGS+= -ggdb
else
FLAGS+= -O3
endif

ifdef testing
FLAGS+=  -I$(TESTS_UNIT)/include
endif

#Get the source files with their path
SOURCES := $(shell find . -not -path "*test*" -name '*.cpp' | sed 's/\.\///')
TEST_FILES := $(shell find .  -path "*test*" -name '*.cpp' | sed 's/\.\///')

#Create their respective object files
OBJS := $(patsubst %.cpp, $(OBJS_DIR)/%.o, $(SOURCES))
OBJS_UNIT := $(patsubst %.cpp, $(OBJS_DIR)/%.o, $(TEST_FILES))
#Create their respective include files
INCLS := $(shell find . -name "*.hh")
REASONER_INCS :=$(shell find $(REASONER_IDIR) -name "*.hh")
THREADING_INCS := $(shell find $(THREADING_IDIR) -name "*.hh")
GDL_INCS := $(shell find $(GDL_IDIR) -maxdepth 1  -name "*.hh")
GDLP_INCS := $(shell find $(GDLP_IDIR)  -name "*.hh")
UTILS_UTILS_INCS := $(shell find $(UTILS_UTILS_IDIR)  -name "*.hh")
GAME_INCS := $(shell find $(GAME_IDIR)  -name "*.hh")
MEMORY_INCS := $(shell find $(MEMORY_IDIR)  -name "*.hh")

COMMON_INCS := $(GDL_INCS) $(UTILS_UTILS_INCS) $(MEMORY_INCS)

#create the directories
OBJ_SUBDIRS := $(shell mkdir -p `dirname $(OBJS)`)
OBJ_UNIT_DIRS := $(shell mkdir -p `dirname $(OBJS_UNIT)`)

setup:
	mkdir lib/cpprestsdk/build.debug
	cd lib/cpprestsdk/build.debug && cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Debug && ninja

ares:  $(OBJS) #$(INCLS)
	$(CC) $(FLAGS) $(LIBS) -o $@ -Wl,--start-group $^ -Wl,--end-group 
$(OBJS_DIR)/%.o : %.cpp $(INCLS)
	$(CC) -c $(FLAGS) -o $@ $< 

# ares:  $(OBJS) 
# 	$(CC) $(FLAGS) $(LIBS) -o $@ -Wl,--start-group $^ -Wl,--end-group 

# $(OBJS_DIR)/ares.o: ares.cpp $(INCLS)
# 	$(CC) -c $(FLAGS) -o $@ $< 

# $(OBJS_DIR)/$(THREADING)/threading.o: $(THREADING)/threading.cpp $(THREADING_INCS) $(COMMON_INCS)
# 	$(CC) -c $(FLAGS) -o $@ $< 

# $(OBJS_DIR)/$(UTILS_DIR)/hashing.o : $(UTILS_DIR)/hashing.cpp $(COMMON_INCS)
# 	$(CC) -c $(FLAGS) -o $@ $< 
# $(OBJS_DIR)/$(GAME)/visualizer.o : $(GAME)/visualizer.cpp $(GAME_INCS) $(COMMON_INCS)
# 	$(CC) -c $(FLAGS) -o $@ $<
# $(OBJS_DIR)/$(MEMORY_DIR)/memCache.o : $(MEMORY_DIR)/memCache.cpp  $(COMMON_INCS)
# 	$(CC) -c $(FLAGS) -o $@ $<
# $(OBJS_DIR)/$(MEMORY_DIR)/memoryPool.o : $(MEMORY_DIR)/memoryPool.cpp  $(COMMON_INCS) $(THREADING_IDIR)/locks.hh
# 	$(CC) -c $(FLAGS) -o $@ $<
# $(OBJS_DIR)/$(GDL_DIR)/structuredTerm.o : $(GDL_DIR)/structuredTerm.cpp $(INCLS)
# 	$(CC) -c $(FLAGS) -o $@ $<
# $(OBJS_DIR)/$(GDLP_DIR)/transformer.o : $(GDLP_DIR)/transformer.cpp $(GDLP_INCS)  $(GAME_INCS) $(COMMON_INCS)
# 	$(CC) -c $(FLAGS) -o $@ $<
# $(OBJS_DIR)/$(GDLP_DIR)/gdlParser.o : $(GDLP_DIR)/gdlParser.cpp $(GDLP_INCS)  $(COMMON_INCS)
# 	$(CC) -c $(FLAGS) -o $@ $<
# $(OBJS_DIR)/$(RESNR_DIR)/prover.o :$(RESNR_DIR)/prover.cpp $(INCLS)
# 	$(CC) -c $(FLAGS) -o $@ $<
# $(OBJS_DIR)/$(RESNR_DIR)/reasoner.o: $(RESNR_DIR)/reasoner.cpp $(INCLS)
# 	$(CC) -c $(FLAGS) -o $@ $<
# $(OBJS_DIR)/$(RESNR_DIR)/substitution.o: $(RESNR_DIR)/substitution.cpp  $(REASONER_IDIR)/substitution.hh $(COMMON_INCS)
# 	$(CC) -c $(FLAGS) -o $@ $<
# $(OBJS_DIR)/$(RESNR_DIR)/suffixRenamer.o:$(RESNR_DIR)/suffixRenamer.cpp $(REASONER_IDIR)/substitution.hh $(COMMON_INCS)
# 	$(CC) -c $(FLAGS) -o $@ $<

# $(OBJS_DIR)/$(RESNR_DIR)/unifier.o:$(RESNR_DIR)/unifier.cpp $(REASONER_IDIR)/substitution.hh $(COMMON_INCS)
# 	$(CC) -c $(FLAGS) -o $@ $<
# $(OBJS_DIR)/$(UTILS_DIR)/httpHandler.o:$(UTILS_DIR)/httpHandler.cpp 
# 	$(CC) -c $(FLAGS) -o $@ $<

#Compile any object file like objs/reasoner/reasoner.o from 
#reasoner/reasoner.cpp
# $(OBJS_DIR)/%.o : %.cpp $(INCLS)
# 	$(CC) -c $(FLAGS) -o $@ $< 



#These are to make and run tests
TEST_OBJS = $(OBJS_DIR)/$(GDL_DIR)/structuredTerm.o $(OBJS_DIR)/$(MEMORY_DIR)/memoryPool.o $(OBJS_DIR)/$(UTILS_DIR)/hashing.o $(OBJS_DIR)/$(RESNR_DIR)/substitution.o $(OBJS_DIR)/$(MEMORY_DIR)/memCache.o
$(TESTS_UNIT)/Test_ThreadPool: $(OBJS_DIR)/$(TESTS_UNIT)/Test_ThreadPool.o $(OBJS_DIR)/$(THREADING)/threadPool.o
	$(CC) $(FLAGS) -o $(UNIT_BIN)/Test_ThreadPool -Wl,--start-group $^ -Wl,--end-group 

$(TESTS_UNIT)/Test_MemPool: $(OBJS_DIR)/$(TESTS_UNIT)/Test_MemPool.o $(TEST_OBJS)
	$(CC) $(FLAGS)  -o $(UNIT_BIN)/Test_MemPool -Wl,--start-group $^ -Wl,--end-group 

$(TESTS_UNIT)/AnswerList_Test: $(OBJS_DIR)/$(TESTS_UNIT)/AnswerList_Test.o $(TEST_OBJS)
	$(CC) $(FLAGS) $(LIBS)  -I $(TESTS_UNIT_INC) -o $(UNIT_BIN)/AnswerList_Test -Wl,--start-group $^ -Wl,--end-group 

$(OBJS_DIR)/$(TESTS_UNIT)/AnswerList_Test.o: $(TESTS_UNIT)/AnswerList_Test.cpp
	$(CC) -c $(FLAGS) -I $(TESTS_UNIT_INC) -o $@ $<

$(TESTS_UNIT)/Cache_Test: $(OBJS_DIR)/$(TESTS_UNIT)/Cache_Test.o $(TEST_OBJS)
	$(CC) $(FLAGS) $(LIBS)  -I $(TESTS_UNIT_INC) -o $(UNIT_BIN)/Cache_Test -Wl,--start-group $^ -Wl,--end-group 

$(OBJS_DIR)/$(TESTS_UNIT)/Cache_Test.o: $(TESTS_UNIT)/Cache_Test.cpp
	$(CC) -c $(FLAGS) -I $(TESTS_UNIT_INC) -o $@ $<

$(OBJS_DIR)/$(TESTS)/verifier.o : $(TESTS)/verifier.cpp
	$(CC) -c $(FLAGS) -I $(TESTS_UNIT_INC) -o $@ $<

$(OBJS_DIR)/$(TESTS)/simulator.o : $(TESTS)/simulator.cpp
	$(CC) -c $(FLAGS) -I $(TESTS_UNIT_INC) -o $@ $<

#Tests involving Random simulation and verification of the reasoner
VERIFIER_DEP =  ../objs/utils/game/game.o ../objs/utils/threading/threading.o ../objs/utils/hashing.o ../objs/utils/game/visualizer.o ../objs/utils/memory/memCache.o ../objs/utils/memory/memoryPool.o ../objs/utils/gdl/structuredTerm.o ../objs/utils/gdl/gdlParser/transformer.o ../objs/utils/gdl/gdlParser/gdlParser.o ../objs/utils/httpHandler.o ../objs/reasoner/prover.o ../objs/reasoner/reasoner.o ../objs/reasoner/substitution.o ../objs/reasoner/suffixRenamer.o ../objs/reasoner/unifier.o
verifier: $(OBJS_DIR)/$(TESTS)/verifier.o $(VERIFIER_DEP)
	$(CC) $(FLAGS) $(LIBS) -o $(TESTS)/verifier $^
simulator: $(OBJS_DIR)/$(TESTS)/simulator.o $(VERIFIER_DEP)
	$(CC) $(FLAGS) $(LIBS) -o $(TESTS)/simulator $^

## Run the tests
run_verifier:
	$(TESTS)/verifier $(game)
run_simulator:
	$(TESTS)/simulator $(game)

Test_ThreadPool:
	$(UNIT_BIN)/Test_ThreadPool
Test_MemPool:
	$(UNIT_BIN)/Test_MemPool
Test_AnswerList:
	$(UNIT_BIN)/AnswerList_Test

Cache_Test:
	$(UNIT_BIN)/Cache_Test

.clean:	
	find $(OBJS_DIR) -type f -name '*.o' -delete && rm ares && rm $(UNIT_BIN)/*