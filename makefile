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
TESTS_STRESS = $(TESTS)/stress
STRESS_BIN = $(TESTS_STRESS)/bin
TESTS_UNIT_INC = $(TESTS_UNIT)/include
UNIT_BIN = $(TESTS_UNIT)/bin

#EXTERNAL LIBRARIES
CPPREST_INC = ./lib/cpprestsdk/Release/include
CPPREST_SO = ./lib/cpprestsdk/build.debug/Release/Binaries

#create unit tests binary directory
UNIT_BIN_DIR := $(shell mkdir -p $(UNIT_BIN))
STRESS_BIN_DIR := $(shell mkdir -p $(STRESS_BIN))
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
FLAGS = 
ifdef DEBUG_ARES
FLAGS+= -ggdb
else
FLAGS+= -Ofast -march=native
endif


FLAGS +=  -Wall -Wextra -std=c++17 -I$(IDIR) -I$(CPPREST_INC)
LIBS =  -L$(CPPREST_SO) -lboost_regex -lboost_thread -lboost_chrono -lpthread -ltbb  -lboost_system -lcrypto -lssl -lcpprest


ifdef testing
FLAGS+=  -I$(TESTS_UNIT)/include
endif

#Get the source files with their path
SOURCES := $(shell find . -not -path "*test*" -not -path "*lib*" -name '*.cpp' | sed 's/\.\///')
TEST_FILES := $(shell find .  -path "*test*" -name '*.cpp' | sed 's/\.\///')

#Create their respective object files
OBJS := $(patsubst %.cpp, $(OBJS_DIR)/%.o, $(SOURCES))
OBJS_TESTS := $(patsubst %.cpp, $(OBJS_DIR)/%.o, $(TEST_FILES))
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
OBJ_TEST_DIRS := $(shell mkdir -p `dirname $(OBJS_TESTS)`)
setup:
	mkdir -p lib/cpprestsdk/build.debug
	cd lib/cpprestsdk/build.debug && cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Debug && ninja

all: ares

ares:  $(OBJS) #$(INCLS)
	$(CC) $(FLAGS) $(LIBS) -o $@ -Wl,--start-group $^ -Wl,--end-group 
$(OBJS_DIR)/%.o : %.cpp $(INCLS)
	$(CC) -c $(FLAGS) -o $@ $< 
run:
	export LD_LIBRARY_PATH=$(CPPREST_SO)${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} && ./ares
vis_server:
	cd visualization && npm start

#######################################################################################################
#######################################################################################################
#												TESTS											      #
#######################################################################################################
#######################################################################################################
#These are to make and run tests
TEST_OBJS = $(OBJS_DIR)/$(RESNR_DIR)/renamers.o $(OBJS_DIR)/$(GDL_DIR)/structuredTerm.o $(OBJS_DIR)/$(MEMORY_DIR)/memoryPool.o $(OBJS_DIR)/$(UTILS_DIR)/hashing.o $(OBJS_DIR)/$(RESNR_DIR)/substitution.o $(OBJS_DIR)/$(MEMORY_DIR)/memCache.o
VERIFIER_DEP = ../objs/gameAnalyzer/gameAnalyzer.o ../objs/utils/utils/iterators.o ../objs/$(STRATEGY_DIR)/montecarlo.o ../objs/utils/game/game.o ../objs/utils/threading/threading.o ../objs/utils/hashing.o ../objs/utils/game/visualizer.o ../objs/utils/memory/memCache.o ../objs/utils/memory/memoryPool.o ../objs/utils/gdl/structuredTerm.o ../objs/utils/gdl/gdlParser/transformer.o ../objs/utils/gdl/gdlParser/gdlParser.o ../objs/utils/httpHandler.o $(OBJS_DIR)/$(RESNR_DIR)/prover.o $(OBJS_DIR)/$(RESNR_DIR)/reasoner.o $(OBJS_DIR)/$(RESNR_DIR)/substitution.o $(OBJS_DIR)/$(RESNR_DIR)/renamers.o $(OBJS_DIR)/$(RESNR_DIR)/unifier.o
MONTE_DEP =  $(VERIFIER_DEP)
PARSER_DEP = $(VERIFIER_DEP)


# Test_MemPool: $(OBJS_DIR)/$(TESTS_UNIT)/Test_MemPool.o $(TEST_OBJS)
# 	$(CC) $(FLAGS)  -o $(UNIT_BIN)/MemPool_Test -Wl,--start-group $^ -Wl,--end-group 

unit_tests: answerListTest cacheTest monteTest parserTest verifier


answerListTest: $(OBJS_DIR)/$(TESTS_UNIT)/answerListTest.o $(TEST_OBJS)
	$(CC) $(FLAGS) $(LIBS)  -I $(TESTS_UNIT_INC) -o $(UNIT_BIN)/answerListTest -Wl,--start-group $^ -Wl,--end-group 

cacheTest: $(OBJS_DIR)/$(TESTS_UNIT)/cacheTest.o $(TEST_OBJS)
	$(CC) $(FLAGS) $(LIBS)  -I $(TESTS_UNIT_INC) -o $(UNIT_BIN)/cacheTest -Wl,--start-group $^ -Wl,--end-group 

monteTest: $(OBJS_DIR)/$(TESTS_UNIT)/monteTest.o  $(OBJS_DIR)/$(TESTS_UNIT)/mock_reasoner.o $(MONTE_DEP)
	$(CC) $(FLAGS) $(LIBS)  -I $(TESTS_UNIT_INC) -o $(UNIT_BIN)/monteTest -Wl,--start-group $^ -Wl,--end-group 
parserTest: $(OBJS_DIR)/$(TESTS_UNIT)/parserTest.o $(PARSER_DEP)
	$(CC) $(FLAGS) $(LIBS)  -I $(TESTS_UNIT_INC) -o $(UNIT_BIN)/parserTest -Wl,--start-group $^ -Wl,--end-group 
verifier: $(OBJS_DIR)/$(TESTS_UNIT)/verifier.o $(VERIFIER_DEP)
	$(CC) $(FLAGS) $(LIBS) -o $(UNIT_BIN)/verifier  -Wl,--start-group $^ -Wl,--end-group

# run_memPool_test:
# 	$(UNIT_BIN)/Test_MemPool
run_unit_tests: run_answerListTest run_cacheTest run_monteTest run_parserTest

run_verifier:
	export LD_LIBRARY_PATH=$(CPPREST_SO)${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} && $(UNIT_BIN)/verifier $(game)
run_answerListTest:
	$(UNIT_BIN)/answerListTest
run_cacheTest:
	$(UNIT_BIN)/cacheTest
run_monteTest:
	$(UNIT_BIN)/monteTest
run_parserTest:
	$(UNIT_BIN)/parserTest

#Tests involving Random simulation and verification of the reasoner

stress_tests: simulator strategy_test

simulator: $(OBJS_DIR)/$(TESTS_STRESS)/simulator.o $(VERIFIER_DEP)
	$(CC) $(FLAGS) $(LIBS) -o $(STRESS_BIN)/simulator  -Wl,--start-group $^ -Wl,--end-group
strategy_test:$(OBJS_DIR)/$(TESTS_STRESS)/strategy_test.o $(VERIFIER_DEP)  
	$(CC) $(FLAGS) $(LIBS) -o $(STRESS_BIN)/strategy_test  -Wl,--start-group $^ -Wl,--end-group
## Run the tests

run_stress_tests:run_simulator run_strategy_test
run_simulator:
	export LD_LIBRARY_PATH=$(CPPREST_SO)${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} && $(STRESS_BIN)/simulator $(game)
run_strategy_test:
	export LD_LIBRARY_PATH=$(CPPREST_SO)${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} && $(STRESS_BIN)/strategy_test 

$(OBJS_DIR)/$(TESTS_UNIT)/answerListTest.o: $(TESTS_UNIT)/answerListTest.cpp $(INCLS)
	$(CC) -c $(FLAGS) -I $(TESTS_UNIT_INC) -o $@ $<

$(OBJS_DIR)/$(TESTS_UNIT)/cacheTest.o: $(TESTS_UNIT)/cacheTest.cpp $(INCLS)
	$(CC) -c $(FLAGS) -I $(TESTS_UNIT_INC) -o $@ $<

$(OBJS_DIR)/$(TESTS_UNIT)/verifier.o : $(TESTS_UNIT)/verifier.cpp $(INCLS)
	$(CC) -c $(FLAGS) -I $(TESTS_UNIT_INC) -o $@ $<

$(OBJS_DIR)/$(TESTS_STRESS)/simulator.o : $(TESTS_STRESS)/simulator.cpp $(INCLS)
	$(CC) -c $(FLAGS) -I $(TESTS_UNIT_INC) -o $@ $<
$(OBJS_DIR)/$(TESTS_STRESS)/strategy_test.o : $(TESTS_STRESS)/strategy_test.cpp $(INCLS)
	$(CC) -c $(FLAGS) -I $(TESTS_UNIT_INC) -o $@ $<
$(OBJS_DIR)/$(TESTS_UNIT)/monteTest.o: $(TESTS_UNIT)/monteTest.cpp $(INCLS)
	$(CC) -c $(FLAGS) -I $(TESTS_UNIT_INC) -o $@ $<
$(OBJS_DIR)/$(TESTS_UNIT)/mock_reasoner.o: $(TESTS_UNIT)/mock_reasoner.cpp $(INCLS)
	$(CC) -c $(FLAGS) -I $(TESTS_UNIT_INC) -o $@ $<
$(OBJS_DIR)/$(TESTS_UNIT)/parserTest.o: $(TESTS_UNIT)/parserTest.cpp
	$(CC) -c $(FLAGS) -I $(TESTS_UNIT_INC) -o $@ $<

.clean:	
	find $(OBJS_DIR) -type f -name '*.o' -delete 
	rm -f ares
	rm -f $(UNIT_BIN)/* 
	rm -f $(STRESS_BIN)/* 

.nuke:
	rm -rf $(OBJS_DIR)
	rm -rf $(STRESS_BIN)
	rm -rf $(UNIT_BIN)
	rm -f ares
