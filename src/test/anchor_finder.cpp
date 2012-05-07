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

BOOST_AUTO_TEST_CASE (AnchorFinder_palindrome_elimination) {
    using namespace bloomrepeats;
    SequencePtr s1 = boost::make_shared<InMemorySequence>("atgcat");
    std::vector<BlockPtr> blocks;
    AnchorFinder anchor_finder;
    anchor_finder.add_sequence(s1);
    anchor_finder.set_anchor_handler(
        boost::bind(&std::vector<BlockPtr>::push_back, &blocks, _1));
    anchor_finder.set_anchor_size(6);
    anchor_finder.set_palindromes_elimination(true);
    BOOST_REQUIRE(anchor_finder.palindromes_elimination());
    anchor_finder.run();
    BOOST_REQUIRE(blocks.size() == 0);
    //
    blocks.clear();
    anchor_finder.set_palindromes_elimination(false);
    BOOST_REQUIRE(!anchor_finder.palindromes_elimination());
    anchor_finder.run();
    BOOST_REQUIRE(blocks.size() == 1);
}

BOOST_AUTO_TEST_CASE (AnchorFinder_only_ori) {
    using namespace bloomrepeats;
    SequencePtr s1 = boost::make_shared<InMemorySequence>("tgGTCCGagCGGACggcc");
    std::vector<BlockPtr> blocks;
    AnchorFinder anchor_finder;
    anchor_finder.add_sequence(s1);
    anchor_finder.set_anchor_handler(
        boost::bind(&std::vector<BlockPtr>::push_back, &blocks, _1));
    anchor_finder.set_anchor_size(5);
    anchor_finder.set_only_ori(0);
    BOOST_REQUIRE(anchor_finder.only_ori() == 0);
    anchor_finder.run();
    BOOST_REQUIRE(blocks.size() == 1);
    //
    blocks.clear();
    anchor_finder.set_only_ori(1);
    BOOST_REQUIRE(anchor_finder.only_ori() == 1);
    anchor_finder.run();
    BOOST_REQUIRE(blocks.size() == 0);
    //
    blocks.clear();
    anchor_finder.set_only_ori(-1);
    BOOST_REQUIRE(anchor_finder.only_ori() == -1);
    anchor_finder.run();
    BOOST_REQUIRE(blocks.size() == 0);
}

BOOST_AUTO_TEST_CASE (AnchorFinder_one_from_long_repeat) {
    using namespace bloomrepeats;
    SequencePtr s1 = boost::make_shared<InMemorySequence>("aaGCCCaaGCCCaa");
    std::vector<BlockPtr> blocks;
    AnchorFinder anchor_finder;
    anchor_finder.add_sequence(s1);
    anchor_finder.set_anchor_handler(
        boost::bind(&std::vector<BlockPtr>::push_back, &blocks, _1));
    anchor_finder.set_anchor_size(3);
    anchor_finder.run();
    BOOST_REQUIRE(blocks.size() == 1);
}

BOOST_AUTO_TEST_CASE (AnchorFinder_several_sequences) {
    using namespace bloomrepeats;
    SequencePtr s1 = boost::make_shared<InMemorySequence>("aaGCCCaaGCCCaa");
    SequencePtr s2 = boost::make_shared<InMemorySequence>("aaGCCCaaGCCCaa");
    std::vector<BlockPtr> blocks;
    AnchorFinder anchor_finder;
    anchor_finder.add_sequence(s1);
    anchor_finder.add_sequence(s2);
    anchor_finder.set_anchor_handler(
        boost::bind(&std::vector<BlockPtr>::push_back, &blocks, _1));
    anchor_finder.set_anchor_size(3);
    anchor_finder.run();
    BOOST_REQUIRE(blocks.size() == 1);
    BOOST_REQUIRE(blocks.front()->size() == 4);
}

BOOST_AUTO_TEST_CASE (AnchorFinder_two_workers) {
    using namespace bloomrepeats;
    SequencePtr s1 = boost::make_shared<InMemorySequence>("aaGCCCaaGCCCaa");
    SequencePtr s2 = boost::make_shared<InMemorySequence>("aaGCCCaaGCCCaa");
    std::vector<BlockPtr> blocks;
    AnchorFinder anchor_finder;
    anchor_finder.add_sequence(s1);
    anchor_finder.add_sequence(s2);
    anchor_finder.set_anchor_handler(
        boost::bind(&std::vector<BlockPtr>::push_back, &blocks, _1));
    anchor_finder.set_anchor_size(3);
    anchor_finder.set_workers(2);
    anchor_finder.run();
    BOOST_REQUIRE(blocks.size() == 1);
    BOOST_REQUIRE(blocks.front()->size() == 4);
}

