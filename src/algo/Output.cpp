/*
 * bloomrepeats, Find genomic repeats, using Bloom filter based prefiltration
 * Copyright (C) 2012 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#include <ostream>
#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include "Output.hpp"
#include "BlockSet.hpp"
#include "Block.hpp"
#include "Fragment.hpp"
#include "Sequence.hpp"

namespace bloomrepeats {

Output::Output(const std::string& prefix) {
    set_opt_prefix(prefix);
    add_opt("dump-seq", "dump sequences before blocks", false);
    add_opt("dump-block", "dump blocks", true);
    add_opt("export-alignment",
            "use alignment information if available", true);
}

static struct FragmentCompareName2 {
    bool operator()(const Fragment* a, const Fragment* b) const {
        typedef boost::tuple<size_t, size_t, int, const std::string&> Tie;
        return Tie(a->min_pos(), a->max_pos(), a->ori(), a->seq()->name()) <
               Tie(b->min_pos(), b->max_pos(), b->ori(), b->seq()->name());
    }
} fcn2;

void Output::print_block(std::ostream& o, Block* block) const {
    bool export_alignment = opt_value("export-alignment").as<bool>();
    if (opt_value("dump-block").as<bool>()) {
        std::vector<Fragment*> fragments(block->begin(), block->end());
        std::sort(fragments.begin(), fragments.end(), fcn2);
        BOOST_FOREACH (Fragment* fr, fragments) {
            o << '>';
            fr->print_header(o, block);
            o << std::endl;
            fr->print_contents(o, export_alignment ? '-' : 0x00);
            o << std::endl;
        }
        o << std::endl;
    }
}

void Output::print_header(std::ostream& o) const {
    if (opt_value("dump-seq").as<bool>()) {
        BOOST_FOREACH (SequencePtr seq, block_set()->seqs()) {
            o << *seq << '\n';
        }
    }
}

const char* Output::name_impl() const {
    return "Output block set";
}

}

