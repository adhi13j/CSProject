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
# The benchmark links a second copy of the library compiled with -DBENCH_INSTRUMENT
# (stack/queue/recursion counters, src/instrument.h). The normal build has no counters.
BENCH_OBJS := $(patsubst src/%.cpp,bench/obj/%.o,$(LIB_SRCS))

.PHONY: all test bench docs dist clean

all: logic_a1

logic_a1: src/main.o $(LIB_OBJS)
	$(CXX) $(BASEFLAGS) $(CXXFLAGS) $^ -o $@

tests/run_tests: tests/run_tests.o tests/gen.o $(LIB_OBJS)
	$(CXX) $(BASEFLAGS) $(CXXFLAGS) $^ -o $@

test: tests/run_tests
	./tests/run_tests

bench/bench: bench/bench.o tests/gen.o $(BENCH_OBJS)
	$(CXX) $(BASEFLAGS) -DBENCH_INSTRUMENT $(CXXFLAGS) $^ -o $@

# Crash-depth search runs against the UNinstrumented library, so the frame sizes
# it measures are those of the real code.
bench/crash_search: bench/crash_search.o tests/gen.o $(LIB_OBJS)
	$(CXX) $(BASEFLAGS) $(CXXFLAGS) $^ -o $@

bench: bench/bench bench/crash_search
	./bench/bench bench/results.csv
	./bench/crash_search bench/results.csv

bench/obj/%.o: src/%.cpp
	@mkdir -p bench/obj
	$(CXX) $(BASEFLAGS) -DBENCH_INSTRUMENT $(CXXFLAGS) -c $< -o $@

bench/bench.o: bench/bench.cpp
	$(CXX) $(BASEFLAGS) -DBENCH_INSTRUMENT $(CXXFLAGS) -c $< -o $@

docs:
	doxygen docs/Doxyfile

# Submission archive (SPEC section 11):  make dist BITSID=<your BITS ID>
# Built from the working tree on purpose: docs/html is git-ignored but must be shipped.
# No binaries, no .o files, no raw SAT files (data/README.md lists them), no .lake.
BITSID ?= BITSID
dist: docs
	rm -f $(BITSID)_LOGIC_A1.zip
	zip -rq $(BITSID)_LOGIC_A1.zip include src tests bench docs lean data/README.md Makefile README.md SPEC.md \
	    -x '*.o' '*.d' 'tests/run_tests' 'bench/bench' 'bench/crash_search' 'bench/obj/*' \
	       'tests/data/_scratch.cnf' 'lean/.lake/*' 'lean/.lake' '*/.lake/*'
	@echo "created $(BITSID)_LOGIC_A1.zip"

%.o: %.cpp
	$(CXX) $(BASEFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f src/*.o src/*.d tests/*.o tests/*.d bench/*.o bench/*.d \
	      logic_a1 tests/run_tests bench/bench bench/crash_search
	rm -rf docs/html bench/obj

-include $(wildcard src/*.d tests/*.d bench/*.d bench/obj/*.d)
