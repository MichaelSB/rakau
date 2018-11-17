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
#include <limits>
#include <random>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

#include "test_utils.hpp"

using namespace rakau;
using namespace rakau::kwargs;
using namespace rakau_test;

using fp_types = std::tuple<float, double>;

static std::mt19937 rng;

TEST_CASE("ctors")
{
    tuple_for_each(fp_types{}, [](auto x) {
        using Catch::Matchers::Contains;
        using fp_type = decltype(x);
        constexpr fp_type bsize = 10;
        constexpr unsigned N = 100;
        using tree_t = octree<fp_type>;
        // Default ctor.
        tree_t t0;
        REQUIRE(t0.box_size() == fp_type(0));
        REQUIRE(!t0.box_size_deduced());
        REQUIRE(t0.ncrit() == default_ncrit);
        REQUIRE(t0.max_leaf_n() == default_max_leaf_n);
        REQUIRE(t0.perm().empty());
        REQUIRE(t0.last_perm().empty());
        REQUIRE(t0.inv_perm().empty());
        // Generate some particles in 3D.
        auto parts = get_uniform_particles<3>(N, bsize, rng);
        // Ctor from array of iterators, box size given, default ncrit/max_leaf_n.
        tree_t t1{std::array{parts.begin() + N, parts.begin() + 2u * N, parts.begin() + 3u * N, parts.begin()}, N,
                  box_size = bsize};
        REQUIRE(t1.box_size() == bsize);
        REQUIRE(!t1.box_size_deduced());
        REQUIRE(t1.max_leaf_n() == default_max_leaf_n);
        REQUIRE(t1.ncrit() == default_ncrit);
        REQUIRE(t1.perm() == t1.last_perm());
        REQUIRE(t1.inv_perm().size() == N);
        // Non-default ncrit/max_leaf_n.
        tree_t t2{std::array{parts.begin() + N, parts.begin() + 2u * N, parts.begin() + 3u * N, parts.begin()}, N,
                  max_leaf_n = 4, ncrit = 5, box_size = bsize};
        REQUIRE(t2.box_size() == bsize);
        REQUIRE(!t2.box_size_deduced());
        REQUIRE(t2.max_leaf_n() == 4u);
        REQUIRE(t2.ncrit() == 5u);
        REQUIRE(t2.perm() == t2.last_perm());
        REQUIRE(t2.inv_perm().size() == N);
        // Same, with ctors from init lists.
        tree_t t1a{
            {parts.begin() + N, parts.begin() + 2u * N, parts.begin() + 3u * N, parts.begin()}, N, box_size = bsize};
        REQUIRE(t1a.box_size() == bsize);
        REQUIRE(!t1a.box_size_deduced());
        REQUIRE(t1a.max_leaf_n() == default_max_leaf_n);
        REQUIRE(t1a.ncrit() == default_ncrit);
        REQUIRE(t1a.perm() == t1a.last_perm());
        REQUIRE(t1a.inv_perm().size() == N);
        tree_t t2a{{parts.begin() + N, parts.begin() + 2u * N, parts.begin() + 3u * N, parts.begin()},
                   N,
                   box_size = bsize,
                   max_leaf_n = 4,
                   ncrit = 5};
        REQUIRE(t2a.box_size() == bsize);
        REQUIRE(!t2a.box_size_deduced());
        REQUIRE(t2a.max_leaf_n() == 4u);
        REQUIRE(t2a.ncrit() == 5u);
        REQUIRE(t2a.perm() == t2a.last_perm());
        REQUIRE(t2a.inv_perm().size() == N);
        // Ctors from vectors.
        std::array<std::vector<fp_type>, 4> arr_vec;
        for (auto &vec : arr_vec) {
            vec.resize(N);
            std::uniform_real_distribution<fp_type> urd(-fp_type(1), fp_type(1));
            std::generate(vec.begin(), vec.end(), [&urd]() { return urd(rng); });
        }
        tree_t tvec1{arr_vec};
        REQUIRE(tvec1.nparts() == N);
        REQUIRE(std::equal(arr_vec[0].begin(), arr_vec[0].end(), tvec1.p_its_o()[0]));
        REQUIRE(std::equal(arr_vec[1].begin(), arr_vec[1].end(), tvec1.p_its_o()[1]));
        REQUIRE(std::equal(arr_vec[2].begin(), arr_vec[2].end(), tvec1.p_its_o()[2]));
        REQUIRE(std::equal(arr_vec[3].begin(), arr_vec[3].end(), tvec1.p_its_o()[3]));
        tree_t tvec2{arr_vec, box_size = 100};
        REQUIRE(tvec2.nparts() == N);
        REQUIRE(tvec2.box_size() == fp_type(100));
        REQUIRE(std::equal(arr_vec[0].begin(), arr_vec[0].end(), tvec2.p_its_o()[0]));
        REQUIRE(std::equal(arr_vec[1].begin(), arr_vec[1].end(), tvec2.p_its_o()[1]));
        REQUIRE(std::equal(arr_vec[2].begin(), arr_vec[2].end(), tvec2.p_its_o()[2]));
        REQUIRE(std::equal(arr_vec[3].begin(), arr_vec[3].end(), tvec2.p_its_o()[3]));
        arr_vec[2].clear();
        REQUIRE_THROWS_WITH((tree_t{arr_vec, box_size = 3, max_leaf_n = 4, ncrit = 5}),
                            Contains("Inconsistent sizes detected in the construction of a tree from an array "
                                     "of vectors: the first vector has a size of "
                                     + std::to_string(N)
                                     + ", while the vector at index 2 has a size of 0 (all the vectors in the input "
                                       "array must have the same size)"));
        // Ctors with deduced box size.
        fp_type xcoords[] = {-10, 1, 2, 10}, ycoords[] = {-10, 1, 2, 10}, zcoords[] = {-10, 1, 2, 10},
                masses[] = {1, 1, 1, 1};
        tree_t t3{std::array{xcoords, ycoords, zcoords, masses}, 4};
        REQUIRE(t3.box_size() == fp_type(21));
        REQUIRE(t3.box_size_deduced());
        REQUIRE(t3.max_leaf_n() == default_max_leaf_n);
        REQUIRE(t3.ncrit() == default_ncrit);
        REQUIRE(t3.perm() == t3.last_perm());
        REQUIRE(t3.inv_perm().size() == 4u);
        tree_t t4{std::array{xcoords, ycoords, zcoords, masses}, 4, max_leaf_n = 4, ncrit = 5};
        REQUIRE(t4.box_size() == fp_type(21));
        REQUIRE(t4.box_size_deduced());
        REQUIRE(t4.max_leaf_n() == 4u);
        REQUIRE(t4.ncrit() == 5u);
        REQUIRE(t4.perm() == t4.last_perm());
        REQUIRE(t4.inv_perm().size() == 4u);
        tree_t t3a{{xcoords, ycoords, zcoords, masses}, 4};
        REQUIRE(t3a.box_size() == fp_type(21));
        REQUIRE(t3a.box_size_deduced());
        REQUIRE(t3a.max_leaf_n() == default_max_leaf_n);
        REQUIRE(t3a.ncrit() == default_ncrit);
        REQUIRE(t3a.perm() == t3a.last_perm());
        REQUIRE(t3a.inv_perm().size() == 4u);
        tree_t t4a{{xcoords, ycoords, zcoords, masses}, 4, max_leaf_n = 4, ncrit = 5};
        REQUIRE(t4a.box_size() == fp_type(21));
        REQUIRE(t4a.box_size_deduced());
        REQUIRE(t4a.max_leaf_n() == 4u);
        REQUIRE(t4a.ncrit() == 5u);
        REQUIRE(t4a.perm() == t4a.last_perm());
        REQUIRE(t4a.inv_perm().size() == 4u);
        // Provide explicit box size of zero, which generates an infinity
        // when trying to discretise.
        REQUIRE_THROWS_WITH((tree_t{{xcoords, ycoords, zcoords, masses}, 4, box_size = 0., max_leaf_n = 4, ncrit = 5}),
                            Contains("While trying to discretise the input coordinate"));
        // Box size too small.
        REQUIRE_THROWS_WITH((tree_t{{xcoords, ycoords, zcoords, masses}, 4, box_size = 3, max_leaf_n = 4, ncrit = 5}),
                            Contains("produced the floating-point value"));
        // Box size negative.
        REQUIRE_THROWS_WITH((tree_t{{xcoords, ycoords, zcoords, masses}, 4, box_size = -3, max_leaf_n = 4, ncrit = 5}),
                            Contains("The box size must be a finite non-negative value, but it is"));
        if (std::numeric_limits<fp_type>::has_infinity) {
            // Box size not finite.
            REQUIRE_THROWS_WITH((tree_t{{xcoords, ycoords, zcoords, masses},
                                        4,
                                        box_size = std::numeric_limits<fp_type>::infinity(),
                                        max_leaf_n = 4,
                                        ncrit = 5}),
                                Contains("The box size must be a finite non-negative value, but it is"));
        }
        // Wrong max_leaf_n/ncrit.
        REQUIRE_THROWS_WITH((tree_t{{xcoords, ycoords, zcoords, masses}, 4, max_leaf_n = 0, ncrit = 5}),
                            Contains("The maximum number of particles per leaf must be nonzero"));
        REQUIRE_THROWS_WITH((tree_t{{xcoords, ycoords, zcoords, masses}, 4, max_leaf_n = 4, ncrit = 0}),
                            Contains("The critical number of particles for the vectorised computation of the"));
        // Wrong number of elements in init list.
        REQUIRE_THROWS_WITH((tree_t{{xcoords, ycoords}, 4, max_leaf_n = 4, ncrit = 5}),
                            Contains("An initializer list containing 2 iterators was used in the construction of a "
                                     "3-dimensional tree, but a list with 4 iterators is required instead (3 iterators "
                                     "for the coordinates, 1 for the masses)"));
        // Copy ctor.
        tree_t t4a_copy(t4a);
        REQUIRE(t4a_copy.box_size() == fp_type(21));
        REQUIRE(t4a_copy.box_size_deduced());
        REQUIRE(t4a_copy.max_leaf_n() == 4u);
        REQUIRE(t4a_copy.ncrit() == 5u);
        REQUIRE(t4a_copy.perm() == t4a.perm());
        REQUIRE(t4a_copy.last_perm() == t4a.last_perm());
        REQUIRE(t4a_copy.inv_perm() == t4a.inv_perm());
        // Move ctor.
        tree_t t4a_move(std::move(t4a_copy));
        REQUIRE(t4a_move.box_size() == fp_type(21));
        REQUIRE(t4a_move.box_size_deduced());
        REQUIRE(t4a_move.max_leaf_n() == 4u);
        REQUIRE(t4a_move.ncrit() == 5u);
        REQUIRE(t4a_copy.box_size() == fp_type(0));
        REQUIRE(!t4a_copy.box_size_deduced());
        REQUIRE(t4a_copy.max_leaf_n() == default_max_leaf_n);
        REQUIRE(t4a_copy.ncrit() == default_ncrit);
        REQUIRE(t4a_copy.perm().empty());
        REQUIRE(t4a_copy.last_perm().empty());
        REQUIRE(t4a_copy.inv_perm().empty());
        // Copy assignment.
        t4a = t3a;
        REQUIRE(t4a.box_size() == fp_type(21));
        REQUIRE(t4a.box_size_deduced());
        REQUIRE(t4a.max_leaf_n() == default_max_leaf_n);
        REQUIRE(t4a.ncrit() == default_ncrit);
        REQUIRE(t4a.perm() == t3a.perm());
        REQUIRE(t4a.last_perm() == t3a.last_perm());
        REQUIRE(t4a.inv_perm() == t3a.inv_perm());
        // Move assignment.
        t4a = std::move(t3);
        REQUIRE(t4a.box_size() == fp_type(21));
        REQUIRE(t4a.box_size_deduced());
        REQUIRE(t4a.max_leaf_n() == default_max_leaf_n);
        REQUIRE(t4a.ncrit() == default_ncrit);
        REQUIRE(t3.box_size() == fp_type(0));
        REQUIRE(!t3.box_size_deduced());
        REQUIRE(t3.max_leaf_n() == default_max_leaf_n);
        REQUIRE(t3.ncrit() == default_ncrit);
        REQUIRE(t3.perm().empty());
        REQUIRE(t3.last_perm().empty());
        REQUIRE(t3.inv_perm().empty());
        // Self assignments.
        t4a = *&t4a;
        REQUIRE(t4a.box_size() == fp_type(21));
        REQUIRE(t4a.box_size_deduced());
        REQUIRE(t4a.max_leaf_n() == default_max_leaf_n);
        REQUIRE(t4a.ncrit() == default_ncrit);
        t4a = std::move(t4a);
        REQUIRE(t4a.box_size() == fp_type(21));
        REQUIRE(t4a.box_size_deduced());
        REQUIRE(t4a.max_leaf_n() == default_max_leaf_n);
        REQUIRE(t4a.ncrit() == default_ncrit);
    });
}
