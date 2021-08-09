#include <iostream>
#include <cassert>
#include <unistd.h>
#include "util.hpp"
#include "dpll/dpll.hpp"
#include "cdcl/cdcl.hpp"

auto solve(CNF *cnf, Valuation *va, bool dpll) {
    if (dpll) {
        return DPLL(cnf, va).solve();
    } else {
        return cdcl::CDCL(cnf, va).solve();
    }
}

int main(int argc, char *argv[]) {
    bool queen = false;
    std::optional<bool> dpll = std::nullopt;
    {
        int opt;
        while ((opt = getopt(argc, argv, "qm:")) != -1) {
            switch (opt) {
                case 'q':
                    queen = true;
                    break;
                case 'm': 
                    {
                        std::string s = optarg;
                        if (s == "dpll") dpll = true;
                        else if (s == "cdcl") dpll = false;
                        else assert(false);
                        break;
                    }
                default:
                    assert(false);
            }
        }
    }

    auto [ cnf, pn, line ] = parse();
    Valuation va_(cnf->get_pnum());
    auto res = solve(cnf, &va_, dpll.value());
    if (res.has_value()) {
        auto va = *res;
        std::cout << "SAT\n";
        if (queen) {
            int hw = [&] {
                for (int i = 1;; i++) if (i * i == pn) return i;
            }();
            std::vector<std::vector<int>> ans(hw, std::vector<int>(hw));
            for (int i = 0; i < hw; i++) {
                for (int j = 0; j < hw; j++) {
                    auto v = int(va->get_value(i * hw + j + 1));
                    ans[i][j] = (v < 0 ? 0 : 1);
                    std::cout << v << ' ';
                }
                std::cout << std::endl;
            }
            // std::cout << std::boolalpha << check_ans(ans, hw) << std::endl;
        } else {
            for (int i = 1; i <= pn; i++) {
                if (va->get_value(i) == PValue::FALSE) std::cout << -i;
                else if (va->get_value(i) == PValue::TRUE) std::cout << i;
                else assert(false);
                std::cout << ' ';
            }
            std::cout << std::endl;
        }
        // std::cout << cnf->size() << std::endl;
    } else {
        std::cout << "UNSAT\n";
    }

    cnf->free();
    delete cnf;
    return 0;
}
