// Type definitions for CP

export fn rotr16(x: u32): u32;
export fn rotr12(x: u32): u32;
export fn rotr8(x: u32): u32;
export fn rotr7(x: u32): u32;
export fn readU32Le(buf: &[]u8, offset: usize): u32;
export fn writeU32Le(buf: []u8, offset: usize, v: u32);
export fn wordsFromLeBytes32(bytes: [32]u8): [8]u32;
export fn wordsFromLeBytes64(bytes: [64]u8): [16]u32;
export fn gMix(state: &mut [16]u32, a: usize, b: usize, c: usize, d: usize, mx: u32, my: u32);
export fn roundMix(state: &mut [16]u32, m: [16]u32);
export fn permuteWords(m: [16]u32): [16]u32;
export fn compress(chaining_value: [8]u32, block_words_in: [16]u32, counter: u64, block_len: u32, flags: u32): [16]u32;
export fn first8Words(compression_output: [16]u32): [8]u32;
export class OutputNode {
    input_chaining_value: [8]u32,
    block_words: [16]u32,
    counter: u64,
    block_len: u32,
    flags: u32,
    fn chainingValue(): [8]u32;
    fn rootOutputBytes(out: []u8);
}

export class ChunkState {
    chaining_value: [8]u32,
    chunk_counter: u64,
    block: [64]u8,
    block_len: u8,
    blocks_compressed: u8,
    flags: u32,
    fn init(key_words: [8]u32, chunk_counter: u64, flags: u32): ChunkState;
    fn len(): usize;
    fn startFlag(): u32;
    fn update(_: auto, input: &[]u8);
    fn output(): OutputNode;
}

export fn parentOutput(left_child_cv: [8]u32, right_child_cv: [8]u32, key_words: [8]u32, flags: u32): OutputNode;
export fn parentCv(left_child_cv: [8]u32, right_child_cv: [8]u32, key_words: [8]u32, flags: u32): [8]u32;
export class Blake3Hasher {
    chunk_state: ChunkState,
    key_words: [8]u32,
    cv_stack: [54][8]u32,
    cv_stack_len: u8,
    flags: u32,
    fn initInternal(key_words: [8]u32, flags: u32): Blake3Hasher;
    fn init(): Blake3Hasher;
    fn initKeyed(key: [32]u8): Blake3Hasher;
    fn initDeriveKey(context: string): Blake3Hasher;
    fn pushStack(_: auto, cv: [8]u32);
    fn popStack(_: auto): [8]u32;
    fn addChunkChainingValue(_: auto, new_cv_in: [8]u32, total_chunks_in: u64);
    fn update(_: auto, input: &[]u8);
    fn finalizeXof(out: []u8);
    fn finalize(): [32]u8;
    fn reset(_: auto);
}

export fn hash(data: &[]u8): [32]u8;
export fn keyedHash(key: [32]u8, data: &[]u8): [32]u8;
export fn deriveKey(context: string, key_material: &[]u8): [32]u8;
export fn hashEql(a: [32]u8, b: [32]u8): bool;
export fn toHex(digest: [32]u8, out: []u8): &[]u8;
