/*
 * bloomrepeats, Find genomic repeats, using Bloom filter based prefiltration
 * Copyright (C) 2012 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#include <map>
#include <utility>
#include <boost/test/unit_test.hpp>

#include "tree.hpp"

namespace bloomrepeats {

class TestLeaf;

typedef std::pair<const LeafNode*, const LeafNode*> Pair;
typedef std::map<Pair, float> Map;

Map map;

Pair make_pair(const LeafNode* a, const LeafNode* b) {
    if (a < b) {
        return Pair(a, b);
    } else {
        return Pair(b, a);
    }
}

class TestLeaf : public LeafNode {
public:
    TestLeaf(std::string n):
        name_(n)
    { }

    float distance_to_impl(const LeafNode* leaf) const {
        return map[make_pair(this, leaf)];
    }

    std::string name_impl() const {
        return name_;
    }

    AbstractTreeNode* clone_impl() const {
        return new TestLeaf(name_);
    }

private:
    std::string name_;
};

}

bool almost_equal(float a, float b) {
    return -0.0001 < a - b && a - b < 0.0001;
}

BOOST_AUTO_TEST_CASE (tree_upgma) {
    using namespace bloomrepeats;
    Tree tree;
    TestLeaf* a1 = new TestLeaf("a1");
    TestLeaf* a2 = new TestLeaf("a2");
    TestLeaf* a3 = new TestLeaf("a3");
    tree.add_node(a1);
    tree.add_node(a2);
    tree.add_node(a3);
    map[make_pair(a1, a2)] = 2.0;
    map[make_pair(a1, a3)] = 5.0;
    map[make_pair(a2, a3)] = 7.0;
    tree.upgma();
    BOOST_REQUIRE(tree.root());
    BOOST_CHECK(almost_equal(tree.root()->length(), 0.0));
    BranchNode* root = dynamic_cast<BranchNode*>(tree.root());
    BOOST_REQUIRE(root);
    BOOST_REQUIRE(root->left());
    BOOST_REQUIRE(root->right());
    BOOST_CHECK(almost_equal(root->left()->length(), 3.0));
    BOOST_CHECK(almost_equal(root->right()->length(), 3.0));
    BOOST_REQUIRE(dynamic_cast<TestLeaf*>(root->left()) == a3 ||
            dynamic_cast<TestLeaf*>(root->right()) == a3);
    BranchNode* branch12 = (dynamic_cast<TestLeaf*>(root->left()) == a3) ?
            dynamic_cast<BranchNode*>(root->right()) :
            dynamic_cast<BranchNode*>(root->left());
    BOOST_REQUIRE(branch12->left());
    BOOST_REQUIRE(branch12->right());
    TestLeaf* br_left = dynamic_cast<TestLeaf*>(branch12->left());
    TestLeaf* br_right = dynamic_cast<TestLeaf*>(branch12->right());
    BOOST_REQUIRE(br_left);
    BOOST_REQUIRE(br_right);
    BOOST_CHECK(make_pair(br_left, br_right) == make_pair(a1, a2));
    BOOST_CHECK(almost_equal(a1->length(), 1.0));
    BOOST_CHECK(almost_equal(a2->length(), 1.0));
}

BOOST_AUTO_TEST_CASE (tree_distance) {
    using namespace bloomrepeats;
    Tree tree;
    TestLeaf* a1 = new TestLeaf("a1");
    TestLeaf* a2 = new TestLeaf("a2");
    TestLeaf* a3 = new TestLeaf("a3");
    a1->set_length(1);
    a2->set_length(2);
    BranchNode* a12 = new BranchNode;
    a12->set_left(a1);
    a12->set_right(a2);
    a12->set_length(10);
    BranchNode* a123 = new BranchNode;
    a3->set_length(20);
    a123->set_left(a12);
    a123->set_right(a3);
    TestLeaf* a4 = new TestLeaf("a4");
    tree.add_node(a123);
    tree.add_node(a4);
    BOOST_CHECK(almost_equal(a1->tree_distance_to(a2), 3));
    BOOST_CHECK(almost_equal(a2->tree_distance_to(a1), 3));
    BOOST_CHECK(almost_equal(a1->tree_distance_to(a1), 0));
    BOOST_CHECK(almost_equal(a12->tree_distance_to(a12), 0));
    BOOST_CHECK(almost_equal(a1->tree_distance_to(a12), 1));
    BOOST_CHECK(almost_equal(a2->tree_distance_to(a12), 2));
    BOOST_CHECK(almost_equal(a1->tree_distance_to(a3), 31));
    BOOST_CHECK(almost_equal(a1->tree_distance_to(a123), 11));
    BOOST_CHECK(almost_equal(a2->tree_distance_to(a3), 32));
    BOOST_CHECK(almost_equal(a3->tree_distance_to(a123), 20));
    BOOST_CHECK(almost_equal(a3->tree_distance_to(a12), 30));
    BOOST_CHECK(almost_equal(a3->tree_distance_to(a3), 0));
    BOOST_CHECK(almost_equal(a4->tree_distance_to(a4), 0));
    BOOST_CHECK(a1->tree_distance_to(a4) < 0);
    BOOST_CHECK(a2->tree_distance_to(a4) < 0);
    BOOST_CHECK(a3->tree_distance_to(a4) < 0);
    BOOST_CHECK(a123->tree_distance_to(a4) < 0);
}

