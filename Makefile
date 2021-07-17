# MakeFile
# This file includes automatic dependency tracking
# Please see example of "utils" to see how to add additional file
# to your project

all: benchmarker

CXX=clang++
CC = $(CXX) -std=c++17 -g -ggdb3 -pthread -stdlib=libstdc++ -march=native

# Find OS
ifeq '$(findstring ;,$(PATH))' ';'
    detected_OS := Windows
else
    detected_OS := $(shell uname 2>/dev/null || echo Unknown)
    detected_OS := $(patsubst CYGWIN%,Cygwin,$(detected_OS))
    detected_OS := $(patsubst MSYS%,MSYS,$(detected_OS))
    detected_OS := $(patsubst MINGW%,MSYS,$(detected_OS))
endif

vpath %.cpp src:entropyEstimation:dataGenerators:utils


# Default optimization level
O ?= fast

# Flags and other libraries
override CFLAGS += -Wall -Wextra -pedantic -pthread -O$(O)
LDFLAGS = -L $(big_libs) -l$(small_libs)
LIBS = -lnlopt -lm -lswisstable
#-lfolly -lglog
INCLUDES = include
INCDIR = include
INC = $(INCDIR2) $(INCDIR) 
INC_PARAMS=LINE REDACTED FOR ANONYMITY
LINES REDACTED FOR ANONYMITY
big_libs = /usr/local/Cellar/boost/1.75.0_1/lib/
small_libs = boost_program_options

ifeq ($(FAST),1)
override CFLAGS += -fomit-frame-pointer
endif


ifeq ($(detected_OS),Darwin) # Mac OS X
    LDFLAGS += -framework CoreFoundation
endif

####### Automatic dependency magic #######
# Set-up dependency directory
DEPSDIR := .deps
BUILDSTAMP := $(DEPSDIR)/rebuildstamp
DEPFILES := $(wildcard $(DEPSDIR)/*.d)
ifneq ($(DEPFILES),)
include $(DEPFILES)
endif
DEPCFLAGS = -MD -MF $(DEPSDIR)/$*.d -MP

# Dependency compilation
ifneq ($(DEP_CC),$(CC) $(CFLAGS) $(DEPCFLAGS) $(O))
DEP_CC := $(shell mkdir -p $(DEPSDIR); echo >$(BUILDSTAMP); echo "DEP_CC:=$(CC) $(CFLAGS) $(DEPCFLAGS) $(O)" >$(DEPSDIR)/_cc.d)
endif

# Make sure dependency directories are generated
$(DEPSDIR)/stamp $(BUILDSTAMP):
	mkdir -p $(@D)
	touch $@

####### Automatic dependency magic #######
SRC := $(wildcard *.cpp entropyEstimation/*.cpp)
SRC := $(notdir $(SRC))
#SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)

%.o : %.cpp $(BUILDSTAMP)
	$(CC) $(CFLAGS) $(DEPCFLAGS) -O$(O) -o $@ -c $<

##
# To include additional non-executable files (e.g. selects.c, utils.c, etc),
# you'll need to add an additional build dependency to the file that requires
# the new file.  For example, see that client and server both require utils.o
#
# If you create a new file such as selects.c, then you will need a "selects.o"
# dependency on the right side of whichever one requires the file.
##

benchmarker: $(OBJ) 
	$(CC) $(CFLAGS) $(DEPCFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)


clean:
	rm -f benchmarker
	rm -rf .deps
	rm -rf *.o

distclean: clean
	rm -rf $(DEPSDIR)

.PHONY: all clean distclean

