/*
 * NPG-explorer, Nucleotide PanGenome explorer
 * Copyright (C) 2012-2016 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#include <boost/foreach.hpp>

#include "Joiner.hpp"
#include "MetaAligner.hpp"
#include "AlignmentRow.hpp"
#include "Fragment.hpp"
#include "Block.hpp"
#include "BlockSet.hpp"
#include "block_hash.hpp"
#include "throw_assert.hpp"

namespace npge {

Joiner::Joiner() {
    aligner_ = new MetaAligner;
    aligner_->set_parent(this);
    declare_bs("target", "Target blockset");
}

struct BlockGreater {
    bool operator()(const Block* b1, const Block* b2) const {
        return b1->size() > b2->size();
    }
};

Block* Joiner::neighbor_block(Block* b, int ori) const {
    Block* result = 0;
    Fragment* f = b->front();
    if (f) {
        Fragment* neighbor_f = s2f_.neighbor(f, ori);
        if (neighbor_f) {
            result = neighbor_f->block();
        }
    }
    return result;
}

bool Joiner::can_join(Fragment* one, Fragment* another) const {
    return one->seq() == another->seq() &&
           one->ori() == another->ori() &&
           s2f_.are_neighbors(one, another);
}

int Joiner::can_join(Block* one, Block* another) const {
    if (one->weak() || another->weak()) {
        return false;
    }
    if (one->size() != another->size()) {
        return false;
    }
    if (one->size() < 2) {
        return false;
    }
    if (another->size() < 2) {
        return false;
    }
    bool all[3] = {true, false, true};
    for (int ori = 1; ori >= -1; ori -= 2) {
        BOOST_FOREACH (Fragment* f, *one) {
            Fragment* f1 = s2f_.logical_neighbor(f, ori);
            if (!f1 || f1->block() != another ||
                    !can_join(f, f1)) {
                all[ori + 1] = false;
                break;
            }
        }
        if (all[ori + 1]) {
            break;
        }
    }
    int result = all[1 + 1] ? 1 : all[-1 + 1] ? -1 : 0;
    ASSERT_FALSE(result && !Block::match(one, another));
    return result;
}

void Joiner::build_alignment(Strings& rows,
                             const Fragments& fragments,
                             const Block* another,
                             int logical_ori) const {
    Strings middle;
    int size = fragments.size();
    middle.resize(size);
    for (int i = 0; i < size; i++) {
        Fragment* f = fragments[i];
        Fragment* f1 = s2f_.logical_neighbor(f, logical_ori);
        ASSERT_TRUE(f1);
        ASSERT_EQ(f1->block(), another);
        ASSERT_EQ(f1->ori(), f->ori());
        std::string& seq = middle[i];
        int min_pos, max_pos;
        if (s2f_.next(f) == f1) {
            min_pos = f->max_pos() + 1;
            max_pos = f1->min_pos() - 1;
        } else {
            min_pos = f1->max_pos() + 1;
            max_pos = f->min_pos() - 1;
        }
        if (max_pos >= min_pos) {
            Fragment between(f->seq(),
                             min_pos, max_pos, f->ori());
            seq = between.str(0);
        }
    }
    aligner_->align_seqs(middle);
    rows.resize(size);
    for (int i = 0; i < size; i++) {
        Fragment* f = fragments[i];
        Fragment* f1 = s2f_.logical_neighbor(f, logical_ori);
        std::string& row = rows[i];
        if (logical_ori == 1) {
            row = f->str() + middle[i] + f1->str();
        } else {
            row = f1->str() + middle[i] + f->str();
        }
    }
}

Block* Joiner::join_blocks(Block* one, Block* another,
                           int logical_ori) const {
    TimeIncrementer ti(this);
    ASSERT_FALSE(one->weak());
    ASSERT_FALSE(another->weak());
    ASSERT_EQ(can_join(one, another), logical_ori);
    ASSERT_GTE(one->size(), 2);
    ASSERT_GTE(another->size(), 2);
    Block* result = new Block();
    Fragments fragments((one->begin()), one->end());
    int size = fragments.size();
    ASSERT_GT(size, 0);
    ASSERT_EQ(another->size(), size);
    Strings rows;
    RowType type;
    bool aln = has_alignment(one) && has_alignment(another);
    if (aln) {
        build_alignment(rows, fragments, another, logical_ori);
        type = one->front()->row()->type();
    }
    Fragments new_fragments;
    BOOST_FOREACH (Fragment* f, fragments) {
        Fragment* f1 = s2f_.logical_neighbor(f, logical_ori);
        ASSERT_TRUE(f1);
        ASSERT_EQ(f1->block(), another);
        Fragment* new_fragment = join(f, f1);
        result->insert(new_fragment);
        new_fragments.push_back(new_fragment);
    }
    ASSERT_EQ(new_fragments.size(), size);
    if (aln) {
        ASSERT_EQ(rows.size(), size);
        for (int i = 0; i < size; i++) {
            Fragment* new_fragment = new_fragments[i];
            AlignmentRow* new_row = AlignmentRow::new_row(type);
            new_fragment->set_row(new_row);
            new_row->grow(rows[i]);
        }
    }
    return result;
}

Fragment* Joiner::join(Fragment* one,
                       Fragment* another) const {
    ASSERT_TRUE(can_join(one, another));
    if (s2f_.next(another) == one) {
        std::swap(one, another);
    }
    ASSERT_EQ(s2f_.next(one), another);
    Fragment* new_fragment = new Fragment(one->seq());
    new_fragment->set_min_pos(std::min(one->min_pos(),
                                       another->min_pos()));
    new_fragment->set_max_pos(std::max(one->max_pos(),
                                       another->max_pos()));
    new_fragment->set_ori(one->ori());
    return new_fragment;
}

bool Joiner::can_join_fragments(Fragment* f1,
                                Fragment* f2) const {
    TimeIncrementer ti(this);
    if (!can_join(f1, f2)) {
        return false;
    }
    return true;
}

bool Joiner::can_join_blocks(Block* b1, Block* b2) const {
    TimeIncrementer ti(this);
    int ori = can_join(b1, b2);
    if (ori == 0) {
        return false;
    }
    ASSERT_TRUE(ori);
    ASSERT_FALSE(b1->empty());
    ASSERT_FALSE(b2->empty());
    int min_gap = -1, max_gap = -1;
    BOOST_FOREACH (Fragment* f1, *b1) {
        Fragment* f2 = s2f_.logical_neighbor(f1, ori);
        ASSERT_TRUE(f2);
        ASSERT_EQ(f2->block(), b2);
        if (!can_join_fragments(f1, f2)) {
            return false;
        }
        int dist = f1->dist_to(*f2);
        min_gap = (min_gap == -1 || dist < min_gap)
                  ? dist : min_gap;
        max_gap = (max_gap == -1 || dist > max_gap)
                  ? dist : max_gap;
    }
    return true;
}

Block* Joiner::try_join(Block* one, Block* another) const {
    TimeIncrementer ti(this);
    Block* result = 0;
    int match_ori = Block::match(one, another);
    if (match_ori == -1) {
        another->inverse();
    }
    if (match_ori) {
        int logical_ori = can_join(one, another);
        if (logical_ori && can_join_blocks(one, another)) {
            result = join_blocks(one, another, logical_ori);
        }
    }
    return result;
}

void Joiner::run_impl() const {
    s2f_.set_cycles_allowed(false);
    s2f_.clear();
    s2f_.add_bs(*block_set());
    Blocks bs(block_set()->begin(), block_set()->end());
    std::sort(bs.begin(), bs.end(), BlockGreater());
    BOOST_FOREACH (Block* block, bs) {
        if (block_set()->has(block)) {
            for (int ori = -1; ori <= 1; ori += 2) {
                while (Block* other_block =
                            neighbor_block(block, ori)) {
                    Block* new_block =
                        try_join(block, other_block);
                    if (new_block) {
                        s2f_.remove_block(block);
                        block_set()->erase(block);
                        s2f_.remove_block(other_block);
                        block_set()->erase(other_block);
                        block_set()->insert(new_block);
                        s2f_.add_block(new_block);
                        block = new_block;
                    } else {
                        break;
                    }
                }
            }
        }
    }
}

const char* Joiner::name_impl() const {
    return "Join blocks";
}

}

