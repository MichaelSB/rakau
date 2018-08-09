// Copyright 2018 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the rakau library.
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
#include <limits>
#include <numeric>
#include <random>
#include <tuple>
#include <vector>

#include "test_utils.hpp"

using namespace rakau;
using namespace rakau_test;

using fp_types = std::tuple<float, double>;

static std::mt19937 rng;

// NOTE: this is very similar to the accuracy test, just with various epsilons tested as well.
TEST_CASE("softening")
{
    tuple_for_each(fp_types{}, [](auto x) {
        using fp_type = decltype(x);
        constexpr auto theta = static_cast<fp_type>(.001), bsize = static_cast<fp_type>(1);
        auto sizes = {10u, 100u, 200u, 300u, 1000u};
        auto max_leaf_ns = {1u, 2u, 8u, 16u};
        auto ncrits = {1u, 16u, 128u, 256u};
        auto softs = {fp_type(0), fp_type(.1), fp_type(100)};
        std::array<std::vector<fp_type>, 3> accs;
        auto median = [](auto &v) {
            if (!v.size()) {
                throw;
            }
            std::sort(v.begin(), v.end());
            const auto half_size = v.size() / 2u;
            if (v.size() % 2u) {
                return v[half_size];
            }
            return (v[half_size] + v[half_size - 1u]) / fp_type(2);
        };
        fp_type tot_max_x_diff(0), tot_max_y_diff(0), tot_max_z_diff(0);
        for (auto s : sizes) {
            auto parts = get_uniform_particles<3>(s, bsize, rng);
            for (auto max_leaf_n : max_leaf_ns) {
                for (auto ncrit : ncrits) {
                    for (auto eps : softs) {
                        std::vector<fp_type> x_diff, y_diff, z_diff;
                        octree<fp_type> t(
                            bsize, {parts.begin() + s, parts.begin() + 2u * s, parts.begin() + 3u * s, parts.begin()},
                            s, max_leaf_n, ncrit);
                        t.accs_o(accs, theta, eps);
                        for (auto i = 0u; i < s; ++i) {
                            auto eacc = t.exact_acc_o(i, eps);
                            x_diff.emplace_back(std::abs((eacc[0] - accs[0][i]) / eacc[0]));
                            y_diff.emplace_back(std::abs((eacc[1] - accs[1][i]) / eacc[1]));
                            z_diff.emplace_back(std::abs((eacc[2] - accs[2][i]) / eacc[2]));
                        }
                        std::cout << "Results for size=" << s << ", max_leaf_n=" << max_leaf_n << ", ncrit=" << ncrit
                                  << ", soft=" << eps << ".\n=========\n";
                        const auto local_max_x_diff = *std::max_element(x_diff.begin(), x_diff.end()),
                                   local_max_y_diff = *std::max_element(y_diff.begin(), y_diff.end()),
                                   local_max_z_diff = *std::max_element(z_diff.begin(), z_diff.end());
                        std::cout << "max_x_diff=" << local_max_x_diff << '\n';
                        std::cout << "max_y_diff=" << local_max_y_diff << '\n';
                        std::cout << "max_z_diff=" << local_max_z_diff << '\n';
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
                        tot_max_x_diff = std::max(local_max_x_diff, tot_max_x_diff);
                        tot_max_y_diff = std::max(local_max_y_diff, tot_max_y_diff);
                        tot_max_z_diff = std::max(local_max_z_diff, tot_max_z_diff);
                        if (eps != fp_type(0)) {
                            // Put a few particles in the same spots to generate a singularity.
                            std::uniform_int_distribution<unsigned> dist(0u, s - 2u);
                            for (int i = 0; i < 10; ++i) {
                                const auto idx = dist(rng);
                                *(parts.begin() + s + idx) = *(parts.begin() + s + idx + 1u);
                                *(parts.begin() + 2u * s + idx) = *(parts.begin() + 2u * s + idx + 1u);
                                *(parts.begin() + 3u * s + idx) = *(parts.begin() + 3u * s + idx + 1u);
                            }
                            // Create a new tree.
                            t = octree<fp_type>(
                                bsize,
                                {parts.begin() + s, parts.begin() + 2u * s, parts.begin() + 3u * s, parts.begin()}, s,
                                max_leaf_n, ncrit);
                            // Compute the accelerations.
                            t.accs_u(accs, theta, eps);
                            // Verify all values are finite.
                            REQUIRE(
                                std::all_of(accs[0].begin(), accs[0].end(), [](auto c) { return std::isfinite(c); }));
                            REQUIRE(
                                std::all_of(accs[1].begin(), accs[1].end(), [](auto c) { return std::isfinite(c); }));
                            REQUIRE(
                                std::all_of(accs[2].begin(), accs[2].end(), [](auto c) { return std::isfinite(c); }));
                        }
                    }
                }
            }
        }
        std::cout << "\n\n\ntot_max_x_diff=" << tot_max_x_diff << '\n';
        std::cout << "tot_max_y_diff=" << tot_max_y_diff << '\n';
        std::cout << "tot_max_z_diff=" << tot_max_z_diff << "\n\n\n";
        if constexpr (std::is_same_v<fp_type, double> && std::numeric_limits<fp_type>::is_iec559) {
            // These numbers are, of course, totally arbitrary, based
            // on the fact that 'double' is actually double-precision,
            // and derived experimentally.
            REQUIRE(tot_max_x_diff < fp_type(1E-10));
            REQUIRE(tot_max_y_diff < fp_type(1E-10));
            REQUIRE(tot_max_z_diff < fp_type(1E-10));
        }
    });
}
