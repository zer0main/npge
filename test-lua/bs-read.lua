bs = BlockSet.new()
bs:read(">a\nATGC\n>a_1_1 block=b1 norow")
seqs = bs:seqs()
ss = seqs:iter()
s1 = ss()
blocks = bs:blocks()
b1 = blocks()
f1 = b1:front()
assert(f1:length() == 1)

