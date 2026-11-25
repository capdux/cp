// @cap/blake3 — Pure CP, zero-dependency BLAKE3 cryptographic hash implementation
// Strictly follows the official BLAKE3 specification & Rust `blake3` reference crate.
// Supports:
//   - Standard hashing (`hash`)
//   - Keyed hashing / MAC (`keyedHash`)
//   - Key derivation (`deriveKey`)
//   - Streaming incremental hashing (`Blake3Hasher`: `init`, `initKeyed`, `initDeriveKey`, `update`, `finalize`, `finalizeXof`, `reset`)
//   - Arbitrary-length input via full Merkle tree CV stack (54 levels, up to 2^64 bytes)
//   - Extendable-output function (XOF) via `finalizeXof`
//   - Constant-time digest comparison (`hashEql`) & lowercase hex encoding (`toHex`)

pub const OUT_LEN: usize = 32;
pub const KEY_LEN: usize = 32;
pub const BLOCK_LEN: usize = 64;
pub const CHUNK_LEN: usize = 1024;

const CHUNK_START: u32 = 1;
const CHUNK_END: u32 = 2;
const PARENT: u32 = 4;
const ROOT: u32 = 8;
const KEYED_HASH: u32 = 16;
const DERIVE_KEY_CONTEXT: u32 = 32;
const DERIVE_KEY_MATERIAL: u32 = 64;

const IV: [8]u32 = [
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
    0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19,
];

const MSG_PERMUTATION: [16]usize = [
    2, 6, 3, 10, 7, 0, 4, 13, 1, 11, 12, 5, 9, 14, 15, 8,
];

const HEX_DIGITS: [16]u8 = [
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
];

fn rotr16(x: u32): u32 {
    return (x >> 16) | (x << 16);
}

fn rotr12(x: u32): u32 {
    return (x >> 12) | (x << 20);
}

fn rotr8(x: u32): u32 {
    return (x >> 8) | (x << 24);
}

fn rotr7(x: u32): u32 {
    return (x >> 7) | (x << 25);
}

fn readU32Le(buf: []const u8, offset: usize): u32 {
    const b0: u32 = u32(buf[offset]);
    const b1: u32 = u32(buf[offset + 1]);
    const b2: u32 = u32(buf[offset + 2]);
    const b3: u32 = u32(buf[offset + 3]);
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn writeU32Le(buf: []u8, offset: usize, v: u32) {
    buf[offset] = (v & 0xFF) as u8;
    buf[offset + 1] = ((v >> 8) & 0xFF) as u8;
    buf[offset + 2] = ((v >> 16) & 0xFF) as u8;
    buf[offset + 3] = ((v >> 24) & 0xFF) as u8;
}

fn wordsFromLeBytes32(bytes: [32]u8): [8]u32 {
    let words: [8]u32 = [0; 8];
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        words[i] = readU32Le(bytes[0..], i * 4);
    }
    return words;
}

fn wordsFromLeBytes64(bytes: [64]u8): [16]u32 {
    let words: [16]u32 = [0; 16];
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        words[i] = readU32Le(bytes[0..], i * 4);
    }
    return words;
}

fn gMix(state: *[16]u32, a: usize, b: usize, c: usize, d: usize, mx: u32, my: u32) {
    state[a] = state[a] +% state[b] +% mx;
    state[d] = rotr16(state[d] ^ state[a]);
    state[c] = state[c] +% state[d];
    state[b] = rotr12(state[b] ^ state[c]);
    state[a] = state[a] +% state[b] +% my;
    state[d] = rotr8(state[d] ^ state[a]);
    state[c] = state[c] +% state[d];
    state[b] = rotr7(state[b] ^ state[c]);
}

fn roundMix(state: *[16]u32, m: [16]u32) {
    // Mix the columns.
    gMix(state, 0, 4, 8, 12, m[0], m[1]);
    gMix(state, 1, 5, 9, 13, m[2], m[3]);
    gMix(state, 2, 6, 10, 14, m[4], m[5]);
    gMix(state, 3, 7, 11, 15, m[6], m[7]);
    // Mix the diagonals.
    gMix(state, 0, 5, 10, 15, m[8], m[9]);
    gMix(state, 1, 6, 11, 12, m[10], m[11]);
    gMix(state, 2, 7, 8, 13, m[12], m[13]);
    gMix(state, 3, 4, 9, 14, m[14], m[15]);
}

fn permuteWords(m: [16]u32): [16]u32 {
    let permuted: [16]u32 = [0; 16];
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        permuted[i] = m[MSG_PERMUTATION[i]];
    }
    return permuted;
}

fn compress(
    chaining_value: [8]u32,
    block_words_in: [16]u32,
    counter: u64,
    block_len: u32,
    flags: u32,
): [16]u32 {
    let state: [16]u32 = [
        chaining_value[0],
        chaining_value[1],
        chaining_value[2],
        chaining_value[3],
        chaining_value[4],
        chaining_value[5],
        chaining_value[6],
        chaining_value[7],
        IV[0],
        IV[1],
        IV[2],
        IV[3],
        (counter & 0xFFFFFFFF) as u32,
        ((counter >> 32) & 0xFFFFFFFF) as u32,
        block_len,
        flags,
    ];

    let block: [16]u32 = block_words_in;

    // 7 rounds with message permutation between rounds.
    roundMix(*state, block); // round 1
    block = permuteWords(block);
    roundMix(*state, block); // round 2
    block = permuteWords(block);
    roundMix(*state, block); // round 3
    block = permuteWords(block);
    roundMix(*state, block); // round 4
    block = permuteWords(block);
    roundMix(*state, block); // round 5
    block = permuteWords(block);
    roundMix(*state, block); // round 6
    block = permuteWords(block);
    roundMix(*state, block); // round 7

    let i: usize = 0;
    while (i < 8) : (i += 1) {
        state[i] ^= state[i + 8];
        state[i + 8] ^= chaining_value[i];
    }
    return state;
}

fn first8Words(compression_output: [16]u32): [8]u32 {
    let out: [8]u32 = [
        compression_output[0],
        compression_output[1],
        compression_output[2],
        compression_output[3],
        compression_output[4],
        compression_output[5],
        compression_output[6],
        compression_output[7],
    ];
    return out;
}

struct OutputNode {
    input_chaining_value: [8]u32;
    block_words: [16]u32;
    counter: u64;
    block_len: u32;
    flags: u32;

    fn chainingValue(): [8]u32 {
        return first8Words(compress(
            _.input_chaining_value,
            _.block_words,
            _.counter,
            _.block_len,
            _.flags,
        ));
    }

    fn rootOutputBytes(out: []u8) {
        let output_block_counter: u64 = 0;
        let offset: usize = 0;
        let word_bytes: [64]u8 = [0; 64];
        while (offset < out.len) {
            const words = compress(
                _.input_chaining_value,
                _.block_words,
                output_block_counter,
                _.block_len,
                _.flags | ROOT,
            );
            let w: usize = 0;
            while (w < 16) : (w += 1) {
                writeU32Le(word_bytes[0..], w * 4, words[w]);
            }
            const remaining = out.len - offset;
            let take: usize = 64;
            if (remaining < 64) {
                take = remaining;
            }
            let k: usize = 0;
            while (k < take) : (k += 1) {
                out[offset + k] = word_bytes[k];
            }
            offset += take;
            output_block_counter += 1;
        }
    }
}

struct ChunkState {
    chaining_value: [8]u32;
    chunk_counter: u64;
    block: [64]u8;
    block_len: u8;
    blocks_compressed: u8;
    flags: u32;

    static fn init(key_words: [8]u32, chunk_counter: u64, flags: u32): ChunkState {
        return ChunkState{
            chaining_value: key_words,
            chunk_counter: chunk_counter,
            block: [0; 64],
            block_len: 0,
            blocks_compressed: 0,
            flags: flags,
        };
    }

    fn len(): usize {
        return usize(_.blocks_compressed) * BLOCK_LEN + usize(_.block_len);
    }

    fn startFlag(): u32 {
        if (_.blocks_compressed == 0) {
            return CHUNK_START;
        }
        return 0;
    }

    fn update(*_, input: []const u8) {
        let pos: usize = 0;
        while (pos < input.len) {
            if (usize(_.block_len) == BLOCK_LEN) {
                const block_words = wordsFromLeBytes64(_.block);
                _.chaining_value = first8Words(compress(
                    _.chaining_value,
                    block_words,
                    _.chunk_counter,
                    u32(BLOCK_LEN),
                    _.flags | _.startFlag(),
                ));
                _.blocks_compressed += 1;
                let z: usize = 0;
                while (z < BLOCK_LEN) : (z += 1) {
                    _.block[z] = 0;
                }
                _.block_len = 0;
            }

            const want = BLOCK_LEN - usize(_.block_len);
            const avail = input.len - pos;
            let take = want;
            if (avail < want) {
                take = avail;
            }
            const dst_off = usize(_.block_len);
            let k: usize = 0;
            while (k < take) : (k += 1) {
                _.block[dst_off + k] = input[pos + k];
            }
            _.block_len += take as u8;
            pos += take;
        }
    }

    fn output(): OutputNode {
        const block_words = wordsFromLeBytes64(_.block);
        return OutputNode{
            input_chaining_value: _.chaining_value,
            block_words: block_words,
            counter: _.chunk_counter,
            block_len: u32(_.block_len),
            flags: _.flags | _.startFlag() | CHUNK_END,
        };
    }
}

fn parentOutput(
    left_child_cv: [8]u32,
    right_child_cv: [8]u32,
    key_words: [8]u32,
    flags: u32,
): OutputNode {
    let block_words: [16]u32 = [0; 16];
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        block_words[i] = left_child_cv[i];
        block_words[8 + i] = right_child_cv[i];
    }
    return OutputNode{
        input_chaining_value: key_words,
        block_words: block_words,
        counter: 0,
        block_len: u32(BLOCK_LEN),
        flags: PARENT | flags,
    };
}

fn parentCv(
    left_child_cv: [8]u32,
    right_child_cv: [8]u32,
    key_words: [8]u32,
    flags: u32,
): [8]u32 {
    return parentOutput(left_child_cv, right_child_cv, key_words, flags).chainingValue();
}

/// Incremental streaming BLAKE3 hasher supporting standard, keyed, key-derivation,
/// and arbitrary-length XOF output modes.
pub struct Blake3Hasher {
    chunk_state: ChunkState;
    key_words: [8]u32;
    cv_stack: [54][8]u32;
    cv_stack_len: u8;
    flags: u32;

    static fn initInternal(key_words: [8]u32, flags: u32): Blake3Hasher {
        return Blake3Hasher{
            chunk_state: ChunkState.init(key_words, 0, flags),
            key_words: key_words,
            cv_stack: [[0; 8]; 54],
            cv_stack_len: 0,
            flags: flags,
        };
    }

    /// Creates a new BLAKE3 hasher in standard hashing mode.
    static fn init(): Blake3Hasher {
        return Blake3Hasher.initInternal(IV, 0);
    }

    /// Creates a new BLAKE3 hasher in keyed hashing (MAC) mode with a 32-byte key.
    static fn initKeyed(key: [32]u8): Blake3Hasher {
        const key_words = wordsFromLeBytes32(key);
        return Blake3Hasher.initInternal(key_words, KEYED_HASH);
    }

    /// Creates a new BLAKE3 hasher in key-derivation mode using the domain-separation `context` string.
    static fn initDeriveKey(context: string): Blake3Hasher {
        let context_hasher = Blake3Hasher.initInternal(IV, DERIVE_KEY_CONTEXT);
        context_hasher.update(context);
        const context_key = context_hasher.finalize();
        const context_key_words = wordsFromLeBytes32(context_key);
        return Blake3Hasher.initInternal(context_key_words, DERIVE_KEY_MATERIAL);
    }

    fn pushStack(*_, cv: [8]u32) {
        _.cv_stack[usize(_.cv_stack_len)] = cv;
        _.cv_stack_len += 1;
    }

    fn popStack(*_): [8]u32 {
        _.cv_stack_len -= 1;
        return _.cv_stack[usize(_.cv_stack_len)];
    }

    fn addChunkChainingValue(*_, new_cv_in: [8]u32, total_chunks_in: u64) {
        let new_cv = new_cv_in;
        let total_chunks = total_chunks_in;
        while ((total_chunks & 1) == 0) {
            const left_cv = _.popStack();
            new_cv = parentCv(left_cv, new_cv, _.key_words, _.flags);
            total_chunks >>= 1;
        }
        _.pushStack(new_cv);
    }

    /// Feeds additional input bytes into the hasher state. Can be called any number of times.
    fn update(*_, input: []const u8) {
        let pos: usize = 0;
        while (pos < input.len) {
            if (_.chunk_state.len() == CHUNK_LEN) {
                const chunk_cv = _.chunk_state.output().chainingValue();
                const total_chunks = _.chunk_state.chunk_counter + 1;
                _.addChunkChainingValue(chunk_cv, total_chunks);
                _.chunk_state = ChunkState.init(_.key_words, total_chunks, _.flags);
            }

            const want = CHUNK_LEN - _.chunk_state.len();
            const avail = input.len - pos;
            let take = want;
            if (avail < want) {
                take = avail;
            }
            _.chunk_state.update(input[pos..pos + take]);
            pos += take;
        }
    }

    /// Writes arbitrary-length XOF output bytes into `out` without mutating the hasher state.
    fn finalizeXof(out: []u8) {
        let output = _.chunk_state.output();
        let parent_nodes_remaining = usize(_.cv_stack_len);
        while (parent_nodes_remaining > 0) {
            parent_nodes_remaining -= 1;
            output = parentOutput(
                _.cv_stack[parent_nodes_remaining],
                output.chainingValue(),
                _.key_words,
                _.flags,
            );
        }
        output.rootOutputBytes(out);
    }

    /// Finalizes and returns the standard 32-byte BLAKE3 digest without mutating the hasher state.
    fn finalize(): [32]u8 {
        let out: [32]u8 = [0; 32];
        _.finalizeXof(out[0..]);
        return out;
    }

    /// Resets the hasher back to its initial empty state (preserving key and mode flags).
    fn reset(*_) {
        _.chunk_state = ChunkState.init(_.key_words, 0, _.flags);
        _.cv_stack_len = 0;
    }
}

/// Computes the standard 32-byte BLAKE3 hash of `data`.
pub fn hash(data: []const u8): [32]u8 {
    let hasher = Blake3Hasher.init();
    hasher.update(data);
    return hasher.finalize();
}

/// Computes the 32-byte keyed BLAKE3 MAC of `data` using the 32-byte `key`.
pub fn keyedHash(key: [32]u8, data: []const u8): [32]u8 {
    let hasher = Blake3Hasher.initKeyed(key);
    hasher.update(data);
    return hasher.finalize();
}

/// Derives a 32-byte key from `context` (domain separation string) and `key_material`.
pub fn deriveKey(context: string, key_material: []const u8): [32]u8 {
    let hasher = Blake3Hasher.initDeriveKey(context);
    hasher.update(key_material);
    return hasher.finalize();
}

/// Constant-time comparison of two 32-byte BLAKE3 digests to prevent timing side-channels.
pub fn hashEql(a: [32]u8, b: [32]u8): bool {
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

/// Encodes a 32-byte digest into lowercase hexadecimal ASCII in `out` (requires at least 64 bytes
/// for the full digest) and returns the written slice.
pub fn toHex(digest: [32]u8, out: []u8): []const u8 {
    let max_bytes: usize = 32;
    const cap_bytes = out.len / 2;
    if (cap_bytes < max_bytes) {
        max_bytes = cap_bytes;
    }
    let i: usize = 0;
    while (i < max_bytes) : (i += 1) {
        const b = digest[i];
        out[i * 2] = HEX_DIGITS[usize((b >> 4) & 0x0F)];
        out[i * 2 + 1] = HEX_DIGITS[usize(b & 0x0F)];
    }
    return out[0..max_bytes * 2];
}
