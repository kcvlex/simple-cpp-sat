#include "cdcl.hpp"
#include <algorithm>
#include <iostream>
#include <cassert>
#include <limits>

#define ALL(V) std::begin(V), std::end(V)

namespace cdcl {

void dump_watch(Clause *c) {
    auto [fst,last]=c->get_watch();
    std::cout<<fst<<","<<last<<std::endl;
}

int count_level(const raw_clause &r, const Valuation *va, const int level) {
    int ret = 0;
    for (auto e : r) {
        if (va->decided(e) != level) continue;
        ret++;
    }
    return ret;
}

void resolution(raw_clause &r1, const raw_clause &r2, const int p) {
    r1.insert(std::end(r1), ALL(r2));
    std::sort(ALL(r1));
    r1.erase(std::unique(ALL(r1)), std::end(r1));
    for (auto e : { p, -p }) {
        auto ite = std::lower_bound(ALL(r1), e);
        assert(ite != std::end(r1) && *ite == e);
        r1.erase(ite);
    }
}

bool has_var(const raw_clause &r, const int p) {
    auto ite = std::lower_bound(std::cbegin(r), std::cend(r), p);
    return ite != std::cend(r) && *ite == p;
}

int find_max_level(const raw_clause &r, const Valuation *va, const int exclude) {
    int ret = -1;
    for (auto e : r) {
        const int l = va->decided(e);
        if (l == exclude) continue;
        ret = std::max(ret, l);
    }
    return ret;
}

bool is_first_uip(const raw_clause &r, const Valuation *va, const int level) {
    return count_level(r, va, level) == 1;
}

void dump(const raw_clause &r) {
    for (auto e : r) std::cout << e << ' ';
    std::cout << std::endl;
}

void dump(Clause *c) {
    dump(c->raw());
}


/* ========== Logger ========== */

assigned_log::assigned_log(const int dl_,
                           const int p_,
                           Clause *clause_)
    : dl(dl_), p(p_), clause(clause_)
{
}

void assigned_log::add_clause_log(clause_log log, Clause *c) {
    do {
        auto [ fst, last ] = c->get_watch();
        if (log.fst != fst) break;
        if (log.last != last) break;
        if (log.state != c->state) break;
        return;
    } while (false);
    clogs.push_back(log);
}

void Logger::assign(const int dl, const int p, Clause *c) {
    log_stk.emplace_back(dl, p, c);
}

assigned_log Logger::pop() {
    auto res = std::move(log_stk.back());
    log_stk.pop_back();
    return res;
}

const assigned_log& Logger::top() const {
    return log_stk.back();
}

assigned_log& Logger::top() {
    return log_stk.back();
}

bool Logger::is_empty() const {
    return log_stk.size() == 0;
}

const std::vector<assigned_log>& Logger::raw() const {
    return log_stk;
}

std::vector<assigned_log>& Logger::raw() {
    return log_stk;
}


/* ========== Watcher ========== */

Watcher::Watcher(CNF *cnf)
    : warr(cnf->size()),
      watches(cnf->get_pnum() + 1)
{
    for (int i = 0; i < cnf->size(); i++) {
        auto c = cnf->get(i);
        auto [ fst, last ] = c->get_watch();
        const auto p0 = c->get(fst), p1 = c->get(last - 1);
        warr[i] = std::make_pair(p0, p1);
        add_watch(c);
    }
}

void Watcher::add_watch(Clause *c) {
    if (c->state != ClauseState::Unknown) return;
    auto [ fst, last ] = c->get_watch();
    for (const auto idx : { fst, last - 1 }) {
        const int p = std::abs(c->get(idx));
        auto &v = watches[p];
        const auto cnf_idx = c->cnf_idx;
        auto ite = std::upper_bound(ALL(v), cnf_idx);
        if (ite != std::end(v) && *ite == cnf_idx) continue;
        v.insert(ite, cnf_idx);
    }
}

void Watcher::remove_watch(Clause *c) {
    auto [ fst, last ] = c->get_watch();
    for (const auto idx : { fst, last - 1 }) {
        const int p = std::abs(c->get(idx));
        auto &v = watches[p];
        const auto cnf_idx = c->cnf_idx;
        auto ite = std::lower_bound(ALL(v), cnf_idx);
        if (ite == std::end(v)) continue;
        if (*ite != cnf_idx) continue;
        v.erase(ite);
    }
}

std::vector<int> Watcher::get(const int idx) const {
    return watches[std::abs(idx)];
}


/* ========== bounded_queue ========== */

bounded_queue::bounded_queue(const int bound_)
    : bound_(bound_),
      sum_(0),
      g_sum_(0),
      g_size_(0)
{
}

void bounded_queue::push(const std::uint64_t v) {
    que.push(v);
    sum_ += v;
    g_sum_ += v;
    g_size_++;
    if (bound_ < int(que.size())) pop();
}

void bounded_queue::pop() {
    sum_ -= que.front();
    que.pop();
}

void bounded_queue::clear() {
    while (que.size()) que.pop();
}

bool bounded_queue::is_full() const noexcept {
    return int(que.size()) == bound_;
}

std::uint64_t bounded_queue::sum() const noexcept {
    return sum_;
}

std::uint64_t bounded_queue::bound() const noexcept {
    return bound_;
}

std::uint64_t bounded_queue::global_sum() const noexcept {
    return g_sum_;
}

std::uint64_t bounded_queue::global_size() const noexcept {
    return g_size_;
}


/* ========== CDCL ========== */

CDCL::CDCL(CNF *cnf_, Valuation *va_)
    : cnf(cnf_),
      va(va_),
      watcher(cnf),
      vsids(cnf->get_pnum(), 100, 100),
      lbd_que(50),
      conflict_que(50),
      igraph(cnf->get_pnum()),
      g_data(global_data { 0.8, 0 }),
      level(0)
{
}

void CDCL::decision(const int p, const PValue v) {
    level++;
    va->assign(p, v, level);
    vsids.assign(p);
    igraph.decision(p);
    logger.assign(level, p, nullptr);
}

void CDCL::imply(Clause *c, const int p) {
    assert(c->state == ClauseState::Unit);
    va->imply(p, level);
    vsids.assign(p);
    igraph.imply(c, p);
    logger.assign(level, p, c);
    update_clause(c);
    assert(c->state == ClauseState::SAT);
}

void CDCL::rollback() {
    assert(!logger.is_empty());
    auto log = logger.pop();
    if (log.dl == 0) return;
    assert(level == log.dl);
    va->reset(log.p);
    vsids.rollback(log.p);
    if (log.clause == nullptr) {
        level--;
        igraph.reset_decision(log.p);
    } else {
        igraph.reset_imply(log.clause, log.p);
    }
    std::reverse(ALL(log.clogs));
    for (auto cl : log.clogs) {
        auto c = cl.ptr;
        watcher.remove_watch(c);
        c->rollback(cl.fst, cl.last, cl.state);
        watcher.add_watch(c);
        watcher.warr[c->cnf_idx] = std::make_pair(cl.p0, cl.p1);
    }
}

std::optional<backjump_type> CDCL::learnt_clause(raw_clause c) {
    // std::cout<<count_level(c,va,level)<<std::endl;
    if (!is_first_uip(c, va, level)) {
        for (auto ite = std::crbegin(logger.raw()); ite != std::crend(logger.raw()); ite++) {
            const auto unit = ite->clause;
            const auto p = ite->p;
            assert(unit != nullptr);
            if (!has_var(c, -p)) continue;
            resolution(c, unit->raw(), p);
            if (is_first_uip(c, va, level)) break;
            if (c.size() == 0) return std::nullopt;
        }
    }

    assert(is_first_uip(c, va, level));
    // igraph.recursive_minimize(c);
    igraph.local_minimize(c, va);
    int l = find_max_level(c, va, level);
    Clause *clause = new Clause(std::move(c), va);
    return std::make_tuple(clause, l);
}

bool CDCL::backjump(const backjump_type bj) {
    auto [ clause, l ] = bj;
    if (l <= 0) return false;
    while (true) {
        rollback();
        if (level == l) break;
    }
    return true;
}

void CDCL::update_clause(Clause *c) {
    auto [ fst, last ] = c->get_watch();
    const int idx = c->cnf_idx;
    auto [ p0, p1 ] = watcher.warr[idx];
    clause_log log { fst, last, c->state, c, p0, p1 };
    watcher.remove_watch(c);
    c->update(va, level);
    watcher.add_watch(c);
    logger.top().add_clause_log(log, c);
    if (c->state == ClauseState::Unknown) {
        auto [ fst, last ] = c->get_watch();
        const auto &v = c->raw();
        watcher.warr[idx] = std::make_pair(v[fst], v[last - 1]);
    }
}

std::optional<Clause*> CDCL::bcp(const int p) {
    auto set = watcher.get(std::abs(p));
    for (auto idx : set) {
        auto [ p0, p1 ] = watcher.warr[idx];
        auto c = cnf->get(idx);
        if (match(p0, va->get_value(p0)) ||
            match(p1, va->get_value(p1)))
        {
            auto [ fst, last ] = c->get_watch();
            clause_log clog { fst, last, c->state, c, p0, p1 };
            c->state = ClauseState::SAT;
            logger.top().add_clause_log(clog, c);
            watcher.remove_watch(c);
            continue;
        }
        update_clause(c);
        if (c->state == ClauseState::UNSAT) {
            return c;
        } else if (c->state == ClauseState::Unit) {
            implied.push(c);
        }
    }
    return std::nullopt;
}

void CDCL::rebuild_log(Clause *c) {
    const auto &v = c->raw();
    const auto idx = c->cnf_idx;
    Valuation tmp(va->get_pnum());
    for (auto ite = logger.raw().begin(); ite != logger.raw().end(); ite++) {
        tmp.assign(ite->p, va->get_value(ite->p), ite->dl);
        auto [ fst, last ] = c->get_watch();
        clause_log clog { fst, last, c->state, c, v[fst], v[last - 1]  };
        c->update(&tmp, ite->dl);
        ite->add_clause_log(clog, c);
    }
    watcher.warr.emplace_back(0, 0);
}

bool CDCL::preprocess() {
    level = 0;
    // resolve trivial clause
    for (int i = 0; i < cnf->size(); i++) {
        auto c = cnf->get(i);
        if (c->size() != 1) continue;
        const int p = c->get(0);
        const auto v = (p < 0 ? PValue::FALSE : PValue::TRUE);
        if (int(va->get_value(p)) * int(v) == -1) return false;
        va->assign(p, v, level);
        vsids.assign(p);
    }
    logger.assign(level, 0, nullptr);
    return true;
}

std::optional<Valuation*> CDCL::solve_aux() {
    while (true) {
        auto pick = vsids.pickup();

        if (!pick.has_value()) {
            for (int i = 0; i < cnf->size(); i++) {
                cnf->get(i)->update(va, level);
                assert(cnf->get(i)->state == ClauseState::SAT);
            }
            for (int i = 1; i <= va->get_pnum(); i++) assert(int(va->get_value(i)));
            return va;
        }

        while (implied.size() || pick.has_value()) {
            auto opt = [&] -> std::optional<Clause*> {
                if (pick.has_value()) {
                    auto p = *pick;
                    pick = std::nullopt;
                    decision(p, va->get_cache(p));
                    return bcp(p);
                }
                auto c = implied.front();
                implied.pop();
                c->recalc_LBD(va);
                update_clause(c);
                if (c->state == ClauseState::UNSAT) return c;
                if (c->state == ClauseState::SAT) return std::nullopt;
                auto p = *(c->unit());
                imply(c, p);
                return bcp(p);
            }();
            if (!opt.has_value()) continue;
            
            while (implied.size()) implied.pop();

            const auto conflict_level = level;
            auto bj = learnt_clause((*opt)->raw());
            if (!bj.has_value()) return std::nullopt;
            auto [ clause, l ] = *bj;
            if (l == 0) return std::nullopt;

            if (!backjump(*bj)) return std::nullopt;

            cnf->add(clause);
            vsids.vsi(clause);
            conflict_que.push(conflict_level);
            lbd_que.push(clause->get_LBD());
            
            // restart or not
            if (should_restart()) goto restart_l;

            rebuild_log(clause);
            implied.push(clause);
            assert(clause->state == ClauseState::Unit);
            assert(implied.size() == 1);
            
            // remove or not
            /*
            if (20000 + 500 * g_data.removed < cnf->get_learnt_clause_num()) {
                cnf->remove_learnt_clauses();
                g_data.removed++;
                // FIXME : index
            }
            */
        }
    }

restart_l:
    return restart();
}

bool CDCL::should_restart() const {
    if (!lbd_que.is_full()) return false;

    {
        auto v1 = double(lbd_que.sum());
        v1 /= lbd_que.bound();
        v1 *= g_data.K;
        auto v2 = double(lbd_que.global_sum()) / conflict_que.global_size();
        if (v1 > v2) return true;
    }

    {
        auto v1 = double(conflict_que.sum());
        v1 /= conflict_que.bound();
        auto v2 = double(conflict_que.global_sum()) / conflict_que.global_size();
        return v1 > v2;
    }
}

std::optional<Valuation*> CDCL::restart() {
    while (!logger.is_empty()) rollback();
    lbd_que.clear();
    conflict_que.clear();
    return solve_aux();
}

std::optional<Valuation*> CDCL::solve() {
    if (!preprocess()) return std::nullopt;
    return solve_aux();
}

} // cdcl
