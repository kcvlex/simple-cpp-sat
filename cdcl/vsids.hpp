#pragma once

#include "../cnf.hpp"

struct SegmentTree {
    struct element {
        int score, idx;
        bool active;
    };

    SegmentTree(const int pnum_);

    element access(const int i) const;
    element get() const;
    void div(const int d);
    void inc(const int i);
    void remove(const int i);
    void restore(const int i);

private:
    int n;
    int n_pow2;
    int offset;
    element invalid;
    std::vector<element> data;

    void update(int i);
    element comp(const element &e1, const element &e2);
    std::pair<int, int> child(const int i) const;
};

struct VSIDS {
    VSIDS() = delete;
    VSIDS(const int pnum_,
          const int div_, 
          const int span_);

    void vsi(Clause *c);
    std::optional<int> pickup();
    void assign(const int p);
    void rollback(const int p);

private:
    int pnum;
    int div;
    int span;
    int count_add;
    SegmentTree seg;
    
    void ds();
};
