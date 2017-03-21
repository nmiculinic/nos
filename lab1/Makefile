SRCS = $(wildcard *.cpp)
PROGS = $(patsubst %.cpp,%,$(SRCS))
all: $(PROGS)
%: %.c
	clang++ --std=c++11  -Wall -o $@ $<
