/*
 * NPG-explorer, Nucleotide PanGenome explorer
 * Copyright (C) 2012-2016 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#include <boost/foreach.hpp>

#include "ExactStemBSA.hpp"
#include "BlockSet.hpp"
#include "bsa_algo.hpp"
#include "block_hash.hpp"

namespace npge {

ExactStemBSA::ExactStemBSA() {
    add_opt("bsa-stem-improve", "Move fragments and remove pure gaps",
            true);
    add_gopt("bsa-min-length", "Min length of accepted block",
             "MIN_LENGTH");
    declare_bs("target", "Target blockset");
}

void ExactStemBSA::run_impl() const {
    int genomes = genomes_number(*block_set());
    bool improve = opt_value("bsa-stem-improve").as<bool>();
    int min_length = opt_value("bsa-min-length").as<int>();
    BOOST_FOREACH (std::string bsa_name, block_set()->bsas()) {
        BSA& bsa = block_set()->bsa(bsa_name);
        bsa_filter_exact_stem(bsa, genomes);
        bsa_filter_long(bsa, min_length);
        if (improve) {
            bsa_move_fragments(bsa);
            bsa_remove_pure_gaps(bsa);
            bsa_unwind(bsa);
            bsa_move_fragments(bsa);
            bsa_move_columns(bsa);
            bsa_move_fragments(bsa);
            bsa_move_columns(bsa);
            bsa_remove_pure_gaps(bsa);
        }
    }
}

const char* ExactStemBSA::name_impl() const {
    return "Replace all non-stem blocks with gaps "
           "in blockset alignment";
}

}

