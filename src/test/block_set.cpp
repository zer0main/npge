/*
 * bloomrepeats, Find genomic repeats, using Bloom filter based prefiltration
 * Copyright (C) 2012 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#include "Sequence.hpp"
#include "Fragment.hpp"
#include "Block.hpp"
#include "BlockSet.hpp"
#include "Joiner.hpp"
#include "Filter.hpp"

BOOST_AUTO_TEST_CASE (BlockSet_connect) {
    using namespace bloomrepeats;
    SequencePtr s1 = boost::make_shared<InMemorySequence>("tggtcCGAGATgcgggcc");
    Fragment* f1 = new Fragment(s1, 1, 2, 1);
    Fragment* f2 = new Fragment(s1, 5, 6, -1);
    Fragment* f3 = new Fragment(s1, 7, 8, 1);
    Block* b1 = new Block();
    Block* b2 = new Block();
    Block* b3 = new Block();
    b1->insert(f1);
    b2->insert(f2);
    b3->insert(f3);
    BlockSetPtr block_set = boost::make_shared<BlockSet>();
    block_set->insert(b1);
    block_set->insert(b2);
    block_set->insert(b3);
    block_set->connect_fragments();
    BOOST_CHECK(f1->next() == f2);
    BOOST_CHECK(f2->prev() == f1);
    BOOST_CHECK(f2->next() == f3);
    BOOST_CHECK(f3->prev() == f2);
}

BOOST_AUTO_TEST_CASE (BlockSet_clone) {
    using namespace bloomrepeats;
    SequencePtr s1 = boost::make_shared<InMemorySequence>("tggtcCGAGATgcgggcc");
    Fragment* f1 = new Fragment(s1, 1, 2, 1);
    Fragment* f2 = new Fragment(s1, 5, 6, -1);
    Fragment* f3 = new Fragment(s1, 7, 8, 1);
    Block* b1 = new Block();
    Block* b2 = new Block();
    Block* b3 = new Block();
    b1->insert(f1);
    b2->insert(f2);
    b3->insert(f3);
    BlockSetPtr block_set = boost::make_shared<BlockSet>();
    block_set->insert(b1);
    block_set->insert(b2);
    block_set->insert(b3);
    block_set->connect_fragments();
    BlockSetPtr block_set_copy = block_set->clone();
    BOOST_CHECK(block_set_copy->size() == 3);
    BOOST_CHECK(block_set_copy->front()->front()->prev() ||
                block_set_copy->front()->front()->next());
}

BOOST_AUTO_TEST_CASE (BlockSet_filter) {
    using namespace bloomrepeats;
    SequencePtr s1 = boost::make_shared<InMemorySequence>("tggtcCGAGATgcgggcc");
    Fragment* f1 = new Fragment(s1, 1, 2, 1);
    Fragment* f2 = new Fragment(s1, 4, 6, -1);
    Fragment* f3 = new Fragment(s1, 7, 8, 1);
    Block* b1 = new Block();
    Block* b2 = new Block();
    Block* b3 = new Block();
    b1->insert(f1);
    b2->insert(f2);
    b3->insert(f3);
    BlockSetPtr block_set = boost::make_shared<BlockSet>();
    block_set->insert(b1);
    block_set->insert(b2);
    block_set->insert(b3);
    Filter filter(3, 1);
    filter.apply(block_set);
    BOOST_CHECK(block_set->size() == 1);
}

BOOST_AUTO_TEST_CASE (BlockSet_join) {
    using namespace bloomrepeats;
    SequencePtr s1 = boost::make_shared<InMemorySequence>("tggtcCGAGATgcgggcc");
    Fragment* f11 = new Fragment(s1, 1, 2, 1);
    Fragment* f21 = new Fragment(s1, 4, 6, -1);
    Fragment* f31 = new Fragment(s1, 7, 8, 1);
    SequencePtr s2 = boost::make_shared<InMemorySequence>("tggtcCGAGATgcgggcc");
    Fragment* f12 = new Fragment(s2, 1, 2, 1);
    Fragment* f22 = new Fragment(s2, 4, 6, -1);
    Fragment* f32 = new Fragment(s2, 7, 8, 1);
    Block* b1 = new Block();
    Block* b2 = new Block();
    Block* b3 = new Block();
    b1->insert(f11);
    b1->insert(f12);
    b2->insert(f21);
    b2->insert(f22);
    b3->insert(f31);
    b3->insert(f32);
    BlockSetPtr block_set = boost::make_shared<BlockSet>();
    block_set->insert(b1);
    block_set->insert(b2);
    block_set->insert(b3);
    block_set->connect_fragments();
    block_set->join();
    BOOST_CHECK(block_set->size() == 1);
    BOOST_CHECK(block_set->front()->size() == 2);
    BOOST_CHECK(block_set->front()->front()->length() == 8);
}

BOOST_AUTO_TEST_CASE (BlockSet_join_max_gap) {
    using namespace bloomrepeats;
    SequencePtr s1 = boost::make_shared<InMemorySequence>("tggtcCGAGATgcgggcc");
    Fragment* f11 = new Fragment(s1, 1, 2, 1);
    Fragment* f21 = new Fragment(s1, 4, 6, -1);
    Fragment* f31 = new Fragment(s1, 7, 8, 1);
    SequencePtr s2 = boost::make_shared<InMemorySequence>("tggtcCGAGATgcgggcc");
    Fragment* f12 = new Fragment(s2, 1, 2, 1);
    Fragment* f22 = new Fragment(s2, 4, 6, -1);
    Fragment* f32 = new Fragment(s2, 7, 8, 1);
    Block* b1 = new Block();
    Block* b2 = new Block();
    Block* b3 = new Block();
    b1->insert(f11);
    b1->insert(f12);
    b2->insert(f21);
    b2->insert(f22);
    b3->insert(f31);
    b3->insert(f32);
    BlockSetPtr block_set = boost::make_shared<BlockSet>();
    block_set->insert(b1);
    block_set->insert(b2);
    block_set->insert(b3);
    block_set->connect_fragments();
    Joiner dist_0(0);
    block_set->join(&dist_0);
    BOOST_CHECK(block_set->size() == 2);
    BOOST_CHECK(block_set->front()->size() == 2);
    BOOST_CHECK(block_set->front()->front()->length() == 5 ||
                block_set->front()->front()->length() == 2);
}

BOOST_AUTO_TEST_CASE (BlockSet_join_wrong) {
    using namespace bloomrepeats;
    SequencePtr s1 = boost::make_shared<InMemorySequence>("tggtcCGAGATgcgggcc");
    Fragment* f11 = new Fragment(s1, 1, 2, 1);
    Fragment* f21 = new Fragment(s1, 4, 6, -1);
    Fragment* f31 = new Fragment(s1, 7, 8, 1);
    SequencePtr s2 = boost::make_shared<InMemorySequence>("tggtcCGAGATgcgggcc");
    Fragment* f12 = new Fragment(s2, 1, 2, 1);
    Fragment* f22 = new Fragment(s2, 4, 6, -1);
    Fragment* f32 = new Fragment(s2, 7, 8, -1);
    Block* b1 = new Block();
    Block* b2 = new Block();
    Block* b3 = new Block();
    b1->insert(f11);
    b1->insert(f12);
    b2->insert(f21);
    b2->insert(f22);
    b3->insert(f31);
    b3->insert(f32);
    BlockSetPtr block_set = boost::make_shared<BlockSet>();
    block_set->insert(b1);
    block_set->insert(b2);
    block_set->insert(b3);
    block_set->connect_fragments();
    block_set->join();
    BOOST_CHECK(block_set->size() == 2);
}

BOOST_AUTO_TEST_CASE (BlockSet_expand) {
    using namespace bloomrepeats;
    SequencePtr s1 = boost::make_shared<InMemorySequence>("tGGtccgagcgGAcggcc");
    SequencePtr s2 = boost::make_shared<InMemorySequence>("tGGtccGAcggccgcgga");
    SequencePtr s3 = boost::make_shared<InMemorySequence>("tGGtccgacggccgcgga");
    Block* b1 = new Block();
    b1->insert(new Fragment(s1, 1, 2));
    b1->insert(new Fragment(s2, 1, 2));
    b1->insert(new Fragment(s3, 1, 2));
    Block* b2 = new Block();
    b2->insert(new Fragment(s1, 11, 12));
    b2->insert(new Fragment(s2, 6, 7));
    BlockSetPtr block_set = boost::make_shared<BlockSet>();
    block_set->insert(b1);
    block_set->insert(b2);
    block_set->connect_fragments();
    block_set->expand_blocks();
    BOOST_CHECK(b1->front()->length() == 6);
    BOOST_CHECK(b1->front()->min_pos() == 0);
    BOOST_CHECK(b1->front()->str() == "tggtcc");
    BOOST_CHECK(b2->front()->length() == 7);
    BOOST_CHECK(b2->front()->str() == "gacggcc");
}

BOOST_AUTO_TEST_CASE (BlockSet_overlaps) {
    using namespace bloomrepeats;
    SequencePtr s1 = boost::make_shared<InMemorySequence>("ctgc|ACGC|gacgt");
    SequencePtr s2 = boost::make_shared<InMemorySequence>("ctgc|ACGCGA|cgt");
    SequencePtr s3 = boost::make_shared<InMemorySequence>("ctgcac|GCGA|cgt");
    SequencePtr s4 = boost::make_shared<InMemorySequence>("ctgcac|GCGA|cgt");
    Fragment* f11 = new Fragment(s1, 4, 7, -1);
    Fragment* f12 = new Fragment(s2, 4, 7, -1);
    Block* b1 = new Block();
    b1->insert(f11);
    b1->insert(f12);
    Fragment* f22 = new Fragment(s2, 6, 9);
    Fragment* f23 = new Fragment(s3, 6, 9);
    Fragment* f24 = new Fragment(s4, 6, 9);
    Block* b2 = new Block();
    b2->insert(f22);
    b2->insert(f23);
    b2->insert(f24);
    BlockSetPtr block_set = boost::make_shared<BlockSet>();
    block_set->insert(b1);
    block_set->insert(b2);
    block_set->connect_fragments();
    BOOST_CHECK(block_set->overlaps());
    f22->set_min_pos(8);
    BOOST_CHECK(!block_set->overlaps());
}

BOOST_AUTO_TEST_CASE (BlockSet_resolve_overlaps) {
    using namespace bloomrepeats;
    /*
    Input:
        Block 1:
            seq1: ---xxxx----
            seq2: ---xxxx----
        Block 2
            seq2: -----xxxx--
            seq3: -----xxxx--
            seq4: -----xxxx--
    Output of resolve_overlaps:
        Block 1:
            seq1: ---xx------
            seq2: ---xx------
        Block 2
            seq2: -------xx--
            seq3: -------xx--
            seq4: -------xx--
        Block 3:
            seq1: -----xx----
            seq2: -----xx----
            seq3: -----xx----
            seq4: -----xx----
    */
    SequencePtr s1 = boost::make_shared<InMemorySequence>("ctgc|ACAG|gacgt");
    SequencePtr s2 = boost::make_shared<InMemorySequence>("ctgc|ACAGGA|cgt");
    SequencePtr s3 = boost::make_shared<InMemorySequence>("ctgcac|AGGA|cgt");
    SequencePtr s4 = boost::make_shared<InMemorySequence>("ctgcac|AGGA|cgt");
    Fragment* f11 = new Fragment(s1, 4, 7, -1);
    Fragment* f12 = new Fragment(s2, 4, 7, -1);
    Block* b1 = new Block();
    b1->insert(f11);
    b1->insert(f12);
    Fragment* f22 = new Fragment(s2, 6, 9);
    Fragment* f23 = new Fragment(s3, 6, 9);
    Fragment* f24 = new Fragment(s4, 6, 9);
    Block* b2 = new Block();
    b2->insert(f22);
    b2->insert(f23);
    b2->insert(f24);
    BlockSetPtr block_set = boost::make_shared<BlockSet>();
    block_set->insert(b1);
    block_set->insert(b2);
    block_set->connect_fragments();
    block_set->resolve_overlaps();
    BOOST_REQUIRE(block_set->size() == 3);
    bool b[5] = {0, 0, 0, 0, 0};
    BOOST_FOREACH (Block* block, *block_set) {
        BOOST_REQUIRE(block->size() <= 4);
        b[block->size()] = true;
        if (block->size() == 2) {
            BOOST_CHECK(block->front()->str() == "ac" ||
                        block->front()->str() == "gt");
        } else if (block->size() == 3) {
            BOOST_CHECK(block->front()->str() == "ga" ||
                        block->front()->str() == "tc");
        } else if (block->size() == 4) {
            BOOST_CHECK(block->front()->str() == "ag" ||
                        block->front()->str() == "ct");
        } else {
            BOOST_ERROR("Bad block size");
        }
    }
    BOOST_CHECK(!b[0] && !b[1] && b[2] && b[3] && b[4]);
}

BOOST_AUTO_TEST_CASE (BlockSet_resolve_overlaps_internal) {
    using namespace bloomrepeats;
    /*
    Input:
        Block 1:
            seq1: ---xxxx----
            seq2: ---xxxx----
        Block 2
            seq1: -----xxxx--
            seq2: -----xxxx--
            seq3: -----xxxx--
            seq4: -----xxxx--
    */
    SequencePtr s1 = boost::make_shared<InMemorySequence>("ctgc|ACAGGA|cgt");
    SequencePtr s2 = boost::make_shared<InMemorySequence>("ctgc|ACAGGA|cgt");
    SequencePtr s3 = boost::make_shared<InMemorySequence>("ctgcac|AGGA|cgt");
    SequencePtr s4 = boost::make_shared<InMemorySequence>("ctgcac|AGGA|cgt");
    Fragment* f11 = new Fragment(s1, 4, 7, -1);
    Fragment* f12 = new Fragment(s2, 4, 7, -1);
    Block* b1 = new Block();
    b1->insert(f11);
    b1->insert(f12);
    Fragment* f21 = new Fragment(s1, 6, 9);
    Fragment* f22 = new Fragment(s2, 6, 9);
    Fragment* f23 = new Fragment(s3, 6, 9);
    Fragment* f24 = new Fragment(s4, 6, 9);
    Block* b2 = new Block();
    b2->insert(f21);
    b2->insert(f22);
    b2->insert(f23);
    b2->insert(f24);
    BlockSetPtr block_set = boost::make_shared<BlockSet>();
    block_set->insert(b1);
    block_set->insert(b2);
    block_set->connect_fragments();
    block_set->resolve_overlaps();
    BOOST_REQUIRE(block_set->size() >= 2);
}

BOOST_AUTO_TEST_CASE (BlockSet_resolve_overlaps_two_overlaps) {
    using namespace bloomrepeats;
    /*
    Input:
        Block 1:
            seq0: ---xxxx----
            seq1: ---xxxx----
            seq2: ---xxxx----
        Block 2
            seq1: -----xxxx--
            seq2: -----xxxx--
            seq3: -----xxxx--
    Output of resolve_overlaps():
        Block 1:
            seq0: ---xx------
            seq1: ---xx------
            seq2: ---xx------
        Block 2
            seq1: -------xx--
            seq2: -------xx--
            seq3: -------xx--
        Block 3:
            seq0: -----xx----
            seq1: -----xx----
            seq2: -----xx----
            seq3: -----xx----
    */
    SequencePtr s0 = boost::make_shared<InMemorySequence>("ctgc|ACAG|gacgt");
    SequencePtr s1 = boost::make_shared<InMemorySequence>("ctgc|ACAGGA|cgt");
    SequencePtr s2 = boost::make_shared<InMemorySequence>("ctgc|ACAGGA|cgt");
    SequencePtr s3 = boost::make_shared<InMemorySequence>("ctgcac|AGGA|cgt");
    Fragment* f10 = new Fragment(s0, 4, 7, -1);
    Fragment* f11 = new Fragment(s1, 4, 7, -1);
    Fragment* f12 = new Fragment(s2, 4, 7, -1);
    Block* b1 = new Block();
    b1->insert(f10);
    b1->insert(f11);
    b1->insert(f12);
    Fragment* f21 = new Fragment(s1, 6, 9);
    Fragment* f22 = new Fragment(s2, 6, 9);
    Fragment* f23 = new Fragment(s3, 6, 9);
    Block* b2 = new Block();
    b2->insert(f21);
    b2->insert(f22);
    b2->insert(f23);
    BlockSetPtr block_set = boost::make_shared<BlockSet>();
    block_set->insert(b1);
    block_set->insert(b2);
    block_set->connect_fragments();
    block_set->resolve_overlaps();
    BOOST_REQUIRE(block_set->size() == 3);
    int b[5] = {0, 0, 0, 0, 0};
    BOOST_FOREACH (Block* block, *block_set) {
        BOOST_REQUIRE(block->size() <= 4);
        b[block->size()] += 1;
        if (block->size() == 3) {
            BOOST_CHECK(block->front()->str() == "ac" ||
                        block->front()->str() == "gt" ||
                        block->front()->str() == "ga" ||
                        block->front()->str() == "tc");
        } else if (block->size() == 4) {
            BOOST_CHECK(block->front()->str() == "ag" ||
                        block->front()->str() == "ct");
        } else {
            BOOST_ERROR("Bad block size");
        }
    }
    BOOST_CHECK(!b[0] && !b[1] && !b[2] && b[3] == 2 && b[4] == 1);
}

BOOST_AUTO_TEST_CASE (BlockSet_resolve_overlaps_internal_subfragment) {
    using namespace bloomrepeats;
    /*
    Input:
        Block 1:
            seq0: -----xx----
            seq1: -----xx----
            seq2: -----xx----
        Block 2
            seq1: ---xxxxxx--
            seq2: ---xxxxxx--
            seq3: ---xxxxxx--
    Output of resolve_overlaps():
        Block 1:
            seq1: ---xx------
            seq2: ---xx------
            seq3: ---xx------
        Block 2
            seq1: -------xx--
            seq2: -------xx--
            seq3: -------xx--
        Block 3:
            seq0: -----xx----
            seq1: -----xx----
            seq2: -----xx----
            seq3: -----xx----
    */
    SequencePtr s0 = boost::make_shared<InMemorySequence>("ctgcac|AG|gacgt");
    SequencePtr s1 = boost::make_shared<InMemorySequence>("ctgc|ACAGGA|cgt");
    SequencePtr s2 = boost::make_shared<InMemorySequence>("ctgc|ACAGGA|cgt");
    SequencePtr s3 = boost::make_shared<InMemorySequence>("ctgc|ACAGGA|cgt");
    Fragment* f10 = new Fragment(s0, 6, 7, 1);
    Fragment* f11 = new Fragment(s1, 6, 7, 1);
    Fragment* f12 = new Fragment(s2, 6, 7, 1);
    Block* b1 = new Block();
    b1->insert(f10);
    b1->insert(f11);
    b1->insert(f12);
    Fragment* f21 = new Fragment(s1, 4, 9);
    Fragment* f22 = new Fragment(s2, 4, 9);
    Fragment* f23 = new Fragment(s3, 4, 9);
    Block* b2 = new Block();
    b2->insert(f21);
    b2->insert(f22);
    b2->insert(f23);
    BlockSetPtr block_set = boost::make_shared<BlockSet>();
    block_set->insert(b1);
    block_set->insert(b2);
    block_set->connect_fragments();
    block_set->resolve_overlaps();
    BOOST_REQUIRE(block_set->size() == 3);
    int b[5] = {0, 0, 0, 0, 0};
    BOOST_FOREACH (Block* block, *block_set) {
        BOOST_REQUIRE(block->size() <= 4);
        b[block->size()] += 1;
        if (block->size() == 3) {
            BOOST_CHECK(block->front()->str() == "ac" ||
                        block->front()->str() == "gt" ||
                        block->front()->str() == "ga" ||
                        block->front()->str() == "tc");
            BOOST_CHECK(!block->front()->next() ||
                        block->front()->next()->block()->size() == 4);
            BOOST_CHECK(!block->front()->prev() ||
                        block->front()->prev()->block()->size() == 4);
        } else if (block->size() == 4) {
            BOOST_CHECK(block->front()->str() == "ag" ||
                        block->front()->str() == "ct");
            BOOST_CHECK(!block->front()->next() ||
                        block->front()->next()->block()->size() == 3);
            BOOST_CHECK(!block->front()->prev() ||
                        block->front()->prev()->block()->size() == 3);
        } else {
            BOOST_ERROR("Bad block size");
        }
    }
    BOOST_CHECK(!b[0] && !b[1] && !b[2] && b[3] == 2 && b[4] == 1);
}

BOOST_AUTO_TEST_CASE (BlockSet_resolve_overlaps_multioverlaps) {
    using namespace bloomrepeats;
    SequencePtr s[10];
    for (int j = 0; j < 10; j++) {
        s[j] = boost::make_shared<InMemorySequence>("ctgcacaggacgttgcacggacgt");
    }
    BlockSetPtr block_set = boost::make_shared<BlockSet>();
    for (int i = 0; i < 10; i++) {
        Block* b = new Block();
        for (int j = 0; j < 10; j++) {
            b->insert(new Fragment(s[j], i, i + 10));
        }
        block_set->insert(b);
    }
    block_set->connect_fragments();
    block_set->resolve_overlaps();
}

BOOST_AUTO_TEST_CASE (BlockSet_expand_blocks_by_fragments) {
    using namespace bloomrepeats;
    SequencePtr s1 = boost::make_shared<InMemorySequence>("tGGtccgagcgGAcggcc");
    SequencePtr s2 = boost::make_shared<InMemorySequence>("tGGtccgagcggacggcc");
    Block* b1 = new Block();
    Fragment* f11 = new Fragment(s1, 1, 2);
    Fragment* f12 = new Fragment(s2, 1, 2);
    b1->insert(f11);
    b1->insert(f12);
    Block* b2 = new Block();
    Fragment* f21 = new Fragment(s1, 11, 12);
    b2->insert(f21);
    BlockSetPtr block_set = boost::make_shared<BlockSet>();
    block_set->insert(b1);
    block_set->insert(b2);
    block_set->connect_fragments();
    BOOST_CHECK(block_set->expand_blocks_by_fragments());
    BOOST_CHECK(!b2->expand_by_fragments());
    BOOST_CHECK(b2->size() == 2);
    BOOST_CHECK(f12->next());
}

BOOST_AUTO_TEST_CASE (BlockSet_expand_blocks_by_fragments_batch_1) {
    using namespace bloomrepeats;
    SequencePtr s1 = boost::make_shared<InMemorySequence>("tGGtccgagcgGAcggcc");
    SequencePtr s2 = boost::make_shared<InMemorySequence>("tGGtccgagcggacggcc");
    Block* b1 = new Block();
    Fragment* f11 = new Fragment(s1, 1, 2);
    Fragment* f12 = new Fragment(s2, 1, 2);
    b1->insert(f11);
    b1->insert(f12);
    Block* b2 = new Block();
    Fragment* f21 = new Fragment(s1, 11, 12);
    b2->insert(f21);
    BlockSetPtr block_set = boost::make_shared<BlockSet>();
    block_set->insert(b1);
    block_set->insert(b2);
    block_set->connect_fragments();
    BOOST_CHECK(block_set->expand_blocks_by_fragments(0, /* batch */ 1));
    BOOST_CHECK(!b2->expand_by_fragments());
    BOOST_CHECK(b2->size() == 2);
    BOOST_CHECK(f12->next());
}

BOOST_AUTO_TEST_CASE (BlockSet_rest) {
    using namespace bloomrepeats;
    SequencePtr s1 = boost::make_shared<InMemorySequence>("tGGtccgagcgGAcggcc");
    SequencePtr s2 = boost::make_shared<InMemorySequence>("tGGtccgagcggacggcc");
    Block* b1 = new Block();
    Fragment* f11 = new Fragment(s1, 1, 2);
    Fragment* f12 = new Fragment(s2, 1, 2);
    b1->insert(f11);
    b1->insert(f12);
    Block* b2 = new Block();
    Fragment* f21 = new Fragment(s1, 11, 12);
    b2->insert(f21);
    BlockSetPtr block_set = boost::make_shared<BlockSet>();
    block_set->insert(b1);
    block_set->insert(b2);
    block_set->connect_fragments();
    BlockSetPtr rest = block_set->rest();
    BOOST_CHECK(rest->size() == 5);
    Filter filter;
    filter.set_min_block_size(1);
    filter.set_min_fragment_length(2);
    filter.apply(rest);
    BOOST_CHECK(rest->size() == 3);
    filter.set_min_fragment_length(6);
    filter.apply(rest);
    BOOST_CHECK(rest->size() == 2);
    filter.set_min_fragment_length(8);
    filter.apply(rest);
    BOOST_CHECK(rest->size() == 2);
    filter.set_min_fragment_length(9);
    filter.apply(rest);
    BOOST_CHECK(rest->size() == 1);
}

