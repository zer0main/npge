/*
 * bloomrepeats, Find genomic repeats, using Bloom filter based prefiltration
 * Copyright (C) 2014 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#ifndef BR_MOVE_HPP_
#define BR_MOVE_HPP_

#include "Processor.hpp"

namespace bloomrepeats {

/** Move all blocks from other blockset to target blockset */
class Move : public Processor {
protected:
    bool run_impl() const;
};

}

#endif

