#include "vsids.hpp"
#include <cmath>
#include <iostream>

int ceil_pow2(const int n) {
    int ret = 1;
    while (ret < n) ret *= 2;
    return ret;
}

SegmentTree::SegmentTree(const int pnum)
    : n(pnum + 1),
      n_pow2(ceil_pow2(n)),
      offset(n_pow2 - 1),
      invalid(element { -1, -1, false }),
      data(n_pow2 * 2 - 1, invalid)
{
    for (int i = 1; i <= pnum; i++) data[offset + i] = element { 0, i, true };
    for (int i = offset - 1; 0 <= i; i--) {
        auto [ c1, c2 ] = child(i);
        data[i] = comp(data[c1], data[c2]);
    }
}

SegmentTree::element SegmentTree::comp(const element &e1, const element &e2) {
    if (!e1.active && !e2.active) return invalid;
    if (e1.active && !e2.active) return e1;
    if (!e1.active && e2.active) return e2;
    return e1.score > e2.score ? e1 : e2;
}

SegmentTree::element SegmentTree::get() const {
    return data[0];
}

void SegmentTree::div(const int d) {
    for (auto &e : data) {
        if (e.score < 0) continue;
        e.score /= d;
    }
}

void SegmentTree::inc(const int i) {
    data[offset + i].score += 1;
    update(i);
}

void SegmentTree::remove(const int i) {
    data[offset + i].active = false;
    update(i);
}

void SegmentTree::restore(const int i) {
    data[offset + i].active = true;
    update(i);
}

void SegmentTree::update(int i) {
    i += offset;
    while (i) {
        i = (i - 1) / 2;
        auto [ c1, c2 ] = child(i);
        data[i] = comp(data[c1], data[c2]);
    }
}

std::pair<int, int> SegmentTree::child(const int i) const {
    return std::make_pair(2 * i + 1, 2 * i + 2);
}

VSIDS::VSIDS(const int pnum_,
             const int div_,
             const int span_)
    : pnum(pnum_),
      div(div_),
      span(span_),
      count_add(0),
      seg(pnum)
{
}

void VSIDS::vsi(Clause *c) {
    for (int i = 0; i < c->size(); i++) seg.inc(std::abs(c->get(i)));
    count_add++;
    if (count_add == span) {
        count_add = 0;
        ds();
    }
}

void VSIDS::assign(const int p) {
    seg.remove(std::abs(p));
}

void VSIDS::rollback(const int p) {
    seg.restore(std::abs(p));
}

void VSIDS::ds() {
    seg.div(div);
}

std::optional<int> VSIDS::pickup() {
    auto e = seg.get();
    //std::cout << e.score << ", " << e.idx << std::endl;
    if (!e.active) return std::nullopt;
    return e.idx;
}
