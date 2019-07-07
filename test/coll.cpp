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
#include <initializer_list>
#include <numeric>
#include <random>
#include <tuple>
// #include <type_traits>
// #include <vector>

#include <boost/iterator/permutation_iterator.hpp>
// #include <boost/tuple/tuple.hpp>

// #include <rakau/config.hpp>

#include "test_utils.hpp"

using namespace rakau;
using namespace rakau::kwargs;
using namespace rakau_test;

std::mt19937 rng;

TEST_CASE("coll_leaves_permutation")
{
    // Empty tree.
    octree<double> t;
    auto perm = detail::coll_leaves_permutation(t.nodes());
    REQUIRE(perm.empty());

    // Fill it in with some particles.
    const auto bsize = 1.;
    const auto s = 10000u;
    auto parts = get_uniform_particles<3>(s, bsize, rng);
    t = octree<double>{x_coords = parts.begin() + s,
                       y_coords = parts.begin() + 2u * s,
                       z_coords = parts.begin() + 3u * s,
                       masses = parts.begin(),
                       nparts = s,
                       box_size = bsize};
    perm = detail::coll_leaves_permutation(t.nodes());
    auto p_begin = boost::make_permutation_iterator(t.nodes().begin(), perm.begin());
    auto p_end = boost::make_permutation_iterator(t.nodes().end(), perm.end());
    // Check the output is sorted.
    auto node_sorter = [](const auto &n1, const auto &n2) { return detail::node_compare<3>(n1.code, n2.code); };
    REQUIRE(std::is_sorted(p_begin, p_end, node_sorter));
    // Check the output contains all particles.
    REQUIRE(std::accumulate(p_begin, p_end, 0u,
                            [](auto cur, const auto &n) { return cur + static_cast<unsigned>(n.end - n.begin); })
            == s);
}

TEST_CASE("coll_get_aabb_vertices_2d")
{
    using v2d = std::array<double, 2>;

    // Empty aabb.
    v2d p_pos = {.5, .5};
    v2d aabb_sizes = {0., 0.};

    auto ret = detail::coll_get_aabb_vertices(p_pos, aabb_sizes, -10., 10.);

    REQUIRE(std::all_of(ret.begin(), ret.end(), [p_pos](const auto &v) { return v == p_pos; }));

    aabb_sizes = {.25, .25};
    ret = detail::coll_get_aabb_vertices(p_pos, aabb_sizes, -10., 10.);

    REQUIRE(std::find(ret.begin(), ret.end(), v2d{.375, .375}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{.625, .625}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{.375, .625}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{.625, .375}) != ret.end());

    aabb_sizes = {.25, .25 / 2};
    ret = detail::coll_get_aabb_vertices(p_pos, aabb_sizes, -10., 10.);

    REQUIRE(std::find(ret.begin(), ret.end(), v2d{.375, .4375}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{.625, .5625}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{.375, .5625}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{.625, .4375}) != ret.end());

    // Try a negative coord.
    p_pos = {-.5, .5};
    ret = detail::coll_get_aabb_vertices(p_pos, aabb_sizes, -10., 10.);

    REQUIRE(std::find(ret.begin(), ret.end(), v2d{-.625, .4375}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{-.375, .5625}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{-.625, .5625}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{-.375, .4375}) != ret.end());

    // Clamping.
    p_pos = {9., 9.};
    aabb_sizes = {4., 4.};
    ret = detail::coll_get_aabb_vertices(p_pos, aabb_sizes, -10., 10.);

    REQUIRE(std::find(ret.begin(), ret.end(), v2d{7., 7.}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{7., 10.}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{10., 7.}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{10., 10.}) != ret.end());

    p_pos = {9., -9.};
    aabb_sizes = {4., 4.};
    ret = detail::coll_get_aabb_vertices(p_pos, aabb_sizes, -10., 10.);

    REQUIRE(std::find(ret.begin(), ret.end(), v2d{7., -7.}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{7., -10.}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{10., -7.}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{10., -10.}) != ret.end());

    p_pos = {-9., 9.};
    aabb_sizes = {4., 4.};
    ret = detail::coll_get_aabb_vertices(p_pos, aabb_sizes, -10., 10.);

    REQUIRE(std::find(ret.begin(), ret.end(), v2d{-7., 7.}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{-7., 10.}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{-10., 7.}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{-10., 10.}) != ret.end());

    p_pos = {-9., -9.};
    aabb_sizes = {4., 4.};
    ret = detail::coll_get_aabb_vertices(p_pos, aabb_sizes, -10., 10.);

    REQUIRE(std::find(ret.begin(), ret.end(), v2d{-7., -7.}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{-7., -10.}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{-10., -7.}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{-10., -10.}) != ret.end());

    // Clamp in all directions.
    p_pos = {0., 0.};
    aabb_sizes = {40., 40.};
    ret = detail::coll_get_aabb_vertices(p_pos, aabb_sizes, -10., 10.);

    REQUIRE(std::find(ret.begin(), ret.end(), v2d{-10., 10.}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{10., -10.}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{-10., -10.}) != ret.end());
    REQUIRE(std::find(ret.begin(), ret.end(), v2d{10., 10.}) != ret.end());
}