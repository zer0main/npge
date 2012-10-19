/*
 * bloomrepeats, Find genomic repeats, using Bloom filter based prefiltration
 * Copyright (C) 2012 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#ifndef BR_ALIGNMENT_HPP
#define BR_ALIGNMENT_HPP

#include <iosfwd>
#include <map>
#include <string>

#include "global.hpp"
#include "AlignmentRow.hpp"

namespace bloomrepeats {

class Alignment {
public:
    Alignment(RowType type = MAP_ROW);

    /** Destructor.
    Clear and disconnect from the block, if connected.
    */
    ~Alignment();

    int add_row(Fragment* fragment, const std::string& alignment_string);

    int add_fragment(Fragment* fragment); // with empty body

    Block* block() const {
        return block_;
    }

    void grow_row(int index, const std::string& alignment_string);

    void remove_row(int index);

    int index_of(Fragment* fragment) const;

    Fragment* fragment_at(int index) const;

    int map_to_alignment(int index, int fragment_pos) const;

    int map_to_fragment(int index, int align_pos) const;

    int nearest_in_fragment(int index, int align_pos) const;

    // TODO
    //void bind(int index, int fragment_pos, int align_pos);
    // read from and write to stream

    int size() const;

    int length() const {
        return length_;
    }

    RowType row_type() const {
        return row_type_;
    }

    void set_row_type(RowType row_type) {
        row_type_ = row_type;
    }

    void print_alignment_string(int index, std::ostream& o) const;

    void print(int index, std::ostream& o) const;

private:
    // TODO memory-friendly implementation
    typedef std::map<int, AlignmentRow*> Rows;
    typedef std::map<Fragment*, int> Fragment2Index;

    Rows rows_;
    Fragment2Index fragment_to_index_;
    int length_;
    Block* block_;
    RowType row_type_;

    friend class Block;
};

/** Streaming operator */
std::ostream& operator<<(std::ostream& o, const Alignment& alignment);

}

#endif

