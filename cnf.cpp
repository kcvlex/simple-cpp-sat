#include "cnf.hpp"
#include <algorithm>
#include <iostream>
#include <cassert>
#include <cmath>

#define ALL(V) std::begin(V), std::end(V)

bool match(const int p, const PValue v) {
    return 0 < p * static_cast<int>(v);
}


/* ========== Valuation ========== */

Valuation::Valuation(const int size_)
    : size(size_),
      sigma(size + 1, PValue::BOTTOM),
      cache(size + 1, PValue::FALSE),
      dl_v(size + 1, -1),
      implied(size + 1, 0)
{
}

PValue Valuation::get_value(const int p) const {
    return sigma[std::abs(p)];
}

int Valuation::decided(const int p) const {
    return dl_v[std::abs(p)];
}

void Valuation::assign(const int p, const PValue v, const int dl) {
    const int idx = std::abs(p);
    sigma[idx] = v;
    dl_v[idx] = dl;
}

void Valuation::imply(const int p, const int dl) {
    assign(p, 0 < p ? PValue::TRUE : PValue::FALSE, dl);
    implied[std::abs(p)] = true;
}

int Valuation::get_pnum() const {
    return size;
}

void Valuation::reset(const int p) {
    const int idx = std::abs(p);
    cache[idx] = sigma[idx];
    sigma[idx] = PValue::BOTTOM;
    dl_v[idx] = -1;
    implied[idx] = 0;
}

PValue Valuation::get_cache(const int p) const {
    return cache[std::abs(p)];
}

bool Valuation::was_implied(const int p) const {
    return implied[std::abs(p)];
}


/* ========== Clause ========== */

Clause::Clause(raw_clause clause_)
    : clause(std::move(clause_)),
      fst(0),
      last(clause.size()),
      state(ClauseState::Unknown),
      cnf_idx(-1)
{
    std::sort(ALL(clause));
    type = ClauseType::Original;
    LBD = 0;
}

Clause::Clause(raw_clause clause_, const Valuation *va) : Clause(std::move(clause_)) {
    type = ClauseType::Learnt;
    LBD = pseudo_LBD = calc_LBD(va);
}

const raw_clause& Clause::raw() const {
    return clause;
}

int Clause::get(const int idx) const {
    return clause[idx];
}

int Clause::size() const {
    return clause.size();
}

void Clause::update(const Valuation *va, const int dl) {
    if (state == ClauseState::SAT || state == ClauseState::UNSAT) return;

    while (fst < last) {
        auto d = va->decided(clause[fst]);
        if (d == -1 || dl < d) break;
        const auto cur = va->get_value(clause[fst]);
        assert(cur != PValue::BOTTOM);
        fst++;
        if (match(clause[fst - 1], cur)) {
            state = ClauseState::SAT;
            return;
        }
    }
    while (fst < last) {
        auto d = va->decided(clause[last - 1]);
        if (d == -1 || dl < d) break;
        const auto cur = va->get_value(clause[last - 1]);
        assert(cur != PValue::BOTTOM);
        last--;
        if (match(clause[last], cur)) {
            state = ClauseState::SAT;
            return;
        }
    }

    update_state(va);
}

void Clause::update_state(const Valuation *va) {
    if (0 < fst && match(clause[fst - 1], va->get_value(clause[fst - 1]))) {
        state = ClauseState::SAT;
    } else if (last < size() && match(clause[last], va->get_value(clause[last]))) {
        state = ClauseState::SAT;
    } else if (fst + 1 == last) {
        state = ClauseState::Unit;
    } else if (fst == last) {
        state = ClauseState::UNSAT;
    } else {
        state = ClauseState::Unknown;
    }
}

void Clause::rollback(const int fst, const int last, ClauseState state) {
    this->fst = fst;
    this->last = last;
    this->state = state;
}

std::pair<int, int> Clause::get_watch() const {
    return std::make_pair(fst, last); 
}

std::optional<int> Clause::unit() const {
    return (state == ClauseState::Unit ? std::optional<int>(clause[fst]) : std::nullopt);
}

std::uint64_t Clause::get_LBD() const {
    return LBD;
}

std::uint64_t Clause::get_pseudo_LBD() const {
    return pseudo_LBD;
}

// when unit propagatoin
void Clause::recalc_LBD(const Valuation *va) {
    const auto tmp = calc_LBD(va);
    if (tmp < LBD) {
        LBD = tmp;
        pseudo_LBD = LBD + 1;
    }
}

std::uint64_t Clause::calc_LBD(const Valuation *va) const {
    std::vector<int> dv;
    dv.reserve(clause.size());
    for (auto e : clause) {
        auto d = va->decided(e);
        if (d == -1) continue;
        dv.push_back(d);
    }
    std::sort(ALL(dv));
    return std::distance(std::begin(dv), std::unique(ALL(dv)));
}

/* ========== CNF ========== */

CNF::CNF(std::vector<raw_clause> clauses_)
    : clauses(clauses_.size()), pnum(0), original(int(clauses_.size()))
{
    for (const auto &v : clauses_) for (auto e : v) pnum = std::max(pnum, std::abs(e));
    for (int i = 0; i < int(clauses.size()); i++) {
        clauses[i] = new Clause(std::move(clauses_[i]));
        clauses[i]->cnf_idx = i;
    }
}

void CNF::free() {
    for (auto p : clauses) delete p;
    for (auto p : removed) delete p;
}

void CNF::add(Clause *c) {
    c->cnf_idx = int(clauses.size());
    clauses.push_back(c);
}

void CNF::remove(Clause *c) {
    auto ite = std::find(std::begin(clauses), std::end(clauses), c);
    assert(ite == std::end(clauses));
    clauses.erase(ite);
    removed.push_back(c);
}

int CNF::size() const {
    return clauses.size();
}

int CNF::get_pnum() const {
    return pnum;
}

int CNF::get_learnt_clause_num() const {
    return int(clauses.size()) - original;
}

Clause* CNF::get(const int idx) {
    return clauses[idx];
}

void CNF::remove_learnt_clauses() {
    std::vector<Clause*> buf;
    buf.reserve(original);
    for (int i = 0; i < original; i++) buf.push_back(clauses[i]);
    for (int i = original; i < int(clauses.size()); i++) {
        auto c = clauses[i];
        (c->get_pseudo_LBD() <= 3 ? buf : removed).push_back(c);
    }
    std::swap(clauses, buf);
}
