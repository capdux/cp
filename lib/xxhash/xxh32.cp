// Source: cp/lib/xxhash/xxh32.cp (Pure CP XXH32 & streaming implementation)

const PRIME32_1: u32 = 0x9E3779B1;
const PRIME32_2: u32 = 0x85EBCA77;
const PRIME32_3: u32 = 0xC2B2AE3D;
const PRIME32_4: u32 = 0x27D4EB2F;
const PRIME32_5: u32 = 0x165667B1;

fn rotl32(x: u32, r: u5): u32 {
    if (r == 0) {
        return x;
    }
    return (x << r) | (x >> @intCast(32 - @as(u32, r)));
}

fn readU32LE(buf: []const u8, offset: usize): u32 {
    return @as(u32, buf[offset]) |
        (@as(u32, buf[offset + 1]) << 8) |
        (@as(u32, buf[offset + 2]) << 16) |
        (@as(u32, buf[offset + 3]) << 24);
}

fn round32(acc: u32, input: u32): u32 {
    const a = acc +% (input *% PRIME32_2);
    return rotl32(a, 13) *% PRIME32_1;
}

fn avalanche32(h_in: u32): u32 {
    let h = h_in;
    h ^= h >> 15;
    h = h *% PRIME32_2;
    h ^= h >> 13;
    h = h *% PRIME32_3;
    h ^= h >> 16;
    return h;
}

fn finalize32(h_in: u32, input: []const u8, start: usize): u32 {
    let h = h_in;
    let i = start;
    while (i + 4 <= input.len) {
        h = h +% (readU32LE(input, i) *% PRIME32_3);
        h = rotl32(h, 17) *% PRIME32_4;
        i += 4;
    }
    while (i < input.len) {
        h = h +% (@as(u32, input[i]) *% PRIME32_5);
        h = rotl32(h, 11) *% PRIME32_1;
        i += 1;
    }
    return avalanche32(h);
}

pub fn xxh32(input: []const u8, seed: u32): u32 {
    let h32: u32 = 0;
    let i: usize = 0;

    if (input.len >= 16) {
        let v1 = seed +% PRIME32_1 +% PRIME32_2;
        let v2 = seed +% PRIME32_2;
        let v3 = seed;
        let v4 = seed -% PRIME32_1;

        const limit = input.len - 16;
        while (i <= limit) {
            v1 = round32(v1, readU32LE(input, i));
            v2 = round32(v2, readU32LE(input, i + 4));
            v3 = round32(v3, readU32LE(input, i + 8));
            v4 = round32(v4, readU32LE(input, i + 12));
            i += 16;
        }
        h32 = rotl32(v1, 1) +% rotl32(v2, 7) +% rotl32(v3, 12) +% rotl32(v4, 18);
    } else {
        h32 = seed +% PRIME32_5;
    }

    h32 = h32 +% @as(u32, @truncate(input.len));
    return finalize32(h32, input, i);
}

pub struct Xxh32Stream {
    total_len: u64,
    large_len: bool,
    v1: u32,
    v2: u32,
    v3: u32,
    v4: u32,
    mem: [16]u8,
    mem_size: usize,

    pub static fn init(seed: u32): Xxh32Stream {
        return Xxh32Stream {
            total_len: 0,
            large_len: false,
            v1: seed +% PRIME32_1 +% PRIME32_2,
            v2: seed +% PRIME32_2,
            v3: seed,
            v4: seed -% PRIME32_1,
            mem: [0; 16],
            mem_size: 0,
        };
    }

    pub fn reset(*_, seed: u32) {
        _.total_len = 0;
        _.large_len = false;
        _.v1 = seed +% PRIME32_1 +% PRIME32_2;
        _.v2 = seed +% PRIME32_2;
        _.v3 = seed;
        _.v4 = seed -% PRIME32_1;
        _.mem_size = 0;
    }

    pub fn update(*_, input: []const u8) {
        if (input.len == 0) {
            return;
        }
        _.total_len = _.total_len +% @as(u64, input.len);
        _.large_len = _.large_len || (input.len >= 16) || (_.total_len >= 16);

        let i: usize = 0;
        if (_.mem_size + input.len < 16) {
            while (i < input.len) {
                _.mem[_.mem_size + i] = input[i];
                i += 1;
            }
            _.mem_size += input.len;
            return;
        }

        if (_.mem_size > 0) {
            const fill: usize = @as(usize, 16) - _.mem_size;
            let j: usize = 0;
            while (j < fill) {
                _.mem[_.mem_size + j] = input[j];
                j += 1;
            }
            _.v1 = round32(_.v1, readU32LE(_.mem[0..16], 0));
            _.v2 = round32(_.v2, readU32LE(_.mem[0..16], 4));
            _.v3 = round32(_.v3, readU32LE(_.mem[0..16], 8));
            _.v4 = round32(_.v4, readU32LE(_.mem[0..16], 12));
            i = fill;
            _.mem_size = 0;
        }

        if (i + 16 <= input.len) {
            const limit = input.len - 16;
            while (i <= limit) {
                _.v1 = round32(_.v1, readU32LE(input, i));
                _.v2 = round32(_.v2, readU32LE(input, i + 4));
                _.v3 = round32(_.v3, readU32LE(input, i + 8));
                _.v4 = round32(_.v4, readU32LE(input, i + 12));
                i += 16;
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

    pub fn digest(): u32 {
        let h32: u32 = 0;
        if (_.large_len) {
            h32 = rotl32(_.v1, 1) +% rotl32(_.v2, 7) +% rotl32(_.v3, 12) +% rotl32(_.v4, 18);
        } else {
            h32 = _.v3 +% PRIME32_5;
        }
        h32 = h32 +% @as(u32, @truncate(_.total_len));
        return finalize32(h32, _.mem[0.._.mem_size], 0);
    }
}
