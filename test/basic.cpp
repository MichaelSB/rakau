// Copyright 2018 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the mp++ library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <rakau/tree.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <numeric>
#include <random>
#include <tuple>
#include <vector>

#include "test_utils.hpp"

using namespace rakau;
using namespace rakau_test;

using fp_types = std::tuple<float, double>;

static std::mt19937 rng;

TEST_CASE("accuracy")
{
    tuple_for_each(fp_types{}, [](auto x) {
        using fp_type = decltype(x);
        constexpr auto theta = static_cast<fp_type>(.75);
        auto sizes = {10u, 100u, 1000u, 5000u};
        auto max_leaf_ns = {1u, 2u, 8u, 16u};
        auto ncrits = {1u, 16u, 128u, 256u};
        const fp_type bsize = 1;
        std::array<std::vector<fp_type>, 3> accs;
        auto median = [](auto &v) {
            if (!v.size()) {
                throw;
            }
            std::sort(v.begin(), v.end());
            if (v.size() % 2u) {
                return v[v.size() / 2u];
            } else {
                return (v[v.size() / 2u] + v[v.size() / 2u - 1u]) / 2;
            }
        };
        for (auto s : sizes) {
            auto parts = get_uniform_particles<3>(s, bsize, rng);
            for (auto max_leaf_n : max_leaf_ns) {
                for (auto ncrit : ncrits) {
                    std::vector<fp_type> x_diff, y_diff, z_diff;
                    octree<std::uint64_t, fp_type> t(
                        bsize, parts.begin(), {parts.begin() + s, parts.begin() + 2u * s, parts.begin() + 3u * s}, s,
                        max_leaf_n, ncrit);
                    t.accs_o(accs, theta);
                    for (auto i = 0u; i < s; ++i) {
                        auto eacc = t.exact_acc_o(i);
                        x_diff.emplace_back(std::abs((eacc[0] - accs[0][i]) / eacc[0]));
                        y_diff.emplace_back(std::abs((eacc[1] - accs[1][i]) / eacc[1]));
                        z_diff.emplace_back(std::abs((eacc[2] - accs[2][i]) / eacc[2]));
                    }
                    std::cout << "Results for size=" << s << ", max_leaf_n=" << max_leaf_n << ", ncrit=" << ncrit
                              << ".\n=========\n";
                    std::cout << "max_x_diff=" << *std::max_element(x_diff.begin(), x_diff.end()) << '\n';
                    std::cout << "max_y_diff=" << *std::max_element(y_diff.begin(), y_diff.end()) << '\n';
                    std::cout << "max_z_diff=" << *std::max_element(z_diff.begin(), z_diff.end()) << '\n';
                    std::cout << "average_x_diff="
                              << (std::accumulate(x_diff.begin(), x_diff.end(), fp_type(0)) / fp_type(s)) << '\n';
                    std::cout << "average_y_diff="
                              << (std::accumulate(y_diff.begin(), y_diff.end(), fp_type(0)) / fp_type(s)) << '\n';
                    std::cout << "average_z_diff="
                              << (std::accumulate(z_diff.begin(), z_diff.end(), fp_type(0)) / fp_type(s)) << '\n';
                    std::cout << "median_x_diff=" << median(x_diff) << '\n';
                    std::cout << "median_y_diff=" << median(y_diff) << '\n';
                    std::cout << "median_z_diff=" << median(z_diff) << '\n';
                    std::cout << "=========\n\n";
                }
            }
        }
    });
}
