#pragma once

#include "../cnf.hpp"

struct ImplicationGraph {
    ImplicationGraph() = delete;
    ImplicationGraph(const int pnum);

    void imply(const Clause *c, const int p);
    void decision(const int p);
    void reset_imply(const Clause *c, const int p);
    void reset_decision(const int p);

    void local_minimize(raw_clause &r, const Valuation *va);
    void recursive_minimize(raw_clause &r);

private:
    constexpr static int visited_bit = 1 << 0;
    constexpr static int ok_bit = 1 << 1;

    std::vector<std::vector<int>> redges;
    std::vector<int> mark, cache, decision_v;

    void traverse(const int cur);
};
