#pragma once

#include <vector>
#include <stack>
#include <optional>
#include <cstdint>

template <typename T>
using vstack = std::stack<T, std::vector<T>>;

// enum class Var : std::int32_t { };
// enum class Literal : std::uint32_t { };

using raw_clause = std::vector<int>;

enum class PValue {
    TRUE = 1,
    FALSE = -1,
    BOTTOM = 0,
};

enum class ClauseState {
    Unknown,
    SAT,
    UNSAT,
    Unit,
};

enum class ClauseType {
    Original,
    Learnt,
    Removed,
};

bool match(const int p, const PValue v);

struct Valuation {
    Valuation(const int size_);
    PValue get_value(const int p) const;
    int decided(const int p) const;
    void assign(const int p, const PValue v, const int dl);
    void imply(const int p, const int dl);
    void reset(const int p);
    int get_pnum() const;
    PValue get_cache(const int p) const;
    bool was_implied(const int p) const;

private:
    int size;
    std::vector<PValue> sigma, cache;
    std::vector<int> dl_v;
    std::vector<int> implied;
};

struct Clause {
    ClauseState state;
    ClauseType type;
    int cnf_idx;

    Clause(raw_clause clause_);
    Clause(raw_clause clause_, const Valuation *va);  // learnt clause

    const raw_clause& raw() const;
    int get(const int idx) const;
    int size() const;
   
    void update(const Valuation *va, const int dl);
    std::pair<int, int> get_watch() const;
    std::optional<int> unit() const;
    void rollback(const int fst, const int last, ClauseState state);

    std::uint64_t get_LBD() const;
    std::uint64_t get_pseudo_LBD() const;
    void recalc_LBD(const Valuation *va);

private:
    raw_clause clause;
    int fst, last;
    std::uint64_t LBD, pseudo_LBD;

    void update_state(const Valuation *va);
    std::uint64_t calc_LBD(const Valuation *va) const;
};

struct CNF {
    CNF(std::vector<raw_clause> clauses_);

    void add(Clause *c);
    void remove(Clause *c);
    int size() const;
    int get_pnum() const;
    Clause* get(const int idx);
    int get_learnt_clause_num() const;
    void remove_learnt_clauses();
    void free();

private:
    int pnum;
    int original;
    std::vector<Clause*> clauses;
    std::vector<Clause*> removed;
};
