// Source: cp/lib/blake3/compress.cp (CP stdlib BLAKE3 compression & Merkle primitives)

pub const OUT_LEN: usize = 32;
pub const KEY_LEN: usize = 32;
pub const BLOCK_LEN: usize = 64;
pub const CHUNK_LEN: usize = 1024;

pub const CHUNK_START: u32 = 1 << 0;
pub const CHUNK_END: u32 = 1 << 1;
pub const PARENT: u32 = 1 << 2;
pub const ROOT: u32 = 1 << 3;
pub const KEYED_HASH: u32 = 1 << 4;
pub const DERIVE_KEY_CONTEXT: u32 = 1 << 5;
pub const DERIVE_KEY_MATERIAL: u32 = 1 << 6;

pub const IV: [8]u32 = [
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
    0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19,
];

const MSG_PERMUTATION: [16]usize = [
    2, 6, 3, 10, 7, 0, 4, 13,
    1, 11, 12, 5, 9, 14, 15, 8,
];

pub fn rotr32(x: u32, n: u5): u32 {
    const shift: u5 = @intCast(32 - @as(u6, n));
    return (x >> n) | (x << shift);
}

pub fn readLe32(bytes: []const u8): u32 {
    return @as(u32, bytes[0])
        | (@as(u32, bytes[1]) << 8)
        | (@as(u32, bytes[2]) << 16)
        | (@as(u32, bytes[3]) << 24);
}

pub fn writeLe32(val: u32, out: *[4]u8) {
    out[0] = @truncate(val);
    out[1] = @truncate(val >> 8);
    out[2] = @truncate(val >> 16);
    out[3] = @truncate(val >> 24);
}

pub fn wordsFromLeBytes64(bytes: [64]u8): [16]u32 {
    let words: [16]u32 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let i: usize = 0;
    while (i < 16) {
        const base = i * 4;
        words[i] = readLe32(bytes[base .. base + 4]);
        i += 1;
    }
    return words;
}

pub fn keyWordsFromBytes(key: [32]u8): [8]u32 {
    let words: [8]u32 = [0, 0, 0, 0, 0, 0, 0, 0];
    let i: usize = 0;
    while (i < 8) {
        const base = i * 4;
        words[i] = readLe32(key[base .. base + 4]);
        i += 1;
    }
    return words;
}

fn g(state: *[16]u32, a: usize, b: usize, c: usize, d: usize, mx: u32, my: u32) {
    state[a] = state[a] +% state[b] +% mx;
    state[d] = rotr32(state[d] ^ state[a], 16);
    state[c] = state[c] +% state[d];
    state[b] = rotr32(state[b] ^ state[c], 12);
    state[a] = state[a] +% state[b] +% my;
    state[d] = rotr32(state[d] ^ state[a], 8);
    state[c] = state[c] +% state[d];
    state[b] = rotr32(state[b] ^ state[c], 7);
}

fn round(state: *[16]u32, m: [16]u32) {
    g(state, 0, 4, 8, 12, m[0], m[1]);
    g(state, 1, 5, 9, 13, m[2], m[3]);
    g(state, 2, 6, 10, 14, m[4], m[5]);
    g(state, 3, 7, 11, 15, m[6], m[7]);
    g(state, 0, 5, 10, 15, m[8], m[9]);
    g(state, 1, 6, 11, 12, m[10], m[11]);
    g(state, 2, 7, 8, 13, m[12], m[13]);
    g(state, 3, 4, 9, 14, m[14], m[15]);
}

fn permute(m: *[16]u32) {
    let permuted: [16]u32 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let i: usize = 0;
    while (i < 16) {
        permuted[i] = m[MSG_PERMUTATION[i]];
        i += 1;
    }
    i = 0;
    while (i < 16) {
        m[i] = permuted[i];
        i += 1;
    }
}

pub fn compress(
    cv: [8]u32,
    block_words: [16]u32,
    counter: u64,
    block_len: u32,
    flags: u32,
): [16]u32 {
    const counter_low: u32 = @truncate(counter);
    const counter_high: u32 = @truncate(counter >> 32);
    let state: [16]u32 = [
        cv[0], cv[1], cv[2], cv[3],
        cv[4], cv[5], cv[6], cv[7],
        IV[0], IV[1], IV[2], IV[3],
        counter_low, counter_high, block_len, flags,
    ];
    let m: [16]u32 = [
        block_words[0], block_words[1], block_words[2], block_words[3],
        block_words[4], block_words[5], block_words[6], block_words[7],
        block_words[8], block_words[9], block_words[10], block_words[11],
        block_words[12], block_words[13], block_words[14], block_words[15],
    ];

    round(state, m);
    permute(m);
    round(state, m);
    permute(m);
    round(state, m);
    permute(m);
    round(state, m);
    permute(m);
    round(state, m);
    permute(m);
    round(state, m);
    permute(m);
    round(state, m);

    let i: usize = 0;
    while (i < 8) {
        state[i] ^= state[i + 8];
        state[i + 8] ^= cv[i];
        i += 1;
    }
    return state;
}

pub fn firstEightWords(compression_output: [16]u32): [8]u32 {
    return [
        compression_output[0], compression_output[1],
        compression_output[2], compression_output[3],
        compression_output[4], compression_output[5],
        compression_output[6], compression_output[7],
    ];
}

pub struct Output {
    pub input_cv: [8]u32,
    pub block_words: [16]u32,
    pub counter: u64,
    pub block_len: u32,
    pub flags: u32,

    pub fn chainingValue(): [8]u32 {
        return firstEightWords(compress(
            _.input_cv,
            _.block_words,
            _.counter,
            _.block_len,
            _.flags,
        ));
    }

    pub fn rootOutputBytes(seek: u64, out: []u8) {
        if (out.len == 0) {
            return;
        }
        let output_block_counter: u64 = seek / 64;
        let offset_in_block: usize = @intCast(seek % 64);
        let out_pos: usize = 0;

        while (out_pos < out.len) {
            const words = compress(
                _.input_cv,
                _.block_words,
                output_block_counter,
                _.block_len,
                _.flags | ROOT,
            );
            let block_bytes: [64]u8 = [
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            ];
            let w: usize = 0;
            while (w < 16) {
                const base = w * 4;
                const val = words[w];
                block_bytes[base] = @truncate(val);
                block_bytes[base + 1] = @truncate(val >> 8);
                block_bytes[base + 2] = @truncate(val >> 16);
                block_bytes[base + 3] = @truncate(val >> 24);
                w += 1;
            }

            const avail = 64 - offset_in_block;
            const rem = out.len - out_pos;
            const take = if (rem < avail) rem else avail;

            let k: usize = 0;
            while (k < take) {
                out[out_pos + k] = block_bytes[offset_in_block + k];
                k += 1;
            }

            out_pos += take;
            output_block_counter += 1;
            offset_in_block = 0;
        }
    }
}

pub fn parentOutput(
    left_child_cv: [8]u32,
    right_child_cv: [8]u32,
    key_words: [8]u32,
    flags: u32,
): Output {
    let block_words: [16]u32 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let i: usize = 0;
    while (i < 8) {
        block_words[i] = left_child_cv[i];
        block_words[i + 8] = right_child_cv[i];
        i += 1;
    }
    return Output {
        input_cv: key_words,
        block_words: block_words,
        counter: 0,
        block_len: @intCast(BLOCK_LEN),
        flags: PARENT | flags,
    };
}

pub fn parentCv(
    left_child_cv: [8]u32,
    right_child_cv: [8]u32,
    key_words: [8]u32,
    flags: u32,
): [8]u32 {
    const out = parentOutput(left_child_cv, right_child_cv, key_words, flags);
    return out.chainingValue();
}
