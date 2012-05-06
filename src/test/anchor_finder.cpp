/*
 * bloomrepeats, Find genomic repeats, using Bloom filter based prefiltration
 * Copyright (C) 2012 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#include <boost/test/unit_test.hpp>
#include <boost/bind.hpp>
#include <iostream>

#include "Sequence.hpp"
#include "Fragment.hpp"
#include "Block.hpp"
#include "AnchorFinder.hpp"

BOOST_AUTO_TEST_CASE (AnchorFinder_main) {
    using namespace bloomrepeats;
    SequencePtr s1 = boost::make_shared<InMemorySequence>("tgGTCCGagCGGACggcc");
    std::vector<BlockPtr> blocks;
    AnchorFinder anchor_finder;
    anchor_finder.add_sequence(s1);
    anchor_finder.set_anchor_handler(
        boost::bind(&std::vector<BlockPtr>::push_back, &blocks, _1));
    anchor_finder.set_anchor_size(5);
    anchor_finder.run();
    BOOST_REQUIRE(blocks.size() == 1);
    FragmentPtr f = blocks.front()->front();
    BOOST_REQUIRE(f->str() == "gtccg" || f->str() == "cggac");
}

