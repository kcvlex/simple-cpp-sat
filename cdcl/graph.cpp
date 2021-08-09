#include "graph.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

#define ALL(V) std::begin(V), std::end(V)

ImplicationGraph::ImplicationGraph(const int pnum)
    : redges(pnum + 1),
      mark(pnum + 1),
      cache(pnum + 1),
      decision_v(pnum + 1)
{
}

void ImplicationGraph::imply(const Clause *c, const int p) {
    auto &v = redges[std::abs(p)];
    for (auto e : c->raw()) {
        if (e == p) continue;
        v.push_back(std::abs(e));
    }
}

void ImplicationGraph::decision(const int p) {
    decision_v[std::abs(p)] = 1;
}

namespace {
void dump(const std::vector<int> &v) {
    for(auto e:v)std::cout<<e<<",";
    std::cout<<std::endl;
}
}

void ImplicationGraph::reset_imply(const Clause *c, const int p) {
    auto &v = redges[std::abs(p)];
    const auto &raw = c->raw();
    for (auto ite = std::crbegin(raw); ite != std::crend(raw); ite++) {
        if (*ite == p) continue;
        assert(v.back() == std::abs(*ite));
        v.pop_back();
    }
}

void ImplicationGraph::reset_decision(const int p) {
    assert(decision_v[std::abs(p)]);
    decision_v[std::abs(p)] = 0;
}

void ImplicationGraph::local_minimize(raw_clause &r, const Valuation *va) {
    for (auto e : r) mark[std::abs(e)] = 1;
    
    raw_clause buf = r;
    r.clear();
    for (auto e : buf) {
        bool ok = [&] {
            if (decision_v[std::abs(e)]) return false;
            for (auto f : redges[std::abs(e)]) {
                if (!mark[f]) return false;
                if (va->decided(std::abs(e)) != va->decided(f)) return false;
            }
            return true;
        }();
        if (!ok) r.push_back(e);
    }
    
    for (auto e : buf) mark[std::abs(e)] = 0;
}

void ImplicationGraph::recursive_minimize(raw_clause &r) {
    std::fill(ALL(cache), 0);
    for (auto e : r) mark[std::abs(e)] = 1;
    
    auto buf = r;
    r.clear();
    for (auto e : buf) {
        if (decision_v[std::abs(e)]) {
            r.push_back(e);
            continue;
        }
        bool ok = true;
        for (auto f : redges[std::abs(e)]) {
            traverse(f);
            if (cache[f] & ok_bit) continue;
            ok = false;
            break;
        }
        if (!ok) r.push_back(e);
    }
    for (auto e : buf) mark[std::abs(e)] = 1;
}

void ImplicationGraph::traverse(const int cur) {
    if (cache[cur] & visited_bit) return;
    
    cache[cur] |= visited_bit;
    
    if (mark[cur]) {
        cache[cur] |= ok_bit;
        return;
    }
    
    if (decision_v[cur]) return;
    
    for (auto e : redges[cur]) {
        if (!(cache[e] & visited_bit)) traverse(e);
        if (!(cache[e] & ok_bit)) return;
    }
    cache[cur] |= ok_bit;
}
