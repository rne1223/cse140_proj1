# ------------------------------------------------
# Generic Makefile
#
# Author: yanick.rochon@gmail.com
# Date  : 2011-08-10
#
# Changelog :
#   2010-11-05 - first version
#   2011-08-10 - added structure : sources, objects, binaries
#                thanks to http://stackoverflow.com/users/128940/beta
#   2017-04-24 - changed order of linker params
#   2019-09-20 - Modified by Rene Tellez Rodriguez
# ------------------------------------------------

# project name (generate executable with this name)
TARGET   = sim
#target arguments
TARGS    = test.dump
TARGS2   = test2.dump
TARGS3   = sample.dump

CC       = gcc
# compiling flags here
CFLAGS   = -g -std=c99 -Wall -I.

LINKER   = gcc
# linking flags here
LFLAGS   = -Wall -I. -lm

# change these to proper directories where each file should be
SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

# compile source files
SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# linux commands
rm       = rm -f
mkdir 	 = mkdir -p


$(BINDIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	@$(LINKER) $(OBJECTS) $(LFLAGS) -o $@
	@echo "Linking complete!"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"


###########
# Rules
##########
.PHONY: clean remove debug run run2 run3 runi test

clean:
	@$(rm) $(OBJECTS)
	@echo "Cleanup complete!"

remove: clean
	@$(rm) $(BINDIR)/$(TARGET)
	@echo "Executable removed!

debug:
	 gdb --args $(BINDIR)/$(TARGET) $(SRCDIR)/$(TARGS)


runi:
	 $(BINDIR)/$(TARGET) -i $(SRCDIR)/$(TARGS) 

runi2:
	 $(BINDIR)/$(TARGET) -i $(SRCDIR)/$(TARGS2) 

runi3:
	 $(BINDIR)/$(TARGET) -i $(SRCDIR)/$(TARGS3) 

run:
	# @echo 
	 $(BINDIR)/$(TARGET) $(SRCDIR)/$(TARGS) 
run2:
	@echo 
	 $(BINDIR)/$(TARGET) $(SRCDIR)/$(TARGS2) 
run3:
	@echo 
	 $(BINDIR)/$(TARGET) $(SRCDIR)/$(TARGS3) 
test:
	@echo Running TESTS....
	 # $(BINDIR)/vimdiff sample.output test.output
	



