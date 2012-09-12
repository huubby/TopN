CXX = g++

COMPILE_EXE = -g -Wall -DMAX_LOG_LINE=1500000

CFLAGS = $(INCS) $(COMPILE_EXE)

LINK_EXE = -Wl,-rpath=. -Wall 

LIBS = $(LINK_EXE) 


SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
BIN = ./topn

RM = rm -f


.PHONY: all clean


all: $(OBJS)
	$(CXX) $(OBJS) -o $(BIN) $(LIBS)

clean:
	$(RM) $(OBJS) $(BIN)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c -o $@ $<
