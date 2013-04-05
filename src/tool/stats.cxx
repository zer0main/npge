/*
 * bloomrepeats, Find genomic repeats, using Bloom filter based prefiltration
 * Copyright (C) 2012 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#include "process.hpp"
#include "Pipe.hpp"
#include "AddBlocks.hpp"
#include "Stats.hpp"

using namespace bloomrepeats;

class StatsPipe : public Pipe {
public:
    StatsPipe() {
        add(new AddBlocks);
        add(new Stats, "no_remove_after");
    }
};

int main(int argc, char** argv) {
    return process(argc, argv, new StatsPipe,
                   "Print human readable summary about block set");
}

