# CS F214 Assignment 1 -- build rules (SPEC section 6)
#
# BASEFLAGS always apply. CXXFLAGS is for extras, so that
#   make CXXFLAGS+="-fsanitize=address -g" test
# adds to the base flags instead of replacing them (a command-line CXXFLAGS
# overrides any assignment made inside this file).

CXX       ?= g++
BASEFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude -Itests -MMD -MP
CXXFLAGS  ?=

LIB_SRCS  := src/tokenizer.cpp src/infix_prefix.cpp src/tree_build.cpp \
             src/traversal.cpp src/height.cpp src/eval.cpp src/cnf_reader.cpp
LIB_OBJS  := $(LIB_SRCS:.cpp=.o)

.PHONY: all test bench docs clean

all: logic_a1

logic_a1: src/main.o $(LIB_OBJS)
	$(CXX) $(BASEFLAGS) $(CXXFLAGS) $^ -o $@

tests/run_tests: tests/run_tests.o tests/gen.o $(LIB_OBJS)
	$(CXX) $(BASEFLAGS) $(CXXFLAGS) $^ -o $@

test: tests/run_tests
	./tests/run_tests

bench/bench: bench/bench.o tests/gen.o $(LIB_OBJS)
	$(CXX) $(BASEFLAGS) $(CXXFLAGS) $^ -o $@

bench: bench/bench
	./bench/bench bench/results.csv

docs:
	doxygen docs/Doxyfile

%.o: %.cpp
	$(CXX) $(BASEFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f src/*.o src/*.d tests/*.o tests/*.d bench/*.o bench/*.d \
	      logic_a1 tests/run_tests bench/bench
	rm -rf docs/html

-include $(wildcard src/*.d tests/*.d bench/*.d)
