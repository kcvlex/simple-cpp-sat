#pragma once

#include <queue>
#include <variant>
#include <queue>
#include "../cnf.hpp"
#include "vsids.hpp"
#include "graph.hpp"

namespace cdcl {

struct clause_log {
    int fst, last;
    ClauseState state;
    Clause *ptr;
    int p0, p1;
};

struct assigned_log {
    int dl;
    int p;
    Clause *clause;
    std::vector<clause_log> clogs;

    assigned_log(const int dl_,
                 const int p_,
                 Clause *clause_);
    void add_clause_log(clause_log log, Clause *c);
};

using backjump_type = std::tuple<Clause*, int>;

struct Logger {
    std::vector<assigned_log> log_stk;

    void assign(const int dl, const int p, Clause *c);

    assigned_log pop();
    const assigned_log& top() const;
    assigned_log& top();
    bool is_empty() const;
    std::vector<assigned_log>& raw();
    void clear();
    const std::vector<assigned_log>& raw() const;
};

struct Watcher {
    std::vector<std::pair<int, int>> warr;
    
    Watcher() = delete;
    Watcher(CNF *cnf);

    void add_watch(Clause *c);
    void remove_watch(Clause *c);
    std::vector<int> get(const int idx) const;

private:
    std::vector<std::vector<int>> watches;
};

struct bounded_queue {
    bounded_queue(const int bound_);

    void push(const std::uint64_t v);
    void pop();
    void clear();
    bool is_full() const noexcept;
    std::uint64_t bound() const noexcept;
    std::uint64_t sum() const noexcept;
    std::uint64_t global_sum() const noexcept;
    std::uint64_t global_size() const noexcept;

private:
    const int bound_;
    std::uint64_t sum_, g_sum_, g_size_;
    std::queue<std::uint64_t> que;
};

struct CDCL {
    CDCL() = delete;
    CDCL(CNF *cnf_, Valuation *va_);
    ~CDCL();

    std::optional<Valuation*> solve();

private:
    CNF *cnf;
    Valuation *va;
    Logger logger;
    Watcher watcher;
    VSIDS vsids;
    bounded_queue lbd_que, conflict_que;
    ImplicationGraph igraph;

    struct global_data {
        double K;
        std::uint64_t removed;
    } g_data;

    int level;
    std::queue<Clause*> implied;

    void decision(const int p, const PValue v);
    void imply(Clause *c, const int p);
 
    void rollback();
    std::optional<Clause*> bcp(const int p);

    bool backjump(const backjump_type bj);
    std::optional<backjump_type> learnt_clause(raw_clause c);

    void update_clause(Clause *c);
    void rebuild_log(Clause *c);

    std::optional<Valuation*> restart();
    bool should_restart() const;

    bool preprocess();
    std::optional<Valuation*> solve_aux();
    void check_sat();
};

}
