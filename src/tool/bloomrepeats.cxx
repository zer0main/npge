/*
 * bloomrepeats, Find genomic repeats, using Bloom filter based prefiltration
 * Copyright (C) 2012 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#include "process.hpp"
#include "Pipe.hpp"
#include "AddBlocks.hpp"
#include "AnchorFinder.hpp"
#include "CleanUp.hpp"
#include "CheckNoOverlaps.hpp"
#include "OutputPipe.hpp"

using namespace bloomrepeats;

class BloomRepeatsPipe : public Pipe {
public:
    BloomRepeatsPipe() {
        add(new AddBlocks);
        add(new AnchorFinder);
        add(new CleanUp);
        add(new CheckNoOverlaps);
        add(new OutputPipe);
    }
};

int main(int argc, char** argv) {
    return process(argc, argv, new BloomRepeatsPipe,
                   "Find and expand anchors", "in-blocks");
}

