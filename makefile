RESNR_DIR =reasoner
GDL_DIR = gdl
GDLP_DIR = gdlParser
STRATEGY_DIR = strategy
UTILS_DIR = utils

TESTS = tests
TESTS_SIMP = $(TESTS)/simple

IDIR = ./include
REASONER_INC = $(IDIR)/$(RESNR_DIR)
GDL_INC = $(IDIR)/$(UTILS_DIR)/$(GDL_DIR)
STRATEGY_INC = $(IDIR)/$(STRATEGY_DIR)
THREADING = $(UTILS_DIR)/threading

OBJS_DIR = ../objs

CC = g++
FLAGS = -Wall -std=c++17 -I$(IDIR) -lboost_regex -lboost_thread -lpthread 

ifdef debug
FLAGS+= -ggdb
else
FLAGS+= -O3
endif

#Get the source files with their path
SOURCES := $(shell find . -not -path "*test*" -name '*.cpp' | sed 's/\.\///')

#Create their respective object files
OBJS := $(patsubst %.cpp, $(OBJS_DIR)/%.o, $(SOURCES))

#Create their respective include files
INCLS := $(shell find . -name "*.hh")

# common_dep = 
# ARES_DEPS =  #$(OBJS_DIR)/$(STRATEGY_DIR)/<YOUR_STRATEGY>.o
# ARES_DEPS +=  $(OBJS_DIR)/$(UTILS_DIR)/$(GDL_DIR)/$(GDLP_DIR)/gdlParser.o $(OBJS_DIR)/$(UTILS_DIR)/hashing.o $(OBJS_DIR)/$(RESNR_DIR)/substitution.o $(OBJS_DIR)/ares.o 


ares:  $(OBJS) #$(INCLS)
	$(CC) $(FLAGS) -o $@ -Wl,--start-group $^ -Wl,--end-group 

#Compile any object file like objs/reasoner/reasoner.o from 
#reasoner/reasoner.cpp
$(OBJS_DIR)/%.o : %.cpp $(INCLS)
	$(CC) -c $(FLAGS) -o $@ $< 

$(TESTS_SIMP)/Test_ThreadPool: $(OBJS_DIR)/$(TESTS_SIMP)/Test_ThreadPool.o $(OBJS_DIR)/$(THREADING)/threadPool.o
	$(CC) $(FLAGS) -o $@ -Wl,--start-group $^ -Wl,--end-group 

run_simpTest:
	$(TESTS_SIMP)/Test_ThreadPool
clean:	
	find $(OBJS_DIR) -type f -name '*.o' -delete