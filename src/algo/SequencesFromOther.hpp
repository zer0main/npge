/*
 * NPG-explorer, Nucleotide PanGenome explorer
 * Copyright (C) 2012-2016 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#ifndef NPGE_SEQUENCES_FROM_OTHER_HPP_
#define NPGE_SEQUENCES_FROM_OTHER_HPP_

#include "Processor.hpp"

namespace npge {

/** Copy sequences from other blockset.
Wrapper of BlockSet::add_sequences().
*/
class SequencesFromOther : public Processor {
public:
    /** Constructor
    \param source BlockSet, from which sequences will be added to block_set().
    */
    SequencesFromOther(const BlockSetPtr& source = BlockSetPtr());

protected:
    void run_impl() const;
    const char* name_impl() const;
};

}

#endif

