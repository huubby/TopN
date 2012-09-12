ifeq ($(MODE), REL)
COMPILE_OPTIONS = -DNDEBUG
BUILD_DIR = ../_build
else
COMPILE_OPTIONS = -g -DDEBUG
BUILD_DIR = ../_builddbg
endif
COMPILE_OPTIONS += -ansi -Wall -DMAX_LOG_LINE=1500000 -DLINUX
LINK_OPTIONS = -Wl,-rpath=.
INCLUDES = -I../include
LIBS = -L$(BUILD_DIR) \
	   -ltopn

CFLAGS = $(COMPILE_OPTIONS) $(INCLUDES)

SRC = parse_log_test.cpp
OBJS = $(addprefix $(BUILD_DIR)/, $(SRC:.cpp=.o))
EXE = $(addprefix $(BUILD_DIR)/, $(basename $(SRC)))

.PHONY: test clean

test: $(EXE)

$(EXE) : $(OBJS)
	$(CXX) -o $@ $^ $(LINK_OPTIONS) $(LIBS)

$(BUILD_DIR) :
	mkdir $@

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(OBJS) $(EXE)
