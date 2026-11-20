// Source: cp/lib/xxhash/xxh64.cp (Pure CP XXH64 & streaming implementation)

const PRIME64_1: u64 = 0x9E3779B185EBCA87;
const PRIME64_2: u64 = 0xC2B2AE3D27D4EB4F;
const PRIME64_3: u64 = 0x165667B19E3779F9;
const PRIME64_4: u64 = 0x85EBCA77C2B2AE63;
const PRIME64_5: u64 = 0x27D4EB2F165667C5;

fn rotl64(x: u64, r: u6): u64 {
    if (r == 0) {
        return x;
    }
    return (x << r) | (x >> @intCast(64 - @as(u64, r)));
}

fn readU32LE(buf: []const u8, offset: usize): u32 {
    return @as(u32, buf[offset]) |
        (@as(u32, buf[offset + 1]) << 8) |
        (@as(u32, buf[offset + 2]) << 16) |
        (@as(u32, buf[offset + 3]) << 24);
}

fn readU64LE(buf: []const u8, offset: usize): u64 {
    return @as(u64, buf[offset]) |
        (@as(u64, buf[offset + 1]) << 8) |
        (@as(u64, buf[offset + 2]) << 16) |
        (@as(u64, buf[offset + 3]) << 24) |
        (@as(u64, buf[offset + 4]) << 32) |
        (@as(u64, buf[offset + 5]) << 40) |
        (@as(u64, buf[offset + 6]) << 48) |
        (@as(u64, buf[offset + 7]) << 56);
}

fn round64(acc: u64, input: u64): u64 {
    const a = acc +% (input *% PRIME64_2);
    return rotl64(a, 31) *% PRIME64_1;
}

fn mergeRound64(acc: u64, val_in: u64): u64 {
    const val = round64(0, val_in);
    const a = acc ^ val;
    return (a *% PRIME64_1) +% PRIME64_4;
}

fn avalanche64(h_in: u64): u64 {
    let h = h_in;
    h ^= h >> 33;
    h = h *% PRIME64_2;
    h ^= h >> 29;
    h = h *% PRIME64_3;
    h ^= h >> 32;
    return h;
}

fn finalize64(h_in: u64, input: []const u8, start: usize): u64 {
    let h = h_in;
    let i = start;
    while (i + 8 <= input.len) {
        const k1 = round64(0, readU64LE(input, i));
        h ^= k1;
        h = (rotl64(h, 27) *% PRIME64_1) +% PRIME64_4;
        i += 8;
    }
    while (i + 4 <= input.len) {
        h ^= @as(u64, readU32LE(input, i)) *% PRIME64_1;
        h = (rotl64(h, 23) *% PRIME64_2) +% PRIME64_3;
        i += 4;
    }
    while (i < input.len) {
        h ^= @as(u64, input[i]) *% PRIME64_5;
        h = rotl64(h, 11) *% PRIME64_1;
        i += 1;
    }
    return avalanche64(h);
}

pub fn xxh64(input: []const u8, seed: u64): u64 {
    let h64: u64 = 0;
    let i: usize = 0;

    if (input.len >= 32) {
        let v1 = seed +% PRIME64_1 +% PRIME64_2;
        let v2 = seed +% PRIME64_2;
        let v3 = seed;
        let v4 = seed -% PRIME64_1;

        const limit = input.len - 32;
        while (i <= limit) {
            v1 = round64(v1, readU64LE(input, i));
            v2 = round64(v2, readU64LE(input, i + 8));
            v3 = round64(v3, readU64LE(input, i + 16));
            v4 = round64(v4, readU64LE(input, i + 24));
            i += 32;
        }
        h64 = rotl64(v1, 1) +% rotl64(v2, 7) +% rotl64(v3, 12) +% rotl64(v4, 18);
        h64 = mergeRound64(h64, v1);
        h64 = mergeRound64(h64, v2);
        h64 = mergeRound64(h64, v3);
        h64 = mergeRound64(h64, v4);
    } else {
        h64 = seed +% PRIME64_5;
    }

    h64 = h64 +% @as(u64, input.len);
    return finalize64(h64, input, i);
}

pub struct Xxh64Stream {
    total_len: u64,
    v1: u64,
    v2: u64,
    v3: u64,
    v4: u64,
    mem: [32]u8,
    mem_size: usize,

    pub fn init(seed: u64): Xxh64Stream {
        return Xxh64Stream {
            total_len: 0,
            v1: seed +% PRIME64_1 +% PRIME64_2,
            v2: seed +% PRIME64_2,
            v3: seed,
            v4: seed -% PRIME64_1,
            mem: [0; 32],
            mem_size: 0,
        };
    }

    pub fn reset(*_, seed: u64) {
        _.total_len = 0;
        _.v1 = seed +% PRIME64_1 +% PRIME64_2;
        _.v2 = seed +% PRIME64_2;
        _.v3 = seed;
        _.v4 = seed -% PRIME64_1;
        _.mem_size = 0;
    }

    pub fn update(*_, input: []const u8) {
        if (input.len == 0) {
            return;
        }
        _.total_len = _.total_len +% @as(u64, input.len);

        let i: usize = 0;
        if (_.mem_size + input.len < 32) {
            while (i < input.len) {
                _.mem[_.mem_size + i] = input[i];
                i += 1;
            }
            _.mem_size += input.len;
            return;
        }

        if (_.mem_size > 0) {
            const fill: usize = @as(usize, 32) - _.mem_size;
            let j: usize = 0;
            while (j < fill) {
                _.mem[_.mem_size + j] = input[j];
                j += 1;
            }
            _.v1 = round64(_.v1, readU64LE(_.mem[0..32], 0));
            _.v2 = round64(_.v2, readU64LE(_.mem[0..32], 8));
            _.v3 = round64(_.v3, readU64LE(_.mem[0..32], 16));
            _.v4 = round64(_.v4, readU64LE(_.mem[0..32], 24));
            i = fill;
            _.mem_size = 0;
        }

        if (i + 32 <= input.len) {
            const limit = input.len - 32;
            while (i <= limit) {
                _.v1 = round64(_.v1, readU64LE(input, i));
                _.v2 = round64(_.v2, readU64LE(input, i + 8));
                _.v3 = round64(_.v3, readU64LE(input, i + 16));
                _.v4 = round64(_.v4, readU64LE(input, i + 24));
                i += 32;
            }
        }

        if (i < input.len) {
            const rem = input.len - i;
            let k: usize = 0;
            while (k < rem) {
                _.mem[k] = input[i + k];
                k += 1;
            }
            _.mem_size = rem;
        }
    }

    pub fn digest(): u64 {
        let h64: u64 = 0;
        if (_.total_len >= 32) {
            h64 = rotl64(_.v1, 1) +% rotl64(_.v2, 7) +% rotl64(_.v3, 12) +% rotl64(_.v4, 18);
            h64 = mergeRound64(h64, _.v1);
            h64 = mergeRound64(h64, _.v2);
            h64 = mergeRound64(h64, _.v3);
            h64 = mergeRound64(h64, _.v4);
        } else {
            h64 = _.v3 +% PRIME64_5;
        }
        h64 = h64 +% _.total_len;
        return finalize64(h64, _.mem[0.._.mem_size], 0);
    }
}
