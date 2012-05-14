/*
 * bloomrepeats, Find genomic repeats, using Bloom filter based prefiltration
 * Copyright (C) 2012 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#ifndef BR_FRAGMENT_HPP_
#define BR_FRAGMENT_HPP_

#include <iosfwd>
#include <string>
#include <boost/weak_ptr.hpp>

#include "global.hpp"

namespace bloomrepeats {

class Fragment {
public:
    Fragment(SequencePtr seq = SequencePtr(),
             size_t min_pos = 0, size_t max_pos = 0, int ori = 1);

    SequencePtr seq() const {
        return seq_;
    }

    BlockPtr block() const;

    FragmentPtr prev() const;

    FragmentPtr next() const;

    FragmentPtr neighbour(int ori) const;

    FragmentPtr logical_neighbour(int ori) const;

    bool is_neighbour(const Fragment& other) const;

    FragmentPtr another_neighbour(const Fragment& other) const;

    size_t min_pos() const {
        return min_pos_;
    }

    void set_min_pos(size_t min_pos) {
        min_pos_ = min_pos;
    }

    size_t max_pos() const {
        return max_pos_;
    }

    void set_max_pos(size_t max_pos) {
        max_pos_ = max_pos;
    }

    int ori() const {
        return ori_;
    }

    size_t length() const;

    void set_ori(int ori) {
        ori_ = ori;
    }

    void inverse();

    size_t begin_pos() const;

    const char* begin() const;

    size_t end_pos() const;

    const char* end() const;

    std::string str() const;

    std::string substr(int from, int to) const;

    void shift_end(int shift = 1);

    bool valid() const;

    bool operator==(const Fragment& other) const;

    bool operator!=(const Fragment& other) const;

    char at(int pos) const;

    static void connect(FragmentPtr first, FragmentPtr second);

    void disconnect();

    size_t common_positions(const Fragment& other);

private:
    SequencePtr seq_;
    size_t min_pos_;
    size_t max_pos_;
    int ori_;
    boost::weak_ptr<Block> block_;
    boost::weak_ptr<Fragment> prev_;
    boost::weak_ptr<Fragment> next_;

    friend class Block;
};

std::ostream& operator<<(std::ostream& o, const Fragment& fragment);

}

#endif

