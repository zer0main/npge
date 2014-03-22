/*
 * bloomrepeats, Find genomic repeats, using Bloom filter based prefiltration
 * Copyright (C) 2012 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#ifndef BR_SAME_CHR_HPP_
#define BR_SAME_CHR_HPP_

#include "Processor.hpp"

namespace bloomrepeats {

/** Filter out blocks fragments of which are located on different chromosomes.
Chromosome name is obtained by Sequence::chromosome().
*/
class SameChr : public Processor {
public:
    /** Return if all fragment of this block are located on same chromosome */
    static bool same_chr(const Block* block);

protected:
    void run_impl() const;

    const char* name_impl() const;
};

}

#endif

