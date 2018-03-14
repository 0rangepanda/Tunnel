# NOTE: This is a makefile put under the src folder in old version


TARGET = proja

CPP = g++
CPPFLAGS = -std=c++11 -g -c -o

DIRS := $(shell find . -maxdepth 3 -type d)
SRCS = $(shell ls *.cpp)
SRCS := $(foreach dir,$(DIRS),$(wildcard $(dir)/*.cpp))
BASE = $(basename $(SRCS))
OBJS = $(addsuffix .o, $(addprefix obj/,$(BASE)))
DEPS = $(addsuffix .d, $(addprefix dep/,$(BASE)))

DEPFLAGS = -MM
DEPFILE = $@.$$$$

INCLUDEDIR := -I.\
              -I../include\

ifeq ("$(wildcard $(DIR_DEPS))", "")
	DEP_DIR_DEPS:=dep
endif


$(TARGET):$(OBJS)
		-rm -f $@
		$(CPP) -o $(TARGET) $(OBJS) -lpthread

obj/%.o: %.cpp
		@if test ! -d "obj"; then mkdir -p obj; else : ; fi
		$(CPP) $(CPPFLAGS) $@ -lpthread $(INCLUDEDIR) $<

dep/%.d: %.cpp
		@if test ! -d "dep"; then mkdir -p dep; else : ; fi
		set -e; rm -f $@;
		$(CPP) $(DEPFLAGS) $(INCLUDEDIR) $< > $(DEPFILE); \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$

#-include $(DEPS)


all:
	@ echo $(OBJS)
	@ echo $(SRCS)
	#$(SRCS) $(TARGET)

.PHONY:clean
clean:
	-rm -f $(TARGET) obj/*.o dep/*.d
	-rm -rf obj dep
