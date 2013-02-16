/*
 * bloomrepeats, Find genomic repeats, using Bloom filter based prefiltration
 * Copyright (C) 2012 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#include <algorithm>
#include <boost/foreach.hpp>

#include "stick_impl.hpp"
#include "Sequence.hpp"
#include "Fragment.hpp"
#include "Block.hpp"
#include "BlockSet.hpp"
#include "SortedVector.hpp"
#include "Graph.hpp"
#include "boundaries.hpp"
#include "throw_assert.hpp"

namespace bloomrepeats {

void bs_to_sb(Seq2Boundaries& sb, const BlockSet& bs) {
    BOOST_FOREACH (Block* block, bs) {
        BOOST_FOREACH (Fragment* fragment, *block) {
            sb[fragment->seq()].push_back(fragment->min_pos());
            sb[fragment->seq()].push_back(fragment->max_pos() + 1);
        }
    }
}

bool stick_fragments(BlockSet& bs, const Seq2Boundaries& sb, int min_distance) {
    bool result = false;
    BOOST_FOREACH (Block* block, bs) {
        BOOST_FOREACH (Fragment* f, *block) {
            Seq2Boundaries::const_iterator it = sb.find(f->seq());
            BOOST_ASSERT(it != sb.end());
            const Boundaries& boundaries = it->second;
            size_t min_pos = nearest_element(boundaries, f->min_pos());
            BOOST_ASSERT(std::abs(int(min_pos - f->min_pos())) <
                         min_distance);
            size_t max_pos = nearest_element(boundaries, f->max_pos() + 1) - 1;
            BOOST_ASSERT(std::abs(int(max_pos - f->max_pos())) <
                         min_distance);
            if (min_pos != f->min_pos() || max_pos != f->max_pos()) {
                f->set_min_pos(min_pos);
                f->set_max_pos(max_pos);
                result = true;
            }
        }
    }
    return result;
}

void stick_boundaries(Seq2Boundaries& sb, int min_distance) {
    BOOST_FOREACH (Seq2Boundaries::value_type& s_and_b, sb) {
        const Sequence* seq = s_and_b.first;
        Boundaries& b = s_and_b.second;
        select_boundaries(b, min_distance, seq->size());
    }
}

}

