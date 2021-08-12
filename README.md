## Description

Simple SAT Solver implemented by C++.

Following algorithms are used.

- Conflict Driven Clause Learning (CDCL)
- Backjump
- 2-watching literal
- Restart strategy based on LBD(Literal Block Distance)
- Phase caching
- VSIDS
- Fast satisfiability check

## Build

```
$ cd simple-cpp-sat
$ make
```

## Run


### DPLL

```
$ ./bin/release/main -m dpll < expr.cnf
```

### CDCL

```
$ ./bin/release/main -m cdcl < expr.cnf
```

Input should be in DIMACS CNF format.
