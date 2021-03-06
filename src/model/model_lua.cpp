/*
 * NPG-explorer, Nucleotide PanGenome explorer
 * Copyright (C) 2012-2016 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#include <sstream>
#include <boost/make_shared.hpp>

#include "luabind-error.hpp"
#include "luabind-format-signature.hpp"
#include <luabind/luabind.hpp>
#include <luabind/operator.hpp>
#include <luabind/iterator_policy.hpp>
#include <luabind/out_value_policy.hpp>
#include <luabind/adopt_policy.hpp>
#include <luabind/tag_function.hpp>

#include "model_lua.hpp"
#include "util_lua.hpp"
#include "Sequence.hpp"
#include "Fragment.hpp"
#include "AlignmentRow.hpp"
#include "Block.hpp"
#include "BlockSet.hpp"
#include "FragmentCollection.hpp"
#include "block_set_alignment.hpp"
#include "block_stat.hpp"
#include "block_hash.hpp"
#include "convert_position.hpp"
#include "throw_assert.hpp"

namespace luabind {

template<typename T>
int v_compute_score(lua_State* L, int index) {
    if (lua_type(L, index) != LUA_TTABLE) {
        return -1;
    }
    int type = lua_type(L, -1);
    if (type == LUA_TNIL) {
        return 0;
    }
    // check type of first element
    lua_rawgeti(L, index, 1);
    object o(from_stack(L, -1));
    if (!object_cast_nothrow<T>(o)) {
        return -1;
    }
    lua_pop(L, 1);
    return 0;
}

template<typename T>
std::vector<T> v_from(lua_State* L, int index) {
    ASSERT_EQ(lua_type(L, index), LUA_TTABLE);
    std::vector<T> result;
    lua_pushnil(L);
    if (index < 0) {
        index -= 1;
    }
    while (lua_next(L, index) != 0) {
        luabind::object o(from_stack(L, -1));
        result.push_back(object_cast<T>(o));
        lua_pop(L, 1);
    }
    return result;
}

template<typename T>
void v_to(lua_State* L, const std::vector<T>& a) {
    lua_createtable(L, a.size(), 0);
    for (int i = 0; i < a.size(); i++) {
        T v = a[i];
        luabind::object o(L, v);
        o.push(L);
        lua_rawseti(L, -2, i + 1);
    }
}

typedef default_converter<Fragments> dcF;

int dcF::compute_score(lua_State* L, int index) {
    return v_compute_score<npge::Fragment*>(L, index);
}

Fragments dcF::from(lua_State* L, int index) {
    return v_from<npge::Fragment*>(L, index);
}

void dcF::to(lua_State* L, const Fragments& a) {
    v_to<npge::Fragment*>(L, a);
}

typedef default_converter<Blocks> dcB;

int dcB::compute_score(lua_State* L, int index) {
    return v_compute_score<npge::Block*>(L, index);
}

Blocks dcB::from(lua_State* L, int index) {
    return v_from<npge::Block*>(L, index);
}

void dcB::to(lua_State* L, const Blocks& a) {
    v_to<npge::Block*>(L, a);
}

typedef default_converter<Sequences> dcS;

int dcS::compute_score(lua_State* L, int index) {
    return v_compute_score<npge::SequencePtr>(L, index);
}

Sequences dcS::from(lua_State* L, int index) {
    return v_from<npge::SequencePtr>(L, index);
}

void dcS::to(lua_State* L, const Sequences& a) {
    v_to<npge::SequencePtr>(L, a);
}

}

namespace npge {

static SequencePtr new_sequence0(SequenceType type) {
    return Sequence::new_sequence(type);
}

static SequencePtr new_sequence1() {
    return new_sequence0(COMPACT_SEQUENCE);
}

static SequencePtr new_sequence2(const std::string& text,
                                 SequenceType type) {
    SequencePtr s = Sequence::new_sequence(type);
    s->push_back(text);
    return s;
}

static SequencePtr new_sequence3(const std::string& text) {
    SequencePtr s = Sequence::new_sequence(COMPACT_SEQUENCE);
    s->push_back(text);
    return s;
}

static std::string sequence_to_atgcn(const std::string& text) {
    std::string copy = text;
    Sequence::to_atgcn(copy);
    return copy;
}

static std::string sequence_char_at(const Sequence* s,
                                    pos_t p) {
    return std::string(1, s->char_at(p));
}

static std::string sequence_hash(
    const Sequence* s, pos_t start, pos_t length, pos_t ori) {
    return TO_S(s->hash(start, length, ori));
}

static Fragment* new_fragment0() {
    return new Fragment;
}

static Fragment* new_fragment1(Sequence* seq) {
    return new Fragment(seq);
}

static Fragment* new_fragment2(Sequence* seq,
                               pos_t min_pos,
                               pos_t max_pos) {
    return new Fragment(seq, min_pos, max_pos);
}

static Fragment* new_fragment3(Sequence* seq, pos_t min_pos,
                               pos_t max_pos, int ori) {
    return new Fragment(seq, min_pos, max_pos, ori);
}

static void delete_fragment(Fragment* f) {
    delete f;
}

static void fragment_inverse(Fragment* f) {
    f->inverse();
}

static void fragment_set_ori(Fragment* f, int ori) {
    f->set_ori(ori);
}

static std::string fragment_str(Fragment* f) {
    return f->str(0);
}

static std::string fragment_raw_at(const Fragment* f,
                                   pos_t p) {
    return std::string(1, f->raw_at(p));
}

static std::string fragment_at(const Fragment* f,
                               pos_t p) {
    return std::string(1, f->at(p));
}

static Fragment* fragment_common_fragment(
    const Fragment* a,
    const Fragment* b) {
    return a->common_fragment(*b).clone();
}

static std::string fragment_header(Fragment* f) {
    std::stringstream ss;
    f->print_header(ss);
    return ss.str();
}

static std::string fragment_header2(Fragment* f,
                                    const Block* block) {
    std::stringstream ss;
    f->print_header(ss, block);
    return ss.str();
}

static std::string fragment_contents(Fragment* f) {
    std::stringstream ss;
    f->print_contents(ss, '-', 0);
    return ss.str();
}

static AlignmentRow* alignmentrow_new_row() {
    return new CompactAlignmentRow;
}

static AlignmentRow* alignmentrow_new_row1(
    const std::string& str) {
    CompactAlignmentRow* row = new CompactAlignmentRow;
    row->grow(str);
    return row;
}

static AlignmentRow* alignmentrow_new_row2(
    const std::string& str, RowType type) {
    AlignmentRow* row = AlignmentRow::new_row(type);
    row->grow(str);
    return row;
}

static void alignmentrow_delete_row(AlignmentRow* row) {
    delete row;
}

static Block* new_block0() {
    return new Block;
}

static Block* new_block1(const std::string& name) {
    return new Block(name);
}

static void delete_block(Block* block) {
    delete block;
}

static const Block& block_iter_fragments(const Block* block) {
    return *block;
}

static Fragments block_fragments(const Block* block) {
    return Fragments(block->begin(), block->end());
}

static char block_consensus_char(const Block* block, pos_t pos) {
    return block->consensus_char(pos);
}

static std::string block_consensus_string(const Block* block) {
    return block->consensus_string();
}

static void block_inverse(Block* block) {
    block->inverse();
}

static int alignmentstat_letter_count(
    const AlignmentStat& stat,
    const std::string& letter) {
    ASSERT_EQ(letter.size(), 1);
    return stat.letter_count(letter[0]);
}

static void make_stat0(AlignmentStat& stat,
                       const Block* block) {
    make_stat(stat, block, 0, -1);
}

static Decimal alignmentstat_identity(
    const AlignmentStat& stat) {
    return block_identity(stat);
}

static Decimal block_identity0(
    int ident_nogap, int ident_gap,
    int noident_nogap, int noident_gap) {
    return block_identity(ident_nogap, ident_gap,
                          noident_nogap, noident_gap);
}

static void lite_test_column(const Block* block, pos_t column,
                             bool& ident, bool& gap) {
    test_column(block, column, ident, gap);
}

static std::string block_hash_str(const Block* block) {
    return TO_S(block_hash(block));
}

static const BlockSet& iter_blocks(const BlockSet& bs) {
    return bs;
}

static Blocks blockset_blocks(const BlockSet& bs) {
    return Blocks(bs.begin(), bs.end());
}

static void blockset_read(BlockSet& bs,
                          const std::string& text) {
    std::stringstream ss(text);
    ss >> bs;
}

static std::string blockset_hash1(BlockSet& bs) {
    return TO_S(blockset_hash(bs));
}

static BSA& blockset_bsa(BlockSet& bs, const std::string& k) {
    return bs.bsa(k);
}

static bool bsa_has(const BSA& bsa, Sequence* seq) {
    return bsa.find(seq) != bsa.end();
}

static BSRow& bsa_get(BSA& bsa, Sequence* seq) {
    return bsa[seq];
}

static void bsa_erase(BSA& bsa, Sequence* seq) {
    bsa.erase(seq);
}

static luabind::scope register_sequence() {
    using namespace luabind;
    return class_<Sequence, SequencePtr>("Sequence")
           .enum_("Type") [
               value("ASIS_SEQUENCE", ASIS_SEQUENCE),
               value("COMPACT_SEQUENCE", COMPACT_SEQUENCE),
               value("COMPACT_LOW_N_SEQUENCE",
                     COMPACT_LOW_N_SEQUENCE)
           ]
           .scope [
               def("new", &new_sequence0),
               def("new", &new_sequence1),
               def("new", &new_sequence2),
               def("new", &new_sequence3),
               def("to_atgcn", &sequence_to_atgcn)
           ]
           .def("push_back", &Sequence::push_back)
           .def("size", &Sequence::size)
           .def("set_size", &Sequence::set_size)
           .def("contents", &Sequence::contents)
           .def("name", &Sequence::name)
           .def("set_name", &Sequence::set_name)
           .def("description", &Sequence::description)
           .def("set_description", &Sequence::set_description)
           .def("genome", &Sequence::genome)
           .def("chromosome", &Sequence::chromosome)
           .def("circular", &Sequence::circular)
           .def("ac", &Sequence::ac)
           .def("char_at", &sequence_char_at)
           .def("substr", &Sequence::substr)
           .def("hash", &sequence_hash)
           .def(tostring(self))
          ;
}

typedef boost::shared_ptr<DummySequence> DummySequencePtr;

static DummySequencePtr new_dummy_sequence0() {
    return boost::make_shared<DummySequence>();
}

static DummySequencePtr new_dummy_sequence1(std::string c) {
    ASSERT_EQ(c.size(), 1);
    return boost::make_shared<DummySequence>(c[0]);
}

static DummySequencePtr new_dummy_sequence2(
    std::string c, pos_t length) {
    ASSERT_EQ(c.size(), 1);
    return boost::make_shared<DummySequence>(c[0], length);
}

static std::string dummy_sequence_letter(DummySequence* s) {
    return std::string(1, s->letter());
}

static void dummy_sequence_set_letter(DummySequence* s,
                                      std::string letter) {
    ASSERT_EQ(letter.size(), 1);
    return s->set_letter(letter[0]);
}

static luabind::scope register_dummy_sequence() {
    using namespace luabind;
    return class_<DummySequence, Sequence, DummySequencePtr>
           ("DummySequence")
           .scope [
               def("new", &new_dummy_sequence0),
               def("new", &new_dummy_sequence1),
               def("new", &new_dummy_sequence2)
           ]
           .def("letter", &dummy_sequence_letter)
           .def("set_letter", &dummy_sequence_set_letter)
          ;
}

typedef boost::shared_ptr<FragmentSequence> FragmentSeqPtr;

static FragmentSeqPtr new_fragment_sequence0() {
    return boost::make_shared<FragmentSequence>();
}

static FragmentSeqPtr new_fragment_sequence1(Fragment* f) {
    return boost::make_shared<FragmentSequence>(f);
}

static luabind::scope register_fragment_sequence() {
    using namespace luabind;
    return class_ < FragmentSequence, Sequence,
           FragmentSeqPtr > ("FragmentSequence")
           .scope [
               def("new", &new_fragment_sequence0),
               def("new", &new_fragment_sequence1)
           ]
           .def("fragment", &FragmentSequence::fragment)
           .def("set_fragment",
                &FragmentSequence::set_fragment)
          ;
}

typedef boost::shared_ptr<Fragment> FragmentSharedPtr;

static FragmentSharedPtr get_fragment_deleter(Fragment* f) {
    return FragmentSharedPtr(f);
}

static SequencePtr fragment_seq(const Fragment* f) {
    if (!f->seq()) {
        return SequencePtr();
    } else {
        return f->seq()->shared_from_this();
    }
}

static int block_length(const Fragment* f) {
    if (f->block()) {
        return f->block()->alignment_length();
    } else {
        return f->alignment_length();
    }
}

static pos_t block_pos1(const Fragment* f, int pos) {
    return block_pos(f, pos, block_length(f));
}

static pos_t fragment_pos1(const Fragment* f, int pos) {
    return fragment_pos(f, pos, block_length(f));
}

static luabind::scope register_fragment() {
    using namespace luabind;
    return class_<FragmentSharedPtr>("FragmentSharedPtr"),
           class_<Fragment>("Fragment")
           .scope [
               def("new", &new_fragment0),
               def("new", &new_fragment1),
               def("new", &new_fragment2),
               def("new", &new_fragment3),
               def("delete", &delete_fragment),
               def("deleter", &get_fragment_deleter)
           ]
           .def("seq", &fragment_seq)
           .def("block", &Fragment::block)
           .def("min_pos", &Fragment::min_pos)
           .def("set_min_pos", &Fragment::set_min_pos)
           .def("max_pos", &Fragment::max_pos)
           .def("set_max_pos", &Fragment::set_max_pos)
           .def("ori", &Fragment::ori)
           .def("set_ori", &Fragment::set_ori)
           .def("set_ori", &fragment_set_ori)
           .def("length", &Fragment::length)
           .def("alignment_length",
                &Fragment::alignment_length)
           .def("inverse", &Fragment::inverse)
           .def("inverse", &fragment_inverse)
           .def("begin_pos", &Fragment::begin_pos)
           .def("last_pos", &Fragment::last_pos)
           .def("set_begin_last", &Fragment::set_begin_last)
           .def("end_pos", &Fragment::end_pos)
           .def("str", &Fragment::str)
           .def("str", &fragment_str)
           .def("substr", &Fragment::substr)
           .def("subfragment", &Fragment::subfragment)
           .def("clone", &Fragment::clone)
           .def("id", &Fragment::id)
           .def("hash", &Fragment::hash)
           .def("valid", &Fragment::valid)
           .def("has", &Fragment::has)
           .def("raw_at", &fragment_raw_at)
           .def("at", &fragment_at)
           .def("alignment_at", &Fragment::alignment_at)
           .def("common_positions",
                &Fragment::common_positions)
           .def("common_fragment", &fragment_common_fragment)
           .def("dist_to", &Fragment::dist_to)
           .def("is_subfragment_of",
                &Fragment::is_subfragment_of)
           .def("is_internal_subfragment_of",
                &Fragment::is_internal_subfragment_of)
           .def("row", &Fragment::row)
           .def("detach_row", &Fragment::detach_row)
           .def("set_row", &Fragment::set_row)
           .def("header", &fragment_header)
           .def("header", &fragment_header2)
           .def("contents", &fragment_contents)
           .def(tostring(self))
           .def(const_self == const_self)
           .def(const_self < const_self)
           .def("block_pos", &block_pos)
           .def("block_pos", &block_pos1)
           .def("fragment_pos", &fragment_pos)
           .def("fragment_pos", &fragment_pos1)
           .def("frag_to_seq", &frag_to_seq)
           .def("seq_to_frag", &seq_to_frag)
          ;
}

typedef boost::shared_ptr<AlignmentRow> RowSharedPtr;

static RowSharedPtr get_row_deleter(AlignmentRow* row) {
    return RowSharedPtr(row);
}

static luabind::scope register_alignment_row() {
    using namespace luabind;
    return class_<RowSharedPtr>("RowSharedPtr"),
           class_<AlignmentRow>("AlignmentRow")
           .enum_("Type") [
               value("MAP_ROW", MAP_ROW),
               value("COMPACT_ROW", COMPACT_ROW)
           ]
           .scope [
               def("new", &AlignmentRow::new_row),
               def("new", &alignmentrow_new_row),
               def("new", &alignmentrow_new_row1),
               def("new", &alignmentrow_new_row2),
               def("delete", &alignmentrow_delete_row),
               def("deleter", &get_row_deleter)
           ]
           .def("clear", &AlignmentRow::clear)
           .def("grow", &AlignmentRow::grow)
           .def("bind", &AlignmentRow::bind)
           .def("map_to_alignment",
                &AlignmentRow::map_to_alignment)
           .def("map_to_fragment",
                &AlignmentRow::map_to_fragment)
           .def("length", &AlignmentRow::length)
           .def("set_length", &AlignmentRow::set_length)
           .def("fragment", &AlignmentRow::fragment)
           .def("nearest_in_fragment",
                &AlignmentRow::nearest_in_fragment)
           .def("clone", &AlignmentRow::clone)
           .def("slice", &AlignmentRow::slice)
           .def("type", &AlignmentRow::type)
          ;
}

typedef boost::shared_ptr<Block> BlockSharedPtr;

static BlockSharedPtr get_block_deleter(Block* block) {
    return BlockSharedPtr(block);
}

static luabind::scope register_block() {
    using namespace luabind;
    return class_<BlockSharedPtr>("BlockSharedPtr"),
           class_<Block>("Block")
           .scope [
               def("new", &new_block0),
               def("new", &new_block1),
               def("delete", &delete_block),
               def("deleter", &get_block_deleter)
           ]
           .def("insert", &Block::insert)
           .def("erase", &Block::erase)
           .def("detach", &Block::detach)
           .def("size", &Block::size)
           .def("empty", &Block::empty)
           .def("has", &Block::has)
           .def("clear", &Block::clear)
           .def("swap", &Block::swap)
           .def("front", &Block::front)
           .def("fragments", &block_fragments)
           .def("iter_fragments", &block_iter_fragments,
                return_stl_iterator)
           .def("alignment_length", &Block::alignment_length)
           .def("identity", &Block::identity)
           .def("consensus_char", &block_consensus_char)
           .def("consensus_string", &block_consensus_string)
           .def("match", &Block::match)
           .def("inverse", &Block::inverse)
           .def("inverse", &block_inverse)
           .def("slice", &Block::slice)
           .def("clone", &Block::clone)
           .def("remove_alignment", &Block::remove_alignment)
           .def("common_positions", &Block::common_positions)
           .def("merge", &Block::merge)
           .def("name", &Block::name)
           .def("set_name", &Block::set_name)
           .def("set_canonical_name", &set_canonical_name)
           .def("set_random_name", &Block::set_random_name)
           .def("set_name_from_fragments",
                &Block::set_name_from_fragments)
           .def("weak", &Block::weak)
           .def("set_weak", &Block::set_weak)
           .def(tostring(self))
           .def("is_ident_nogap", &is_ident_nogap)
           .def("test_column", &lite_test_column,
                pure_out_value(_3) + pure_out_value(_4))
           .def("hash", &block_hash_str)
           .def("id", &block_id)
           .def("has_repeats", &has_repeats)
           .def("is_exact_stem", &is_exact_stem)
           .def("make_name", &block_name)
           .def("has_alignment", &has_alignment)
           .def("test_block", &test_block)
           .def("find_slice", &find_slice,
                pure_out_value(_1) + pure_out_value(_2))
           .def(const_self == const_self)
          ;
}

static luabind::scope register_alignment_stat() {
    using namespace luabind;
    return class_<AlignmentStat>("AlignmentStat")
           .def(constructor<>())
           .def("ident_nogap", &AlignmentStat::ident_nogap)
           .def("ident_gap", &AlignmentStat::ident_gap)
           .def("noident_nogap", &AlignmentStat::noident_nogap)
           .def("noident_gap", &AlignmentStat::noident_gap)
           .def("pure_gap", &AlignmentStat::pure_gap)
           .def("total", &AlignmentStat::total)
           .def("spreading", &AlignmentStat::spreading)
           .def("alignment_rows",
                &AlignmentStat::alignment_rows)
           .def("min_fragment_length",
                &AlignmentStat::min_fragment_length)
           .def("letter_count", &alignmentstat_letter_count)
           .def("gc", &AlignmentStat::gc)
           .def("make_stat", &make_stat)
           .def("make_stat", &make_stat0)
           .def("block_identity", &alignmentstat_identity)
          ;
}

static luabind::scope register_block_set() {
    using namespace luabind;
    return class_<BlockSet, BlockSetPtr>("BlockSet")
           .scope [
               def("new", &new_bs)
           ]
           .def("add_sequence", &BlockSet::add_sequence)
           .def("add_sequences", &BlockSet::add_sequences)
           .def("seqs", &BlockSet::seqs)
           // FIXME ^^
           // TODO find_seq(name)
           .def("remove_sequence", &BlockSet::remove_sequence)
           .def("insert", &BlockSet::insert)
           .def("erase", &BlockSet::erase)
           .def("detach", &BlockSet::detach)
           .def("size", &BlockSet::size)
           .def("empty", &BlockSet::empty)
           .def("has", &BlockSet::has)
           .def("clear_blocks", &BlockSet::clear_blocks)
           .def("clear_seqs", &BlockSet::clear_seqs)
           .def("clear", &BlockSet::clear)
           .def("swap", &BlockSet::swap)
           .def("clone", &BlockSet::clone)
           .def("blocks", &blockset_blocks)
           .def("iter_blocks", &iter_blocks,
                return_stl_iterator)
           .def("find_block", &BlockSet::find_block)
           .def(tostring(self))
           .def("read", &blockset_read)
           .def("genomes_number", &genomes_number)
           .def("genomes_list", &genomes_list)
           .def("hash", &blockset_hash1)
           .def("bsa", &blockset_bsa)
           .def("bsas", &BlockSet::bsas)
           .def("has_bsa", &BlockSet::has_bsa)
           .def("remove_bsa", &BlockSet::remove_bsa)
           .def("clear_bsas", &BlockSet::clear_bsas)
           .def(const_self == const_self)
          ;
}

static luabind::scope register_bsrow() {
    using namespace luabind;
    return class_<BSRow>("BSRow")
           .def(constructor<>())
           .def_readwrite("ori", &BSRow::ori)
           .def_readwrite("fragments", &BSRow::fragments)
          ;
}

static luabind::scope register_bsa() {
    using namespace luabind;
    return class_<BSA>("BSA")
           .def(constructor<>())
           .def("empty", &BSA::empty)
           .def("clear", &BSA::clear)
           .def("size", &BSA::size)
           .def("has", &bsa_has)
           .def("get", &bsa_get)
           .def("erase", &bsa_erase)
           .def("length", &bsa_length)
           .def("is_circular", &bsa_is_circular)
          ;
}

template<typename T>
struct find_overlap_fragments {
    Fragments operator()(T* fc, Fragment* f) const {
        Fragments result;
        fc->find_overlap_fragments(result, f);
        return result;
    }
};

template<typename T>
luabind::scope register_fragment_collection(const char* name) {
    using namespace luabind;
    return class_<T, boost::shared_ptr<T> >(name)
           .def(constructor<>())
           .def("cycles_allowed", &T::cycles_allowed)
           .def("set_cycles_allowed", &T::set_cycles_allowed)
           .def("add_fragment", &T::add_fragment)
           .def("remove_fragment", &T::remove_fragment)
           .def("add_block", &T::add_block)
           .def("remove_block", &T::remove_block)
           .def("add_bs", &T::add_bs)
           .def("remove_bs", &T::remove_bs)
           .def("prepare", &T::prepare)
           .def("clear", &T::clear)
           .def("has_overlap", &T::has_overlap)
           .def("block_has_overlap", &T::block_has_overlap)
           .def("bs_has_overlap", &T::bs_has_overlap)
           .def("find_overlap_fragments",
                tag_function<Fragments(T*, Fragment*)>(
                    find_overlap_fragments<T>()))
           .def("next", &T::next)
           .def("prev", &T::prev)
           .def("neighbor", &T::neighbor)
           .def("logical_neighbor", &T::logical_neighbor)
           .def("are_neighbors", &T::are_neighbors)
           .def("another_neighbor", &T::another_neighbor)
           .def("has_seq", &T::has_seq)
           // TODO .def("seqs", &T::seqs)
           // TODO find_overlaps
          ;
}

}

extern "C" int init_model_lua(lua_State* L) {
    using namespace luabind;
    using namespace npge;
    open(L);
    module(L) [
        register_sequence(),
        register_dummy_sequence(),
        register_fragment_sequence(),
        register_fragment(),
        register_alignment_row(),
        register_block(),
        register_alignment_stat(),
        register_block_set(),
        register_bsrow(),
        register_bsa(),
        register_fragment_collection<SetFc>("SetFc"),
        register_fragment_collection<VectorFc>("VectorFc"),
        def("block_identity", &block_identity0),
        def("strict_block_identity", &strict_block_identity)
    ];
    return 0;
}

