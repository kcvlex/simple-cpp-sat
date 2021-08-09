#include <iostream>
#include <algorithm>
#include "util.hpp"
#include <sstream>

std::tuple<CNF*, int, int> parse() {
    int hw;
    std::string cnf_;
    int pn;
    int line;
    {
        std::string s;
        while (true) {
            std::getline(std::cin, s);
            std::stringstream ss;
            ss << s;
            char c;
            ss >> c;
            if (c == 'c') continue;
            ss >> cnf_ >> pn >> line;
            break;
        }
    }
    std::vector<raw_clause> ret;
    ret.reserve(line);
    for (int i = 0; i < line; i++) {
        std::vector<int> v;
        while (true) {
            int e;
            std::cin >> e;
            if (e == 0) break;
            v.push_back(e);
        }
        ret.emplace_back(std::move(v));
    }

    CNF *cnf = new CNF(std::move(ret));
    return std::make_tuple(cnf, pn, line);
}

bool check_ans(const std::vector<std::vector<int>> &board, const int hw) {
    auto valid = [&](const int h, const int w) {
        return 0 <= h && h < hw &&
               0 <= w && w < hw;
    };
    for (int h = 0; h < hw; h++) {
        bool ok = false;
        for (int w = 0; w < hw; w++) {
            if (board[h][w] == 0) continue;
            ok = true;
            int dv[][2] = {
                { 0, 1, },
                { 1, 0, },
                { 1, 1, },
                { 1, -1, },
            };
            for (auto dvi : dv) {
                int dh = dvi[0], dw = dvi[1];
                for (int k = -hw; k <= hw; k++) {
                    int nh = h + dh * k, nw = w + dw * k;
                    if (k == 0) continue;
                    if (!valid(nh, nw)) continue;
                    if (board[nh][nw]) return false;
                }
            }
        }
        if (!ok) return false;
    }
    return true;
}
