#include "dpll.hpp"

DPLL::DPLL(CNF *cnf_, Valuation *va_)
    : cnf(cnf_),
      va(va_),
      sat(cnf->size(), 0)
{
}

std::optional<Valuation*> DPLL::solve_aux(const int p) {
    if (p == cnf->get_pnum() + 1) return va;

    for (auto v : { PValue::TRUE, PValue::FALSE }) {
        va->assign(p, v, -1);
        auto opt = std::move(update());
        if (opt.has_value()) {
            auto res = solve_aux(p + 1);
            if (res.has_value()) return res;
            for (auto e : *opt) sat[e] = 0;
        }
    }
    va->reset(p);
    return std::nullopt;
}

std::optional<Valuation*> DPLL::solve() {
    return solve_aux(0);
}

std::optional<std::vector<int>> DPLL::update() {
    std::vector<int> res;
    for (int i = 0; i < cnf->size(); i++) {
        if (sat[i]) continue;
        const auto c = cnf->get(i);
        
        bool unknown = false;
        for (int j = 0; j < c->size(); j++) {
            const auto p = c->get(j);
            const auto v = va->get_value(p);
            if (v == PValue::BOTTOM) {
                unknown = true;
            } else if (match(p, v)) {
                res.push_back(i);
                sat[i] = 1;
                break;
            }
        }

        if (sat[i]) continue;
        if (!unknown) {
            for (auto e : res) sat[e] = 0;
            return std::nullopt;
        }
    }

    return res;
}
