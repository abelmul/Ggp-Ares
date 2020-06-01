RESNR_DIR =reasoner
GDL_DIR = gdl
STRATEGY_DIR = strategy

IDIR = ./include
REASONER_INC = $(IDIR)/$(RESNR_DIR)
GDL_INC = $(IDIR)/utils/$(GDL_DIR)
STRATEGY_INC = $(IDIR)/$(STRATEGY_DIR)


OBJS_DIR = ../objs

CC = g++
FLAGS = -Wall -std=c++11 -I$(IDIR) 

#Get the source files with their path
SOURCES := $(shell find .  -name '*.cpp' | sed 's/\.\///')

#Create their respective object files
OBJS := $(patsubst %.cpp, $(OBJS_DIR)/%.o, $(SOURCES))

#Create their respective include files
INCLS := $(shell find . -name "*.hh")

# common_dep = 
ARES_DEPS =  #$(OBJS_DIR)/$(STRATEGY_DIR)/<YOUR_STRATEGY>.o
ARES_DEPS += $(OBJS_DIR)/$(RESNR_DIR)/substitution.o $(OBJS_DIR)/ares.o 

#MAKE SURE TO COMPILE ARES FROM ALL OF THE OBJS IN OBJS. This is just temporary.
ares:  $(ARES_DEPS) #$(INCLS)
	$(CC) -o $@ $^ $(FLAGS)

#Compile any object file like objs/reasoner/reasoner.o from 
#reasoner/reasoner.cpp
$(OBJS_DIR)/%.o : %.cpp $(INCLS)
	$(CC) -c -o $@ $< $(FLAGS)

test: test.cpp
	g++ test.cpp -o test
	
clean:	
	rm $(OBJS) 2> /dev/null