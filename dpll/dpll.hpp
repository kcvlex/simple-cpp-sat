#pragma once

#include "../cnf.hpp"
#include <vector>
#include <optional>

struct DPLL {
    DPLL(CNF *cnf_, Valuation *va_);

    std::optional<Valuation*> solve();

private:
    CNF *cnf;
    Valuation *va;
    std::vector<int> sat;

    std::optional<Valuation*> solve_aux(const int p);
    std::optional<std::vector<int>> update();
};
