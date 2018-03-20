SHELL = /bin/bash

TARGET = proja

CPP = g++
CPPFLAGS = -std=c++11 -g -c -o
FLAGS = -lpthread -lcrypto

INCLUDEDIR := -I.\
              -I./include\

SRCDIR    := $(shell find ./src -maxdepth 3 -type d)
SRCS      := $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.cpp))
BASE      := $(basename $(notdir $(SRCS)))
OBJS      := $(addsuffix .o,$(addprefix obj/,$(BASE)))
DEPS 			:= $(addsuffix .d, $(addprefix dep/,$(BASE)))
VPATH 		:= $(SRCDIR)


DEPFLAGS = -MM
DEPFILE = $@.$$$$

ifeq ("$(wildcard $(DIR_DEPS))", "")
	DEP_DIR_DEPS:=dep
endif


$(TARGET):$(OBJS)
		-rm -f $@
		$(CPP) -o $(TARGET) $(OBJS) $(FLAGS)


obj/%.o: %.cpp
		@ if test ! -d "obj"; then mkdir -p obj; else : ; fi
		$(CPP) $(CPPFLAGS) $@ $(FLAGS) $(INCLUDEDIR) $<


dep/%.d: %.cpp
		@if test ! -d "dep"; then mkdir -p dep; else : ; fi
		set -e; rm -f $@;
		$(CPP) $(DEPFLAGS) $(INCLUDEDIR) $< > $(DEPFILE); \
		#sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		sed 's,.*\.o[ :]*,$*.o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$

-include $(DEPS)


all:
	@ echo $(OBJS)
	@ echo $(SRCS)


.PHONY:clean
clean:
	-rm -f $(TARGET) obj/*.o dep/*.d
	-rm -rf obj dep
	-rm -f *.out
