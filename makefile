RESNR_DIR =reasoner
STRATEGY_DIR = strategy
UTILS_DIR = utils
GDL_DIR =$(UTILS_DIR)/gdl
GDLP_DIR=$(GDL_DIR)/gdlParser
MEMORY_DIR = $(UTILS_DIR)/memory
THREADING = $(UTILS_DIR)/threading
GAME = $(UTILS_DIR)/game
TESTS = tests
TESTS_SIMP = $(TESTS)/simple

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

CC = g++
FLAGS = -Wall -std=c++17 -I$(IDIR) 
LIBS =  -lboost_regex -lboost_thread -lpthread
ifdef debug
FLAGS+= -ggdb
else
FLAGS+= -O3
endif

ifdef testing
FLAGS+=  -I$(TESTS_SIMP)/include
endif

#Get the source files with their path
SOURCES := $(shell find . -not -path "*test*" -name '*.cpp' | sed 's/\.\///')
# TESTS := $(shell find .  -path "*test*" -name '*.cpp' | sed 's/\.\///')

#Create their respective object files
OBJS := $(patsubst %.cpp, $(OBJS_DIR)/%.o, $(SOURCES))
# OBJS_TESTS := $(patsubst %.cpp, $(OBJS_DIR)/%.o, $(TESTS))
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
# OBJ_TESTS_DIRS := $(shell mkdir -p `dirname $(OBJS_TESTS)`)

ares:  $(OBJS) 
	$(CC) $(FLAGS) $(LIBS) -o $@ -Wl,--start-group $^ -Wl,--end-group 

$(OBJS_DIR)/ares.o: ares.cpp $(INCLS)
	$(CC) -c $(FLAGS) -o $@ $< 

$(OBJS_DIR)/$(THREADING)/loadBalancer.o: $(THREADING)/loadBalancer.cpp $(THREADING_INCS) $(COMMON_INCS)
	$(CC) -c $(FLAGS) -o $@ $< 

$(OBJS_DIR)/$(UTILS_DIR)/hashing.o : $(UTILS_DIR)/hashing.cpp $(COMMON_INCS)
	$(CC) -c $(FLAGS) -o $@ $< 
$(OBJS_DIR)/$(GAME)/visualizer.o : $(GAME)/visualizer.cpp $(GAME_INCS) $(COMMON_INCS)
	$(CC) -c $(FLAGS) -o $@ $<
$(OBJS_DIR)/$(MEMORY_DIR)/expressionPool.o : $(MEMORY_DIR)/expressionPool.cpp  $(COMMON_INCS)
	$(CC) -c $(FLAGS) -o $@ $<
$(OBJS_DIR)/$(MEMORY_DIR)/memoryPool.o : $(MEMORY_DIR)/memoryPool.cpp  $(COMMON_INCS) $(THREADING_IDIR)/locks.hh
	$(CC) -c $(FLAGS) -o $@ $<
$(OBJS_DIR)/$(GDL_DIR)/structuredTerm.o : $(GDL_DIR)/structuredTerm.cpp $(INCLS)
	$(CC) -c $(FLAGS) -o $@ $<
$(OBJS_DIR)/$(GDLP_DIR)/transformer.o : $(GDLP_DIR)/transformer.cpp $(GDLP_INCS)  $(GAME_INCS) $(COMMON_INCS)
	$(CC) -c $(FLAGS) -o $@ $<
$(OBJS_DIR)/$(GDLP_DIR)/gdlParser.o : $(GDLP_DIR)/gdlParser.cpp $(GDLP_INCS)  $(COMMON_INCS)
	$(CC) -c $(FLAGS) -o $@ $<
$(OBJS_DIR)/$(RESNR_DIR)/prover.o :$(RESNR_DIR)/prover.cpp $(INCLS)
	$(CC) -c $(FLAGS) -o $@ $<
$(OBJS_DIR)/$(RESNR_DIR)/reasoner.o: $(RESNR_DIR)/reasoner.cpp $(INCLS)
	$(CC) -c $(FLAGS) -o $@ $<
$(OBJS_DIR)/$(RESNR_DIR)/substitution.o: $(RESNR_DIR)/substitution.cpp  $(REASONER_IDIR)/substitution.hh $(COMMON_INCS)
	$(CC) -c $(FLAGS) -o $@ $<
$(OBJS_DIR)/$(RESNR_DIR)/suffixRenamer.o:$(RESNR_DIR)/suffixRenamer.cpp $(REASONER_IDIR)/substitution.hh $(COMMON_INCS)
	$(CC) -c $(FLAGS) -o $@ $<

$(OBJS_DIR)/$(RESNR_DIR)/unifier.o:$(RESNR_DIR)/unifier.cpp $(REASONER_IDIR)/substitution.hh $(COMMON_INCS)
	$(CC) -c $(FLAGS) -o $@ $<
$(OBJS_DIR)/$(UTILS_DIR)/httpHandler.o:$(UTILS_DIR)/httpHandler.cpp 
	$(CC) -c $(FLAGS) -o $@ $<

#Compile any object file like objs/reasoner/reasoner.o from 
#reasoner/reasoner.cpp
# $(OBJS_DIR)/%.o : %.cpp $(INCLS)
# 	$(CC) -c $(FLAGS) -o $@ $< 

$(TESTS_SIMP)/Test_ThreadPool: $(OBJS_DIR)/$(TESTS_SIMP)/Test_ThreadPool.o $(OBJS_DIR)/$(THREADING)/threadPool.o
	$(CC) $(FLAGS) -o $@ -Wl,--start-group $^ -Wl,--end-group 

$(TESTS_SIMP)/Test_MemPool: $(OBJS_DIR)/$(TESTS_SIMP)/Test_MemPool.o $(OBJS_DIR)/$(MEMORY_DIR)/memoryPool.o $(OBJS_DIR)/$(UTILS_DIR)/hashing.o $(OBJS_DIR)/$(RESNR_DIR)/substitution.o
	$(CC) $(FLAGS)  -o $@ -Wl,--start-group $^ -Wl,--end-group 

Test_ThreadPool:
	$(TESTS_SIMP)/Test_ThreadPool
Test_MemPool:
	$(TESTS_SIMP)/Test_MemPool

.clean:	
	find $(OBJS_DIR) -type f -name '*.o' -delete && rm ares