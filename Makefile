CXX = g++
CXX_FLAGS = -g -std=c++17 -O2 
CC = cc
SOURCES = cnf.cpp util.cpp dpll/dpll.cpp cdcl/cdcl.cpp cdcl/vsids.cpp cdcl/


%.d: %.cpp
	${CC} -MM ${CXX_FLAGS} $< | sed -e "s/\(.*.o\):/\1 $@:/g" > $@

%.o: %.d %.cpp
	${CXX} ${CXX_FLAGS} -c $(shell echo $< | sed -e "s/\(.*\).d/\1.cpp/g") -o $@

# CXX_FLAGS = -g -std=c++17 -pg -g

.PHONY: all
all: bin/main

.PHONY: dpll
dpll: bin/libdpll.a

.PHONY: cdcl
cdcl: bin/libcdcl.a


# COMMON
bin/cnf.o: cnf.cpp cnf.hpp
	g++ ${CXX_FLAGS} -c $< -o $@

bin/util.o: util.cpp util.hpp cnf.hpp
	g++ ${CXX_FLAGS} -c $< -o $@

# DPLL
bin/dpll.o: dpll/dpll.cpp dpll/dpll.hpp cnf.hpp
	g++ ${CXX_FLAGS} -c $< -o $@

bin/libdpll.a: bin/cnf.o bin/dpll.o
	ar -rv $@ $^

# CDCL
bin/vsids.o: cdcl/vsids.cpp cdcl/vsids.hpp cnf.hpp
	g++ ${CXX_FLAGS} -c $< -o $@

bin/graph.o: cdcl/graph.cpp cdcl/graph.hpp cnf.hpp
	g++ ${CXX_FLAGS} -c $< -o $@

bin/cdcl.o: cdcl/cdcl.cpp cnf.hpp cdcl/vsids.hpp cdcl/graph.hpp
	g++ ${CXX_FLAGS} -c $< -o $@

bin/libcdcl.a: bin/cnf.o bin/cdcl.o bin/vsids.o bin/graph.o
	ar -rv $@ $^

# MAIN
bin/main.o: main.cpp cnf.hpp util.hpp cdcl/cdcl.hpp dpll/dpll.hpp
	g++ ${CXX_FLAGS} -c $< -o $@

bin/main: bin/main.o bin/libdpll.a bin/libcdcl.a bin/util.o
	g++ ${CXX_FLAGS} $^ -o $@

clean:
	rm -f bin/*
