// Source: cp/lib/lsm_sst/bloom.cp (Kirsch-Mitzenmacher Double-Hashing Bloom Filter)
const std = @import("std");

fn insertKey(out: []u8, actual_bits: u64, k: u32, key: []const u8) {
    const h1: u64 = std.hash.Wyhash.hash(0, key);
    const h2: u64 = std.hash.Wyhash.hash(0x9e3779b97f4a7c15, key) | 1;
    let i: u32 = 0;
    while (i < k) {
        const bit_pos: usize = @as(usize, @intCast((h1 +% @as(u64, i) *% h2) % actual_bits));
        const byte_idx = bit_pos >> 3;
        const bit_mask: u8 = @as(u8, 1) << @as(u3, @intCast(bit_pos & 7));
        out[byte_idx] |= bit_mask;
        i += 1;
    }
}

fn allocFilterHeader(alloc: std.mem.Allocator, key_count: usize, bits_per_key: u32, out_bits: *u64, out_k: *u32): ![]u8 {
    const bpk: usize = if (bits_per_key == 0) 10 else @as(usize, bits_per_key);
    let bits: usize = key_count * bpk;
    if (bits < 64) {
        bits = 64;
    }
    const bytes_len: usize = (bits + 7) / 8;
    out_bits.* = @as(u64, bytes_len) * 8;

    let k: u32 = @as(u32, @intCast((bpk * 69) / 100));
    if (k < 1) {
        k = 1;
    }
    if (k > 30) {
        k = 30;
    }
    out_k.* = k;

    let out = try alloc.alloc(u8, bytes_len + 1);
    @memset(out, 0);
    out[bytes_len] = @as(u8, @intCast(k));
    return out;
}

pub struct BloomFilter {
    pub static fn build(alloc: std.mem.Allocator, keys: []const []const u8, bits_per_key: u32): ![]u8 {
        let actual_bits: u64 = 0;
        let k: u32 = 0;
        let out = try allocFilterHeader(alloc, keys.len, bits_per_key, &actual_bits, &k);
        for (key of keys) {
            insertKey(out, actual_bits, k, key);
        }
        return out;
    }

    pub static fn buildFromMutKeys(alloc: std.mem.Allocator, keys: []const []u8, bits_per_key: u32): ![]u8 {
        let actual_bits: u64 = 0;
        let k: u32 = 0;
        let out = try allocFilterHeader(alloc, keys.len, bits_per_key, &actual_bits, &k);
        for (key of keys) {
            insertKey(out, actual_bits, k, key);
        }
        return out;
    }

    pub static fn mayContain(filter_bytes: []const u8, key: []const u8): bool {
        if (filter_bytes.len < 2) {
            return true;
        }
        const k: u32 = filter_bytes[filter_bytes.len - 1];
        if (k == 0 or k > 30) {
            return true;
        }
        const actual_bits: u64 = @as(u64, filter_bytes.len - 1) * 8;
        const h1: u64 = std.hash.Wyhash.hash(0, key);
        const h2: u64 = std.hash.Wyhash.hash(0x9e3779b97f4a7c15, key) | 1;

        let i: u32 = 0;
        while (i < k) {
            const bit_pos: usize = @as(usize, @intCast((h1 +% @as(u64, i) *% h2) % actual_bits));
            const byte_idx = bit_pos >> 3;
            const bit_mask: u8 = @as(u8, 1) << @as(u3, @intCast(bit_pos & 7));
            if ((filter_bytes[byte_idx] & bit_mask) == 0) {
                return false;
            }
            i += 1;
        }
        return true;
    }
}

pub fn buildBloomFilter(alloc: std.mem.Allocator, keys: []const []const u8, bits_per_key: u32): ![]u8 {
    return BloomFilter.build(alloc, keys, bits_per_key);
}

pub fn mayContain(filter_bytes: []const u8, key: []const u8): bool {
    return BloomFilter.mayContain(filter_bytes, key);
}
