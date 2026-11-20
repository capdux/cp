// Source: cp/lib/xxhash/xxh3.cp (Pure CP XXH3_64bits & XXH3_128bits implementation)

const PRIME32_1: u64 = 0x9E3779B1;
const PRIME32_2: u64 = 0x85EBCA77;
const PRIME32_3: u64 = 0xC2B2AE3D;

const PRIME64_1: u64 = 0x9E3779B185EBCA87;
const PRIME64_2: u64 = 0xC2B2AE3D27D4EB4F;
const PRIME64_3: u64 = 0x165667B19E3779F9;
const PRIME64_4: u64 = 0x85EBCA77C2B2AE63;
const PRIME64_5: u64 = 0x27D4EB2F165667C5;

const PRIME_MX1: u64 = 0x165667919E3779F9;
const PRIME_MX2: u64 = 0x9FB21C651E98DF25;

pub const XXH3_DEFAULT_SECRET: [192]u8 = [
    0xb8, 0xfe, 0x6c, 0x39, 0x23, 0xa4, 0x4b, 0xbe, 0x7c, 0x01, 0x81, 0x2c, 0xf7, 0x21, 0xad, 0x1c,
    0xde, 0xd4, 0x6d, 0xe9, 0x83, 0x90, 0x97, 0xdb, 0x72, 0x40, 0xa4, 0xa4, 0xb7, 0xb3, 0x67, 0x1f,
    0xcb, 0x79, 0xe6, 0x4e, 0xcc, 0xc0, 0xe5, 0x78, 0x82, 0x5a, 0xd0, 0x7d, 0xcc, 0xff, 0x72, 0x21,
    0xb8, 0x08, 0x46, 0x74, 0xf7, 0x43, 0x24, 0x8e, 0xe0, 0x35, 0x90, 0xe6, 0x81, 0x3a, 0x26, 0x4c,
    0x3c, 0x28, 0x52, 0xbb, 0x91, 0xc3, 0x00, 0xcb, 0x88, 0xd0, 0x65, 0x8b, 0x1b, 0x53, 0x2e, 0xa3,
    0x71, 0x64, 0x48, 0x97, 0xa2, 0x0d, 0xf9, 0x4e, 0x38, 0x19, 0xef, 0x46, 0xa9, 0xde, 0xac, 0xd8,
    0xa8, 0xfa, 0x76, 0x3f, 0xe3, 0x9c, 0x34, 0x3f, 0xf9, 0xdc, 0xbb, 0xc7, 0xc7, 0x0b, 0x4f, 0x1d,
    0x8a, 0x51, 0xe0, 0x4b, 0xcd, 0xb4, 0x59, 0x31, 0xc8, 0x9f, 0x7e, 0xc9, 0xd9, 0x78, 0x73, 0x64,
    0xea, 0xc5, 0xac, 0x83, 0x34, 0xd3, 0xeb, 0xc3, 0xc5, 0x81, 0xa0, 0xff, 0xfa, 0x13, 0x63, 0xeb,
    0x17, 0x0d, 0xdd, 0x51, 0xb7, 0xf0, 0xda, 0x49, 0xd3, 0x16, 0x55, 0x26, 0x29, 0xd4, 0x68, 0x9e,
    0x2b, 0x16, 0xbe, 0x58, 0x7d, 0x47, 0xa1, 0xfc, 0x8f, 0xf8, 0xb8, 0xd1, 0x7a, 0xd0, 0x31, 0xce,
    0x45, 0xcb, 0x3a, 0x8f, 0x95, 0x16, 0x04, 0x28, 0xaf, 0xd7, 0xfb, 0xca, 0xbb, 0x4b, 0x40, 0x7e,
];

const XXH3_INIT_ACC: [8]u64 = [
    PRIME32_3, PRIME64_1, PRIME64_2, PRIME64_3,
    PRIME64_4, PRIME32_2, PRIME64_5, PRIME32_1,
];

pub struct Xxh128 {
    pub low: u64,
    pub high: u64,

    pub fn eql(other: Xxh128): bool {
        return _.low == other.low && _.high == other.high;
    }
}

fn rotl64(x: u64, r: u6): u64 {
    if (r == 0) {
        return x;
    }
    return (x << r) | (x >> @intCast(64 - @as(u64, r)));
}

fn rotr64(x: u64, r: u6): u64 {
    if (r == 0) {
        return x;
    }
    return (x >> r) | (x << @intCast(64 - @as(u64, r)));
}

fn bswap32(x: u32): u32 {
    return ((x & 0x000000FF) << 24) |
        ((x & 0x0000FF00) << 8) |
        ((x & 0x00FF0000) >> 8) |
        ((x & 0xFF000000) >> 24);
}

fn bswap64(x: u64): u64 {
    return ((x & 0x00000000000000FF) << 56) |
        ((x & 0x000000000000FF00) << 40) |
        ((x & 0x0000000000FF0000) << 24) |
        ((x & 0x00000000FF000000) << 8) |
        ((x & 0x000000FF00000000) >> 8) |
        ((x & 0x0000FF0000000000) >> 24) |
        ((x & 0x00FF000000000000) >> 40) |
        ((x & 0xFF00000000000000) >> 56);
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

fn writeU64LE(buf: []u8, offset: usize, val: u64) {
    buf[offset] = @truncate(val);
    buf[offset + 1] = @truncate(val >> 8);
    buf[offset + 2] = @truncate(val >> 16);
    buf[offset + 3] = @truncate(val >> 24);
    buf[offset + 4] = @truncate(val >> 32);
    buf[offset + 5] = @truncate(val >> 40);
    buf[offset + 6] = @truncate(val >> 48);
    buf[offset + 7] = @truncate(val >> 56);
}

fn mul128Fold64(lhs: u64, rhs: u64): u64 {
    const prod = @as(u128, lhs) * @as(u128, rhs);
    const lo: u64 = @truncate(prod);
    const hi: u64 = @truncate(prod >> 64);
    return lo ^ hi;
}

fn xxh64Avalanche(h_in: u64): u64 {
    let h = h_in;
    h ^= h >> 33;
    h = h *% PRIME64_2;
    h ^= h >> 29;
    h = h *% PRIME64_3;
    h ^= h >> 32;
    return h;
}

fn xxh3Avalanche(h_in: u64): u64 {
    let h = h_in;
    h ^= h >> 37;
    h = h *% PRIME_MX1;
    h ^= h >> 32;
    return h;
}

fn rrmxmx(h_in: u64, len: u64): u64 {
    let h = h_in ^ (rotl64(h_in, 49) ^ rotl64(h_in, 24));
    h = h *% PRIME_MX2;
    h ^= (h >> 35) +% len;
    h = h *% PRIME_MX2;
    return h ^ (h >> 28);
}

fn mix16B(input: []const u8, in_off: usize, secret: []const u8, sec_off: usize, seed: u64): u64 {
    const input_lo = readU64LE(input, in_off);
    const input_hi = readU64LE(input, in_off + 8);
    return mul128Fold64(
        input_lo ^ (readU64LE(secret, sec_off) +% seed),
        input_hi ^ (readU64LE(secret, sec_off + 8) -% seed),
    );
}

fn mixStep(input: []const u8, in_off: usize, secret: []const u8, sec_off: usize, acc: *[8]u64) {
    let i: usize = 0;
    while (i < 8) {
        const data_val = readU64LE(input, in_off + i * 8);
        const key_val = data_val ^ readU64LE(secret, sec_off + i * 8);
        acc[i ^ 1] = acc[i ^ 1] +% data_val;
        acc[i] = acc[i] +% (@as(u64, @as(u32, @truncate(key_val))) *% @as(u64, @as(u32, @truncate(key_val >> 32))));
        i += 1;
    }
}

fn scrambleAcc(acc: *[8]u64, secret: []const u8, sec_off: usize) {
    let i: usize = 0;
    while (i < 8) {
        let x = acc[i] ^ (acc[i] >> 47);
        x ^= readU64LE(secret, sec_off + i * 8);
        acc[i] = x *% PRIME32_1;
        i += 1;
    }
}

fn hashLong64Internal(input: []const u8, secret: []const u8): u64 {
    let acc: [8]u64 = XXH3_INIT_ACC;
    const nb_stripes_per_block = (secret.len - 64) / 8;
    const block_len = 64 * nb_stripes_per_block;
    const nb_blocks = (input.len - 1) / block_len;

    let b: usize = 0;
    while (b < nb_blocks) {
        let s: usize = 0;
        while (s < nb_stripes_per_block) {
            mixStep(input, b * block_len + s * 64, secret, s * 8, &acc);
            s += 1;
        }
        scrambleAcc(&acc, secret, secret.len - 64);
        b += 1;
    }

    const nb_stripes = ((input.len - 1) - (block_len * nb_blocks)) / 64;
    let s: usize = 0;
    while (s < nb_stripes) {
        mixStep(input, nb_blocks * block_len + s * 64, secret, s * 8, &acc);
        s += 1;
    }
    mixStep(input, input.len - 64, secret, secret.len - 64 - 7, &acc);

    let result = @as(u64, input.len) *% PRIME64_1;
    let i: usize = 0;
    while (i < 4) {
        result = result +% mul128Fold64(
            acc[i * 2] ^ readU64LE(secret, 11 + i * 16),
            acc[i * 2 + 1] ^ readU64LE(secret, 11 + i * 16 + 8),
        );
        i += 1;
    }
    return xxh3Avalanche(result);
}

fn initCustomSecret(custom_secret: *[192]u8, seed: u64) {
    let i: usize = 0;
    while (i < 12) {
        const lo = readU64LE(XXH3_DEFAULT_SECRET[0..192], i * 16) +% seed;
        const hi = readU64LE(XXH3_DEFAULT_SECRET[0..192], i * 16 + 8) -% seed;
        writeU64LE(custom_secret[0..192], i * 16, lo);
        writeU64LE(custom_secret[0..192], i * 16 + 8, hi);
        i += 1;
    }
}

fn len1to3_64(input: []const u8, secret: []const u8, seed: u64): u64 {
    const c1: u32 = @as(u32, input[0]);
    const c2: u32 = @as(u32, input[input.len >> 1]);
    const c3: u32 = @as(u32, input[input.len - 1]);
    const combined: u32 = (c1 << 16) | (c2 << 24) | c3 | (@as(u32, @truncate(input.len)) << 8);
    const bitflip = @as(u64, readU32LE(secret, 0) ^ readU32LE(secret, 4)) +% seed;
    const keyed = @as(u64, combined) ^ bitflip;
    return xxh64Avalanche(keyed);
}

fn len4to8_64(input: []const u8, secret: []const u8, seed_in: u64): u64 {
    const seed = seed_in ^ (@as(u64, bswap32(@truncate(seed_in))) << 32);
    const input1 = readU32LE(input, 0);
    const input2 = readU32LE(input, input.len - 4);
    const bitflip = (readU64LE(secret, 8) ^ readU64LE(secret, 16)) -% seed;
    const input64 = @as(u64, input2) +% (@as(u64, input1) << 32);
    const keyed = input64 ^ bitflip;
    return rrmxmx(keyed, @as(u64, input.len));
}

fn len9to16_64(input: []const u8, secret: []const u8, seed: u64): u64 {
    const bitflip1 = (readU64LE(secret, 24) ^ readU64LE(secret, 32)) +% seed;
    const bitflip2 = (readU64LE(secret, 40) ^ readU64LE(secret, 48)) -% seed;
    const input_lo = readU64LE(input, 0) ^ bitflip1;
    const input_hi = readU64LE(input, input.len - 8) ^ bitflip2;
    const acc = @as(u64, input.len) +% bswap64(input_lo) +% input_hi +% mul128Fold64(input_lo, input_hi);
    return xxh3Avalanche(acc);
}

fn len17to128_64(input: []const u8, secret: []const u8, seed: u64): u64 {
    let acc = @as(u64, input.len) *% PRIME64_1;
    if (input.len > 32) {
        if (input.len > 64) {
            if (input.len > 96) {
                acc = acc +% mix16B(input, 48, secret, 96, seed);
                acc = acc +% mix16B(input, input.len - 64, secret, 112, seed);
            }
            acc = acc +% mix16B(input, 32, secret, 64, seed);
            acc = acc +% mix16B(input, input.len - 48, secret, 80, seed);
        }
        acc = acc +% mix16B(input, 16, secret, 32, seed);
        acc = acc +% mix16B(input, input.len - 32, secret, 48, seed);
    }
    acc = acc +% mix16B(input, 0, secret, 0, seed);
    acc = acc +% mix16B(input, input.len - 16, secret, 16, seed);
    return xxh3Avalanche(acc);
}

fn len129to240_64(input: []const u8, secret: []const u8, seed: u64): u64 {
    let acc = @as(u64, input.len) *% PRIME64_1;
    const nb_rounds = input.len / 16;
    let i: usize = 0;
    while (i < 8) {
        acc = acc +% mix16B(input, 16 * i, secret, 16 * i, seed);
        i += 1;
    }
    acc = xxh3Avalanche(acc);
    i = 8;
    while (i < nb_rounds) {
        acc = acc +% mix16B(input, 16 * i, secret, 16 * (i - 8) + 3, seed);
        i += 1;
    }
    acc = acc +% mix16B(input, input.len - 16, secret, 136 - 17, seed);
    return xxh3Avalanche(acc);
}

pub fn xxh3_64WithSeed(input: []const u8, seed: u64): u64 {
    const secret = XXH3_DEFAULT_SECRET[0..192];
    if (input.len == 0) {
        return xxh64Avalanche(seed ^ (readU64LE(secret, 56) ^ readU64LE(secret, 64)));
    }
    if (input.len <= 3) {
        return len1to3_64(input, secret, seed);
    }
    if (input.len <= 8) {
        return len4to8_64(input, secret, seed);
    }
    if (input.len <= 16) {
        return len9to16_64(input, secret, seed);
    }
    if (input.len <= 128) {
        return len17to128_64(input, secret, seed);
    }
    if (input.len <= 240) {
        return len129to240_64(input, secret, seed);
    }
    if (seed == 0) {
        return hashLong64Internal(input, secret);
    }
    let custom_secret: [192]u8 = [0; 192];
    initCustomSecret(&custom_secret, seed);
    return hashLong64Internal(input, custom_secret[0..192]);
}

pub fn xxh3_64(input: []const u8): u64 {
    return xxh3_64WithSeed(input, 0);
}

fn mix32B(acc_in: Xxh128, input: []const u8, off1: usize, off2: usize, secret: []const u8, sec_off: usize, seed: u64): Xxh128 {
    let low = acc_in.low;
    let high = acc_in.high;
    const in1_lo = readU64LE(input, off1);
    const in1_hi = readU64LE(input, off1 + 8);
    const in2_lo = readU64LE(input, off2);
    const in2_hi = readU64LE(input, off2 + 8);

    low = low +% mix16B(input, off1, secret, sec_off, seed);
    low ^= in2_lo +% in2_hi;
    high = high +% mix16B(input, off2, secret, sec_off + 16, seed);
    high ^= in1_lo +% in1_hi;

    return Xxh128 { low: low, high: high };
}

fn len1to3_128(input: []const u8, secret: []const u8, seed: u64): Xxh128 {
    const c1: u32 = @as(u32, input[0]);
    const c2: u32 = @as(u32, input[input.len >> 1]);
    const c3: u32 = @as(u32, input[input.len - 1]);
    const combinedl: u32 = (c1 << 16) | (c2 << 24) | c3 | (@as(u32, @truncate(input.len)) << 8);
    const combinedh: u32 = bswap32(combinedl);
    const combinedh_rot: u32 = (combinedh << 13) | (combinedh >> 19);

    const bitflipl = @as(u64, readU32LE(secret, 0) ^ readU32LE(secret, 4)) +% seed;
    const bitfliph = @as(u64, readU32LE(secret, 8) ^ readU32LE(secret, 12)) -% seed;
    const keyed_lo = @as(u64, combinedl) ^ bitflipl;
    const keyed_hi = @as(u64, combinedh_rot) ^ bitfliph;

    return Xxh128 {
        low: xxh64Avalanche(keyed_lo),
        high: xxh64Avalanche(keyed_hi),
    };
}

fn len4to8_128(input: []const u8, secret: []const u8, seed_in: u64): Xxh128 {
    const seed = seed_in ^ (@as(u64, bswap32(@truncate(seed_in))) << 32);
    const input_lo = readU32LE(input, 0);
    const input_hi = readU32LE(input, input.len - 4);
    const input_64 = @as(u64, input_lo) +% (@as(u64, input_hi) << 32);
    const bitflip = (readU64LE(secret, 16) ^ readU64LE(secret, 24)) +% seed;
    const keyed = input_64 ^ bitflip;

    const m128 = @as(u128, keyed) * @as(u128, PRIME64_1 +% (@as(u64, input.len) << 2));
    let lo: u64 = @truncate(m128);
    let hi: u64 = @truncate(m128 >> 64);
    hi = hi +% (lo << 1);
    lo ^= hi >> 3;
    lo = (lo ^ (lo >> 35)) *% PRIME_MX2;
    lo ^= lo >> 28;
    hi = xxh3Avalanche(hi);

    return Xxh128 { low: lo, high: hi };
}

fn len9to16_128(input: []const u8, secret: []const u8, seed: u64): Xxh128 {
    const bitflipl = (readU64LE(secret, 32) ^ readU64LE(secret, 40)) -% seed;
    const bitfliph = (readU64LE(secret, 48) ^ readU64LE(secret, 56)) +% seed;
    const input_lo = readU64LE(input, 0);
    let input_hi = readU64LE(input, input.len - 8);
    const m128 = @as(u128, input_lo ^ input_hi ^ bitflipl) * @as(u128, PRIME64_1);
    let lo: u64 = @truncate(m128);
    let hi: u64 = @truncate(m128 >> 64);

    lo = lo +% (@as(u64, input.len - 1) << 54);
    input_hi ^= bitfliph;
    const hi_lo32 = @as(u64, @as(u32, @truncate(input_hi)));
    hi = hi +% (input_hi +% (hi_lo32 *% (PRIME32_2 - 1)));
    lo ^= bswap64(hi);

    const m128_2 = @as(u128, lo) * @as(u128, PRIME64_2);
    lo = @truncate(m128_2);
    let hi2: u64 = @truncate(m128_2 >> 64);
    hi2 = hi2 +% (hi *% PRIME64_2);

    return Xxh128 {
        low: xxh3Avalanche(lo),
        high: xxh3Avalanche(hi2),
    };
}

fn len17to128_128(input: []const u8, secret: []const u8, seed: u64): Xxh128 {
    let acc = Xxh128 { low: @as(u64, input.len) *% PRIME64_1, high: 0 };
    if (input.len > 32) {
        if (input.len > 64) {
            if (input.len > 96) {
                acc = mix32B(acc, input, 48, input.len - 64, secret, 96, seed);
            }
            acc = mix32B(acc, input, 32, input.len - 48, secret, 64, seed);
        }
        acc = mix32B(acc, input, 16, input.len - 32, secret, 32, seed);
    }
    acc = mix32B(acc, input, 0, input.len - 16, secret, 0, seed);

    const low = xxh3Avalanche(acc.low +% acc.high);
    const high = 0 -% xxh3Avalanche((acc.low *% PRIME64_1) +% (acc.high *% PRIME64_4) +% ((@as(u64, input.len) -% seed) *% PRIME64_2));
    return Xxh128 { low: low, high: high };
}

fn len129to240_128(input: []const u8, secret: []const u8, seed: u64): Xxh128 {
    let acc = Xxh128 { low: @as(u64, input.len) *% PRIME64_1, high: 0 };
    const nb_rounds = input.len / 32;
    let i: usize = 0;
    while (i < 4) {
        acc = mix32B(acc, input, 32 * i, 32 * i + 16, secret, 32 * i, seed);
        i += 1;
    }
    acc.low = xxh3Avalanche(acc.low);
    acc.high = xxh3Avalanche(acc.high);

    i = 4;
    while (i < nb_rounds) {
        acc = mix32B(acc, input, 32 * i, 32 * i + 16, secret, 3 + 32 * (i - 4), seed);
        i += 1;
    }
    acc = mix32B(acc, input, input.len - 16, input.len - 32, secret, 136 - 17 - 16, 0 -% seed);

    const low = xxh3Avalanche(acc.low +% acc.high);
    const high = 0 -% xxh3Avalanche((acc.low *% PRIME64_1) +% (acc.high *% PRIME64_4) +% ((@as(u64, input.len) -% seed) *% PRIME64_2));
    return Xxh128 { low: low, high: high };
}

fn hashLong128Internal(input: []const u8, secret: []const u8): Xxh128 {
    let acc: [8]u64 = XXH3_INIT_ACC;
    const nb_stripes_per_block = (secret.len - 64) / 8;
    const block_len = 64 * nb_stripes_per_block;
    const nb_blocks = (input.len - 1) / block_len;

    let b: usize = 0;
    while (b < nb_blocks) {
        let s: usize = 0;
        while (s < nb_stripes_per_block) {
            mixStep(input, b * block_len + s * 64, secret, s * 8, &acc);
            s += 1;
        }
        scrambleAcc(&acc, secret, secret.len - 64);
        b += 1;
    }

    const nb_stripes = ((input.len - 1) - (block_len * nb_blocks)) / 64;
    let s: usize = 0;
    while (s < nb_stripes) {
        mixStep(input, nb_blocks * block_len + s * 64, secret, s * 8, &acc);
        s += 1;
    }
    mixStep(input, input.len - 64, secret, secret.len - 64 - 7, &acc);

    let lo = @as(u64, input.len) *% PRIME64_1;
    let hi = ~(@as(u64, input.len) *% PRIME64_2);
    let i: usize = 0;
    while (i < 4) {
        lo = lo +% mul128Fold64(
            acc[i * 2] ^ readU64LE(secret, 11 + i * 16),
            acc[i * 2 + 1] ^ readU64LE(secret, 11 + i * 16 + 8),
        );
        hi = hi +% mul128Fold64(
            acc[i * 2] ^ readU64LE(secret, 117 + i * 16),
            acc[i * 2 + 1] ^ readU64LE(secret, 117 + i * 16 + 8),
        );
        i += 1;
    }
    return Xxh128 {
        low: xxh3Avalanche(lo),
        high: xxh3Avalanche(hi),
    };
}

pub fn xxh3_128WithSeed(input: []const u8, seed: u64): Xxh128 {
    const secret = XXH3_DEFAULT_SECRET[0..192];
    if (input.len == 0) {
        return Xxh128 {
            low: xxh64Avalanche(seed ^ (readU64LE(secret, 64) ^ readU64LE(secret, 72))),
            high: xxh64Avalanche(seed ^ (readU64LE(secret, 80) ^ readU64LE(secret, 88))),
        };
    }
    if (input.len <= 3) {
        return len1to3_128(input, secret, seed);
    }
    if (input.len <= 8) {
        return len4to8_128(input, secret, seed);
    }
    if (input.len <= 16) {
        return len9to16_128(input, secret, seed);
    }
    if (input.len <= 128) {
        return len17to128_128(input, secret, seed);
    }
    if (input.len <= 240) {
        return len129to240_128(input, secret, seed);
    }
    if (seed == 0) {
        return hashLong128Internal(input, secret);
    }
    let custom_secret: [192]u8 = [0; 192];
    initCustomSecret(&custom_secret, seed);
    return hashLong128Internal(input, custom_secret[0..192]);
}

pub fn xxh3_128(input: []const u8): Xxh128 {
    return xxh3_128WithSeed(input, 0);
}
