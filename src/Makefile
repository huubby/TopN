FILTER_OUT=$(foreach v, $(2), $(if $(findstring $(1), $(v)),,$(v)))

DBG_BUILD_DIR=../_builddbg
REL_BUILD_DIR=../_build

ifeq ($(MODE), REL)
COMPILE_OPTIONS=-DNDEBUG -O2
BUILD_DIR=$(REL_BUILD_DIR)
TARGETS=analyzer combiner
else
COMPILE_OPTIONS=-DDEBUG -D_DEBUG -fstack-check -g -ggdb 
BUILD_DIR=$(DBG_BUILD_DIR)
TARGETS=analyzerd combinerd
endif
WARNING_OPTIONS=-Wall -Wpointer-arith
COMPILE_OPTIONS+=-ansi -DMAX_LOG_LINE=1500000 -DLINUX $(WARNING_OPTIONS)
LINK_OPTIONS=-Wl,-rpath=.
INCLUDES=-I../include

AR_OPTIONS=rc 

CFLAGS=$(COMPILE_OPTIONS) $(INCLUDES)

SRCS=$(wildcard *.cpp)
OBJS=$(addprefix $(BUILD_DIR)/, $(SRCS:.cpp=.o))
LIB=$(BUILD_DIR)/libtopn.a
MAIN=analyzer combiner
EXE=$(addprefix $(BUILD_DIR)/, $(TARGETS))

.PHONY: all clean analyzerd analyzer combinerd combiner

all: $(EXE) $(LIB)

$(EXE) : $(TARGETS)

analyzerd analyzer: $(call FILTER_OUT,combiner,$(OBJS))
	$(CXX) -o $(addprefix $(BUILD_DIR)/, $@) $^ $(LINK_OPTIONS)

combinerd combiner: $(call FILTER_OUT,analyzer,$(OBJS))
	$(CXX) -o $(addprefix $(BUILD_DIR)/, $@) $^ $(LINK_OPTIONS)

$(BUILD_DIR):
	mkdir $@

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CFLAGS) -c -o $@ $<

$(LIB): $(OBJS) | $(BUILD_DIR)
	$(AR) $(AR_OPTIONS) $@ $(call FILTER_OUT, $(MAIN), $^)

clean:
	-$(RM) -r $(DBG_BUILD_DIR) $(REL_BUILD_DIR)
