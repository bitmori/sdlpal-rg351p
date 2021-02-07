# General makefile for generic unix & linux platforms

TARGET = sdlpal

INTER=.unix

HOST =

GENERATED = -DPAL_HAS_GIT_REVISION $(shell ../scripts/gengitrev)

DEPFLAGS = -MT $@ -MMD -MP -MF $*$(INTER).Td

#SOURCES = ../src ../src/adplug ../src/liboggvorbis/src ../src/libopusfile/src ../src/libopusfile/celt ../src/libopusfile/silk ../src/libopusfile/silk/float ../src/libmad
SOURCES = ../src ../src/adplug ../src/libmad ../src/duktape
CFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.c))
CPPFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.cpp)) unix.cpp
TOBJFILES = $(CFILES:.c=$(INTER).o) $(CPPFILES:.cpp=$(INTER).o)
DEPFILES = $(OBJFILES:.o=.d)
TEST_CPPFILES = $(wildcard ../tests/*.cpp)
TEST_OBJFILES = $(TEST_CPPFILES:.cpp=.o)
SDL_CONFIG = sdl2-config


FLTK_CXXFLAGS = -DPAL_NO_LAUNCH_UI
FLTK_CFLAGS =
FLTK_LDFLAGS =


CC = $(HOST)gcc
CXX = $(HOST)g++
override CCFLAGS += `$(SDL_CONFIG) --cflags` -D_GNU_SOURCE -g -Wall -O2 -fno-strict-aliasing $(EXTRA_CCFLAGS) $(GENERATED)
CXXFLAGS = $(CCFLAGS) -std=c++11 $(FLTK_CXXFLAGS)
CFLAGS = $(CCFLAGS) -std=gnu99 $(FLTK_CFLAGS)
LDFLAGS = `$(SDL_CONFIG) --libs` $(FLTK_LDFLAGS) -lstdc++ -lm

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LDFLAGS += -framework OpenGL
	OBJFILES = $(filter-out ../src/mini_glloader.unix.o, $(TOBJFILES))
else
	LDFLAGS += -lGL -pthread
	OBJFILES = $(TOBJFILES)
endif

POSTCOMPILE = @mv -f $*$(INTER).Td $*$(INTER).d && touch $@

.PHONY : all clean

all: $(TARGET)

$(TARGET): $(OBJFILES)
	@echo [LD] $@
	@$(CXX) $^ -o $@ $(LDFLAGS)

%$(INTER).o: %.c %$(INTER).d
	@echo [CC] $<
	@$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@
	$(POSTCOMPILE)

%$(INTER).o: %.cpp %$(INTER).d
	@echo [CC] $<
	@$(CXX) $(DEPFLAGS) $(CXXFLAGS) -c $< -o $@
	$(POSTCOMPILE)

clean:
	-@rm -f $(TARGET) $(TEST_TARGET) $(OBJFILES) $(TEST_OBJFILES) $(DEPFILES)

%.d: ;
.PRECIOUS: %.d

-include $(DEPFILES)

#../src/liboggvorbis/%$(INTER).o: EXTRA_CCFLAGS := -I../src/liboggvorbis/include -I../src/liboggvorbis/src
#../src/libopusfile/%$(INTER).o: EXTRA_CCFLAGS := -I../src/liboggvorbis/include -I../src/libopusfile/include -I../src/libopusfile/src -I../src/libopusfile/celt -I../src/libopusfile/silk -I../src/libopusfile/silk/float -DHAVE_CONFIG_H
%$(INTER).o: EXTRA_CCFLAGS := -I. -I.. -I../src -I../src/duktape -I../src/liboggvorbis/include -I../src/libopusfile/include -DPAL_HAS_PLATFORM_SPECIFIC_UTILS