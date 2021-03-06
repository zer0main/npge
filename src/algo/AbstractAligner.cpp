/*
 * NPG-explorer, Nucleotide PanGenome explorer
 * Copyright (C) 2012-2016 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#include <boost/foreach.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include "AbstractAligner.hpp"
#include "AlignmentRow.hpp"
#include "Block.hpp"
#include "Fragment.hpp"
#include "RowStorage.hpp"
#include "refine_alignment.hpp"
#include "throw_assert.hpp"
#include "cast.hpp"

namespace npge {

AbstractAligner::AbstractAligner() {
    declare_bs("target", "Target blockset");
    add_row_storage_options(this);
}

struct BlockSquareLess {
    bool operator()(Block* a, Block* b) const {
        int a_length = a->front() ? a->front()->length() : 0;
        int b_length = b->front() ? b->front()->length() : 0;
        return b_length * b->size() < a_length * a->size();
    }
};

void AbstractAligner::change_blocks_impl(Blocks& blocks) const {
    std::sort(blocks.begin(), blocks.end(), BlockSquareLess());
}

bool AbstractAligner::test(bool gaps) const {
    Strings aln;
    aln.push_back("AT");
    aln.push_back(gaps ? "T" : "A");
    try {
        align_seqs(aln);
    } catch (...) {
        return false;
    }
    return aln[0] == "AT" && aln[1] == (gaps ? "-T" : "A-");
}

void AbstractAligner::align_block(Block* block) const {
    TimeIncrementer ti(this);
    if (!alignment_needed(block)) {
        return;
    }
    Fragments fragments((block->begin()), block->end());
    Strings rows;
    BOOST_FOREACH (Fragment* f, fragments) {
        rows.push_back(f->str(/* gap */ 0));
    }
    align_seqs(rows);
    refine_alignment(rows);
    ASSERT_EQ(rows.size(), fragments.size());
    for (int i = 0; i < fragments.size(); i++) {
        AlignmentRow* row = create_row(this);
        fragments[i]->set_row(row);
        row->grow(rows[i]);
    }
}

static bool is_pure_gap(const Strings& seqs, int col) {
    BOOST_FOREACH (const std::string& seq, seqs) {
        if (seq[col] != '-') {
            return false;
        }
    }
    return true;
}

static void copy_col(Strings& seqs,
                     int dest_col, int src_col) {
    if (dest_col != src_col) {
        BOOST_FOREACH (std::string& seq, seqs) {
            seq[dest_col] = seq[src_col];
        }
    }
}

static void remove_gaps(Strings& seqs) {
    int length = seqs.front().length();
    int dest_col = 0;
    for (int src_col = 0; src_col < length; src_col++) {
        if (!is_pure_gap(seqs, src_col)) {
            copy_col(seqs, dest_col, src_col);
            dest_col += 1;
        }
    }
    ASSERT_LTE(dest_col, length);
    BOOST_FOREACH (std::string& seq, seqs) {
        seq.resize(dest_col);
    }
}

void AbstractAligner::align_seqs(Strings& seqs) const {
    TimeIncrementer ti(this);
    if (seqs.empty()) {
        return;
    }
    typedef std::vector<int> Ints;
    Ints index_in_seqs, empty_seqs;
    Strings non_empty_seqs;
    for (int i = 0; i < seqs.size(); i++) {
        std::string& seq = seqs[i];
        if (seq.empty()) {
            empty_seqs.push_back(i);
        } else {
            index_in_seqs.push_back(i);
            non_empty_seqs.push_back(std::string());
            non_empty_seqs.back().swap(seq);
        }
    }
    int size_before = non_empty_seqs.size();
    if (size_before == 0) {
        return;
    }
    align_seqs_impl(non_empty_seqs);
    int size_after = non_empty_seqs.size();
    ASSERT_EQ(size_after, size_before);
    int length = non_empty_seqs.front().length();
    for (int i = 0; i < index_in_seqs.size(); i++) {
        int si = index_in_seqs[i];
        seqs[si].swap(non_empty_seqs[i]);
    }
    BOOST_FOREACH (int i, empty_seqs) {
        seqs[i].resize(length, '-');
    }
    BOOST_FOREACH (std::string& seq, seqs) {
        using namespace boost::algorithm;
        to_upper(seq);
        ASSERT_EQ(seq.length(), length);
    }
    remove_gaps(seqs);
}

bool AbstractAligner::alignment_needed(Block* block) const {
    if (block->size() == 0) {
        return false;
    } else if (block->size() == 1) {
        Fragment* f = block->front();
        if (f->row() && f->row()->length() == f->length()) {
            return false;
        }
        AlignmentRow* row = create_row(this);
        int length = f->length();
        row->set_length(length);
        for (int i = 0; i < length; i++) {
            row->bind(i, i);
        }
        f->set_row(row);
        return false;
    }
    if (block->front()->row()) {
        int row_length = block->front()->row()->length();
        bool all_rows = true;
        BOOST_FOREACH (Fragment* f, *block) {
            if (!f->row() || f->row()->length() != row_length) {
                all_rows = false;
                break;
            }
        }
        if (all_rows) {
            // all fragments have rows and lengthes are equal
            return false;
        }
    }
    return true;
}

void AbstractAligner::remove_pure_gap_columns(
    Block* block) {
    Fragments fragments((block->begin()), block->end());
    Strings rows;
    RowType type = COMPACT_ROW;
    BOOST_FOREACH (Fragment* f, fragments) {
        rows.push_back(f->str('-'));
        if (f->row()) {
            type = f->row()->type();
        }
    }
    remove_gaps(rows);
    for (int i = 0; i < fragments.size(); i++) {
        AlignmentRow* row = AlignmentRow::new_row(type);
        fragments[i]->set_row(row);
        row->grow(rows[i]);
    }
}

void AbstractAligner::process_block_impl(Block* block,
        ThreadData*) const {
    align_block(block);
}

const char* AbstractAligner::name_impl() const {
    return "Align blocks";
}

}

