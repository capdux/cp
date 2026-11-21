// Source: cp/lib/blake3/hasher.cp (CP stdlib BLAKE3 streaming hasher, MAC, KDF & XOF)

import {
    OUT_LEN,
    KEY_LEN,
    BLOCK_LEN,
    CHUNK_LEN,
    CHUNK_START,
    CHUNK_END,
    KEYED_HASH,
    DERIVE_KEY_CONTEXT,
    DERIVE_KEY_MATERIAL,
    IV,
    Output,
    compress,
    firstEightWords,
    wordsFromLeBytes64,
    keyWordsFromBytes,
    parentOutput,
    parentCv,
} from "./compress.cp";

const ZERO_BLOCK: [64]u8 = [
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
];

const ZERO_CV: [8]u32 = [0, 0, 0, 0, 0, 0, 0, 0];

pub struct ChunkState {
    pub chaining_value: [8]u32,
    pub chunk_counter: u64,
    pub block: [64]u8,
    pub block_len: u8,
    pub blocks_compressed: u8,
    pub flags: u32,

    pub fn init(key_words: [8]u32, chunk_counter: u64, flags: u32): ChunkState {
        return ChunkState {
            chaining_value: key_words,
            chunk_counter: chunk_counter,
            block: ZERO_BLOCK,
            block_len: 0,
            blocks_compressed: 0,
            flags: flags,
        };
    }

    pub fn len(): usize {
        return BLOCK_LEN * @as(usize, _.blocks_compressed) + @as(usize, _.block_len);
    }

    pub fn startFlag(): u32 {
        if (_.blocks_compressed == 0) {
            return CHUNK_START;
        }
        return 0;
    }

    pub fn update(*_, input: []const u8) {
        let rem = input;
        while (rem.len > 0) {
            if (@as(usize, _.block_len) == BLOCK_LEN) {
                const block_words = wordsFromLeBytes64(_.block);
                _.chaining_value = firstEightWords(compress(
                    _.chaining_value,
                    block_words,
                    _.chunk_counter,
                    @intCast(BLOCK_LEN),
                    _.flags | _.startFlag(),
                ));
                _.blocks_compressed += 1;
                _.block = ZERO_BLOCK;
                _.block_len = 0;
            }

            const want = BLOCK_LEN - @as(usize, _.block_len);
            const take = if (rem.len < want) rem.len else want;
            const dst_start: usize = @as(usize, _.block_len);
            let i: usize = 0;
            while (i < take) {
                _.block[dst_start + i] = rem[i];
                i += 1;
            }
            const take_u8: u8 = @intCast(take);
            _.block_len += take_u8;
            rem = rem[take..];
        }
    }

    pub fn output(): Output {
        const block_words = wordsFromLeBytes64(_.block);
        return Output {
            input_cv: _.chaining_value,
            block_words: block_words,
            counter: _.chunk_counter,
            block_len: @as(u32, _.block_len),
            flags: _.flags | _.startFlag() | CHUNK_END,
        };
    }
}

pub struct Blake3Hasher {
    pub chunk_state: ChunkState,
    pub key_words: [8]u32,
    pub cv_stack: [54][8]u32,
    pub cv_stack_len: u8,
    pub flags: u32,

    fn initInternal(key_words: [8]u32, flags: u32): Blake3Hasher {
        return Blake3Hasher {
            chunk_state: ChunkState.init(key_words, 0, flags),
            key_words: key_words,
            cv_stack: [
                ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV,
                ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV,
                ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV,
                ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV,
                ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV,
                ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV, ZERO_CV,
            ],
            cv_stack_len: 0,
            flags: flags,
        };
    }

    pub fn init(): Blake3Hasher {
        return Blake3Hasher.initInternal(IV, 0);
    }

    pub fn initKeyed(key: [32]u8): Blake3Hasher {
        const kw = keyWordsFromBytes(key);
        return Blake3Hasher.initInternal(kw, KEYED_HASH);
    }

    pub fn initDeriveKey(context: []const u8): Blake3Hasher {
        let context_hasher = Blake3Hasher.initInternal(IV, DERIVE_KEY_CONTEXT);
        context_hasher.update(context);
        let context_key: [32]u8 = [
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        ];
        context_hasher.final(&context_key);
        const context_key_words = keyWordsFromBytes(context_key);
        return Blake3Hasher.initInternal(context_key_words, DERIVE_KEY_MATERIAL);
    }

    pub fn reset(*_) {
        _.chunk_state = ChunkState.init(_.key_words, 0, _.flags);
        _.cv_stack_len = 0;
    }

    fn pushStack(*_, cv: [8]u32) {
        _.cv_stack[@as(usize, _.cv_stack_len)] = cv;
        _.cv_stack_len += 1;
    }

    fn popStack(*_): [8]u32 {
        _.cv_stack_len -= 1;
        return _.cv_stack[@as(usize, _.cv_stack_len)];
    }

    fn addChunkChainingValue(*_, new_cv: [8]u32, total_chunks: u64) {
        let cv = new_cv;
        let chunks = total_chunks;
        while ((chunks & 1) == 0) {
            cv = parentCv(_.popStack(), cv, _.key_words, _.flags);
            chunks >>= 1;
        }
        _.pushStack(cv);
    }

    pub fn update(*_, input: []const u8) {
        let rem = input;
        while (rem.len > 0) {
            if (_.chunk_state.len() == CHUNK_LEN) {
                const chunk_out = _.chunk_state.output();
                const chunk_cv = chunk_out.chainingValue();
                const total_chunks = _.chunk_state.chunk_counter + 1;
                _.addChunkChainingValue(chunk_cv, total_chunks);
                _.chunk_state = ChunkState.init(_.key_words, total_chunks, _.flags);
            }

            const want = CHUNK_LEN - _.chunk_state.len();
            const take = if (rem.len < want) rem.len else want;
            _.chunk_state.update(rem[0..take]);
            rem = rem[take..];
        }
    }

    pub fn finalSeek(seek_pos: u64, out: []u8) {
        let output = _.chunk_state.output();
        let parent_nodes_remaining: usize = @as(usize, _.cv_stack_len);
        while (parent_nodes_remaining > 0) {
            parent_nodes_remaining -= 1;
            output = parentOutput(
                _.cv_stack[parent_nodes_remaining],
                output.chainingValue(),
                _.key_words,
                _.flags,
            );
        }
        output.rootOutputBytes(seek_pos, out);
    }

    pub fn final(out: []u8) {
        _.finalSeek(0, out);
    }
}

pub fn hash(input: []const u8): [32]u8 {
    let hasher = Blake3Hasher.init();
    hasher.update(input);
    let out: [32]u8 = [
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ];
    hasher.final(&out);
    return out;
}

pub fn hashInto(input: []const u8, out: []u8) {
    let hasher = Blake3Hasher.init();
    hasher.update(input);
    hasher.final(out);
}

pub fn keyedHash(key: [32]u8, input: []const u8): [32]u8 {
    let hasher = Blake3Hasher.initKeyed(key);
    hasher.update(input);
    let out: [32]u8 = [
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ];
    hasher.final(&out);
    return out;
}

pub fn deriveKey(context: []const u8, key_material: []const u8, out: []u8) {
    let hasher = Blake3Hasher.initDeriveKey(context);
    hasher.update(key_material);
    hasher.final(out);
}

pub fn hashHex(input: []const u8): [64]u8 {
    const digest = hash(input);
    const hex_chars = "0123456789abcdef";
    let out: [64]u8 = [
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ];
    let i: usize = 0;
    while (i < 32) {
        const b = digest[i];
        out[i * 2] = hex_chars[@as(usize, b >> 4)];
        out[i * 2 + 1] = hex_chars[@as(usize, b & 0x0f)];
        i += 1;
    }
    return out;
}

pub fn constantTimeEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) {
        diff |= a[i] ^ b[i];
        i += 1;
    }
    return diff == 0;
}

pub fn verifyKeyedMac(key: [32]u8, input: []const u8, expected_mac: []const u8): bool {
    const actual = keyedHash(key, input);
    return constantTimeEqual(&actual, expected_mac);
}
