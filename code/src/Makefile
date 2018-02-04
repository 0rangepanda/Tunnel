TARGET = proja

CPP = g++
CPPFLAGS = -std=c++11 -c -o

SRCS = $(shell ls *.cpp)
BASE = $(basename $(SRCS))
OBJS = $(addsuffix .o, $(addprefix obj/,$(BASE)))
DEPS = $(addsuffix .d, $(addprefix dep/,$(BASE)))

DEPFLAGS = -MM
DEPFILE = $@.$$$$

INCLUDEDIR := -I.\
              -I../include\

$(TARGET):$(OBJS)
		-rm -f $@
		$(CPP) -o $(TARGET) $(OBJS)

obj/%.o: %.cpp
		@if test ! -d "obj"; then mkdir -p obj; else : ; fi
		$(CPP) $(CPPFLAGS) $@ $(INCLUDEDIR) $<

dep/%.d: %.cpp
		@if test ! -d "dep"; then mkdir -p dep; else : ; fi
		set -e; rm -f $@;
		$(CPP) $(DEPFLAGS) $(INCLUDEDIR) $< > $(DEPFILE); \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$

include $(DEPS)

all: $(SRCS) $(TARGET)

.PHONY:clean
clean:
	-rm -f $(TARGET) obj/*.o dep/*.d
	-rm -rf obj dep
