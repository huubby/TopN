#Project: Uba
CXX = gcc

COMPILE_EXE = -g -Wall -std=gnu99 

CFLAGS = $(INCS) $(COMPILE_EXE)

LINK_EXE = -Wl,-rpath=. -Wall 

LIBS = $(LINK_EXE) 


SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
BIN = ./topn

RM = rm -f


.PHONY: all clean


all: $(OBJS)
	$(CXX) $(OBJS) -o $(BIN) $(LIBS)

clean:
	$(RM) $(OBJS) $(BIN)

%.o: %.c
	$(CXX) $(CFLAGS) -c -o $@ $<

