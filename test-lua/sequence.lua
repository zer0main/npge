assert(Sequence.to_atgcn("AT-G-C-A#T$N .C") == "ATGCATNC")

seqs = {Sequence.new(),
    Sequence.new(Sequence.ASIS_SEQUENCE),
    Sequence.new(Sequence.COMPACT_SEQUENCE),
    Sequence.new(Sequence.COMPACT_LOW_N_SEQUENCE),
}
for _, s in pairs(seqs) do
    s:push_back("")
    s:set_name("TEST&chr1&c")
    assert(s:name() == "TEST&chr1&c")
    assert(s:genome() == "TEST")
    assert(s:chromosome() == "chr1")
    assert(s:circular() == true)
    s:set_name("TEST&chr1&l")
    assert(s:circular() == false)
    assert(s:size() == 0)
    assert(s:contents() == "")
    s:push_back("ATGC")
    assert(s:size() == 4)
    assert(s:contents() == "ATGC")
    assert(s:char_at(0) == "A")
    assert(s:char_at(1) == "T")
    s:push_back("ATNC")
    s:push_back("")
    assert(s:size() == 8)
    assert(s:contents() == "ATGCATNC")
    assert(s:char_at(1) == "T")
    assert(s:char_at(4) == "A")
    assert(s:char_at(6) == "N")
    assert(s:char_at(7) == "C")
    assert(s:description() == "")
    assert(s:ac() == "")
    s:set_description("ac=T12345 Test t. chr. 1")
    assert(s:description() == "ac=T12345 Test t. chr. 1")
    assert(s:ac() == "T12345")
    assert(s:substr(0, 8, 1) == "ATGCATNC")
    assert(s:substr(1, 7, 1) == "TGCATNC")
    assert(s:substr(0, 7, 1) == "ATGCATN")
    assert(s:substr(0, 0, 1) == "")
    assert(s:substr(4, 0, 1) == "")
    assert(s:substr(4, 0, -1) == "")
    assert(s:substr(4, 1, -1) == "T")
    assert(s:substr(7, 8, -1) == complement_str("ATGCATNC"))
    assert(s:substr(7, 0, -1) == "")
    assert(s:hash(0, 8, 1) == make_hash("ATGCATNC"))
    assert(s:hash(7, 8, -1) == make_hash("ATGCATNC", -1))
end

local s = DummySequence.new()
assert(s:size() == 0)
s:set_letter('A')
assert(s:letter() == 'A')
assert(s:size() == 0)
s:set_size(5)
assert(s:size() == 5)
assert(s:contents() == "AAAAA")

s = DummySequence.new('T')
assert(s:size() == 0)
assert(s:letter() == 'T')
s:set_size(5)
assert(s:size() == 5)
assert(s:contents() == "TTTTT")

s = DummySequence.new('G', 5)
assert(s:size() == 5)
assert(s:letter() == 'G')
assert(s:contents() == "GGGGG")

local f = Fragment.new(s, 0, 4)
local df = Fragment.deleter(f)
local fs = FragmentSequence.new()
fs:set_fragment(f)
assert(fs:size() == 5)
assert(fs:contents() == "GGGGG")
assert(fs:fragment() == f)
fs = FragmentSequence.new(f)
assert(fs:size() == 5)
assert(fs:contents() == "GGGGG")
assert(fs:fragment() == f)

