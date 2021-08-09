#include <tuple>
#include <vector>
#include "cnf.hpp"

std::tuple<CNF*, int, int> parse();
bool check_ans(const std::vector<std::vector<int>> &board, const int hw);
