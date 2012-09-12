FILTER_OUT=$(foreach v, $(2), $(if $(findstring $(1), $(v)),,$(v)))

DBG_BUILD_DIR=../_builddbg
REL_BUILD_DIR=../_build

ifeq ($(MODE), REL)
COMPILE_OPTIONS=-DNDEBUG -O2
BUILD_DIR=$(REL_BUILD_DIR)
else
COMPILE_OPTIONS=-DDEBUG -D_DEBUG -fstack-check -g -ggdb 
BUILD_DIR=$(DBG_BUILD_DIR)
endif
WARNING_OPTIONS=-Wall -Wpointer-arith
COMPILE_OPTIONS += -ansi -DMAX_LOG_LINE=1500000 -DLINUX $(WARNING_OPTIONS)
LINK_OPTIONS=-Wl,-rpath=.
INCLUDES=-I../include

AR_OPTIONS=rc 

CFLAGS=$(COMPILE_OPTIONS) $(INCLUDES)

SRCS=$(wildcard *.cpp)
OBJS=$(addprefix $(BUILD_DIR)/, $(SRCS:.cpp=.o))
LIB=$(BUILD_DIR)/libtopn.a
MAIN=main
EXE=$(addprefix $(BUILD_DIR)/, Analyzer)


.PHONY: all clean debug release

debug: all
release: all
all: $(EXE) $(LIB)

$(EXE) : $(OBJS)
	$(CXX) -o $@ $^ $(LINK_OPTIONS)

$(BUILD_DIR):
	mkdir $@

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CFLAGS) -c -o $@ $<

$(LIB): $(OBJS) | $(BUILD_DIR)
	$(AR) $(AR_OPTIONS) $@ $(call FILTER_OUT, $(MAIN), $^)

clean:
	-$(RM) -r $(DBG_BUILD_DIR) $(REL_BUILD_DIR)
