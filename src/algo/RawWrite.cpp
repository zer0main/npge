/*
 * NPG-explorer, Nucleotide PanGenome explorer
 * Copyright (C) 2012-2016 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#include <ostream>
#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include "RawWrite.hpp"
#include "BlockSet.hpp"
#include "Block.hpp"
#include "Fragment.hpp"
#include "Sequence.hpp"

namespace npge {

RawWrite::RawWrite(const std::string& prefix) {
    set_opt_prefix(prefix);
    add_opt("dump-seq", "dump sequences before blocks", false);
    add_opt("dump-block", "dump blocks", true);
    add_opt("export-contents", "print contents of fragments", true);
    add_opt("export-alignment",
            "use alignment information if available", true);
    declare_bs("target", "Target blockset");
}

static struct FragmentCompareName2 {
    bool operator()(const Fragment* a, const Fragment* b) const {
        typedef boost::tuple<pos_t, pos_t, int, const std::string&> Tie;
        return Tie(a->min_pos(), a->max_pos(), a->ori(), a->seq()->name()) <
               Tie(b->min_pos(), b->max_pos(), b->ori(), b->seq()->name());
    }
} fcn2;

void RawWrite::print_block(std::ostream& o, Block* block) const {
    bool export_alignment = opt_value("export-alignment").as<bool>();
    bool export_contents = opt_value("export-contents").as<bool>();
    if (opt_value("dump-block").as<bool>()) {
        std::vector<Fragment*> fragments(block->begin(), block->end());
        std::sort(fragments.begin(), fragments.end(), fcn2);
        BOOST_FOREACH (Fragment* fr, fragments) {
            o << '>';
            fr->print_header(o, block);
            o << std::endl;
            if (export_contents) {
                fr->print_contents(o, export_alignment ? '-' : 0x00);
                o << std::endl;
            }
        }
        o << std::endl;
    }
}

struct SeqNameCmp {
    bool operator()(const SequencePtr& a,
                    const SequencePtr& b) const {
        return a->name() < b->name();
    }
};

void RawWrite::print_header(std::ostream& o) const {
    if (opt_value("dump-seq").as<bool>()) {
        std::vector<SequencePtr> seqs = block_set()->seqs();
        std::sort(seqs.begin(), seqs.end(), SeqNameCmp());
        BOOST_FOREACH (SequencePtr seq, seqs) {
            o << *seq << '\n';
        }
    }
}

const char* RawWrite::name_impl() const {
    return "Write blockset to file";
}

}

