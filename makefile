#
# Generic Makefile
# by Leonidas Tsaousis
# for Unix Systems
# 
# Command line calls:
#	make			(self explanatory)
#	make BIN=<name>	to name the execuble (default: PassCrack) 
#	make DBG=1		to add debugging info and extra warnings 
#	make clean		to erase everything except the source
#	make wipe		to erase everything regarding this project - DISABLED to avoid accidents
#	make count		to calculate the amount of code 
#		


CPP		= g++
CC		= gcc
RM		= rm
WC		= wc -l

SRC		= main.cpp blake.cpp rainbowfuncts.cpp								
OBJ		= $(SRC:.c=.o)
INC		= blake.hpp
BIN		= PassCrack

DBG		= 0



ifeq ($(DBG),1)												#Stuff programmer cares about
	FLAGS	= -W -Wall -Wextra -Wshadow -g
else														#...but user does not!
	FLAGS	= -s -w
endif


	
.PHONY: all clean count 									#to avoid shadows


all: $(BIN)													#default rule


$(BIN): $(OBJ)												#link object files
	$(CPP) $(OBJ) $(FLAGS) -o $@ -std=c++11

.c.o :	$(INC)												#compile source files (interfaces dependent)
	$(CPP) $(FLAGS) -c $<



count:														#other rules
	echo LINES OF CODE: ; $(WC) $(SRC) $(INC) 

# wipe:
#	$(RM) $(OBJ) $(BIN) $(SRC) $(ADTS) $(INC) $(ARH) $(AR) 

clean:
	$(RM) $(OBJ) $(BIN)


	
	