CXX = g++
CXX_FLAGS = -std=c++17

all: release debug

directories: bin bin/release bin/debug

bin:
	mkdir -p bin

bin/release:
	mkdir -p bin/release

bin/debug:
	mkdir -p bin/debug

release: CXX_FLAGS += -O2
release: bin/release/main

debug: CXX_FLAGS += -g -pg -Og
debug: bin/debug/main

define RULES =

# COMMON
$(1)/cnf.o: cnf.cpp cnf.hpp directories
	g++ $${CXX_FLAGS} -c $$< -o $$@

$(1)/util.o: util.cpp util.hpp cnf.hpp directories
	g++ $${CXX_FLAGS} -c $$< -o $$@

# DPLL
$(1)/dpll.o: dpll/dpll.cpp dpll/dpll.hpp cnf.hpp directories
	g++ $${CXX_FLAGS} -c $$< -o $$@

$(1)/libdpll.a: $(1)/cnf.o $(1)/dpll.o
	ar -rv $$@ $$^

# CDCL
$(1)/vsids.o: cdcl/vsids.cpp cdcl/vsids.hpp cnf.hpp directories
	g++ $${CXX_FLAGS} -c $$< -o $$@

$(1)/graph.o: cdcl/graph.cpp cdcl/graph.hpp cnf.hpp directories
	g++ $${CXX_FLAGS} -c $$< -o $$@

$(1)/cdcl.o: cdcl/cdcl.cpp cnf.hpp cdcl/vsids.hpp cdcl/graph.hpp directories
	g++ $${CXX_FLAGS} -c $$< -o $$@

$(1)/libcdcl.a: $(1)/cnf.o $(1)/cdcl.o $(1)/vsids.o $(1)/graph.o
	ar -rv $$@ $$^

# MAIN
$(1)/main.o: main.cpp cnf.hpp util.hpp cdcl/cdcl.hpp dpll/dpll.hpp directories
	g++ $${CXX_FLAGS} -c $$< -o $$@

$(1)/main: $(1)/main.o $(1)/libcdcl.a $(1)/util.o $(1)/libdpll.a
	g++ $${CXX_FLAGS} $$^ -o $$@
endef

$(eval $(call RULES,bin/release))
$(eval $(call RULES,bin/debug))

clean:
	rm -f bin/{release,debug}/*
