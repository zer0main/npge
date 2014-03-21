/*
 * bloomrepeats, Find genomic repeats, using Bloom filter based prefiltration
 * Copyright (C) 2014 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#ifndef BR_CHR_BLOCK_SET_ALIGNMENT_HPP_
#define BR_CHR_BLOCK_SET_ALIGNMENT_HPP_

#include "global.hpp"
#include "Processor.hpp"

namespace bloomrepeats {

/** Build block set alignments for all chromosomes */
class ChrBlockSetAlignment : public Processor {
public:
    /** Constructor */
    ChrBlockSetAlignment();

protected:
    bool run_impl() const;

    const char* name_impl() const;

private:
    BlockSetAlignment* bsa_;
};

}

#endif
