// Source: cp/lib/xorfilter/xorfilter.cp (Pure CP Static Xor8 / Xor16 Minimal-Space Filter)
// Zero-C-dependency static approximate set membership filter based on 3-peeling
// hypergraph construction and reverse XOR fingerprint assignment.
// Supports O(1) lookup (3 array accesses), binary serialization, and raw 64-bit hash streams.

const std = @import("std");

const XOR8_MAGIC: u32 = 0x38524F58;  // "XOR8" in little-endian
const XOR16_MAGIC: u32 = 0x36524F58; // "XOR6" in little-endian
const FORMAT_VERSION: u32 = 1;
const HEADER_SIZE: usize = 32;
const MAX_BUILD_RETRIES: usize = 64;
const INITIAL_SEED: u64 = 0x9e3779b97f4a7c15;

/// Error set for Xor8Filter and Xor16Filter operations.
pub err XorFilterError {
    ConstructionFailed,
    InvalidSerializedData,
    OutOfMemory,
}

struct T2Set {
    count: u32,
    mask: u64,
}

struct SlotTriple {
    h0: usize,
    h1: usize,
    h2: usize,
    fp16: u16,
}

struct DedupResult {
    buf: []u64,
    unique_len: usize,
}

struct HeaderInfo {
    seed: u64,
    block_length: usize,
    item_count: usize,
    array_length: usize,
}

struct PeelPlan {
    seed: u64,
    block_length: usize,
    unique_len: usize,
    unique_hashes: []u64,
    reverse_order: []u64,
    reverse_h: []u8,
    alloc: std.mem.Allocator,

    fn deinit(*_) {
        if (_.unique_hashes.len > 0) {
            _.alloc.free(_.unique_hashes);
            _.unique_hashes = [];
        }
        if (_.reverse_order.len > 0) {
            _.alloc.free(_.reverse_order);
            _.reverse_order = [];
        }
        if (_.reverse_h.len > 0) {
            _.alloc.free(_.reverse_h);
            _.reverse_h = [];
        }
    }
}

fn hashBytes(data: []const u8): u64 {
    return std.hash.Wyhash.hash(0, data);
}

fn mix64(x_in: u64): u64 {
    let x = x_in;
    x = (x ^ (x >> 30)) *% 0xbf58476d1ce4e5b9;
    x = (x ^ (x >> 27)) *% 0x94d049bb133111eb;
    x ^= x >> 31;
    return x;
}

fn computeSlots(key_hash: u64, seed: u64, block_length: usize): SlotTriple {
    const h1_mix = mix64(key_hash ^ seed);
    const h2_mix = mix64(h1_mix);
    const bl = block_length as u64;
    const r0 = h1_mix & 0xffffffff;
    const r1 = (h1_mix >> 32) & 0xffffffff;
    const r2 = h2_mix & 0xffffffff;
    const idx0 = ((r0 *% bl) >> 32) as usize;
    const idx1 = (((r1 *% bl) >> 32) as usize) + block_length;
    const idx2 = (((r2 *% bl) >> 32) as usize) + (block_length * 2);
    const fp16 = ((h2_mix >> 32) & 0xffff) as u16;
    return SlotTriple{
        h0: idx0,
        h1: idx1,
        h2: idx2,
        fp16: fp16,
    };
}

fn siftDown(arr: []u64, start: usize, end: usize) {
    let root = start;
    while ((root * 2 + 1) < end) {
        let child = root * 2 + 1;
        if ((child + 1) < end && arr[child] < arr[child + 1]) {
            child += 1;
        }
        if (arr[root] < arr[child]) {
            const tmp = arr[root];
            arr[root] = arr[child];
            arr[child] = tmp;
            root = child;
        } else {
            return;
        }
    }
}

fn sortU64(arr: []u64) {
    if (arr.len <= 1) return;
    let start = arr.len / 2;
    while (start > 0) {
        start -= 1;
        siftDown(arr, start, arr.len);
    }
    let end = arr.len;
    while (end > 1) {
        end -= 1;
        const tmp = arr[0];
        arr[0] = arr[end];
        arr[end] = tmp;
        siftDown(arr, 0, end);
    }
}

fn sortAndDedupHashes(allocator: std.mem.Allocator, hashes: []const u64): DedupResult ! XorFilterError {
    if (hashes.len == 0) {
        return DedupResult{ buf: [], unique_len: 0 };
    }
    let buf = allocator.alloc(u64, hashes.len) catch return XorFilterError.OutOfMemory;
    @memcpy(buf, hashes);
    sortU64(buf);

    let write_idx: usize = 1;
    let i: usize = 1;
    while (i < buf.len) : (i += 1) {
        if (buf[i] != buf[write_idx - 1]) {
            buf[write_idx] = buf[i];
            write_idx += 1;
        }
    }
    return DedupResult{ buf: buf, unique_len: write_idx };
}

fn computeBlockLength(unique_len: usize): usize ! XorFilterError {
    if (unique_len == 0) return 0;
    const max_u = std.math.maxInt(usize);
    if (unique_len > (max_u / 2)) {
        return XorFilterError.ConstructionFailed;
    }
    const capacity = (unique_len / 100) * 123 + ((unique_len % 100) * 123) / 100 + 32;
    const bl = (capacity + 2) / 3;
    if (bl == 0 || bl > (max_u / 3)) {
        return XorFilterError.ConstructionFailed;
    }
    return bl;
}

fn peelHashes(allocator: std.mem.Allocator, raw_hashes: []const u64): PeelPlan ! XorFilterError {
    if (raw_hashes.len == 0) {
        return PeelPlan{
            seed: INITIAL_SEED,
            block_length: 0,
            unique_len: 0,
            unique_hashes: [],
            reverse_order: [],
            reverse_h: [],
            alloc: allocator,
        };
    }

    const dedup = sortAndDedupHashes(allocator, raw_hashes) catch |e| return e;
    const unique_hashes = dedup.buf;
    const unique_len = dedup.unique_len;
    errdefer allocator.free(unique_hashes);

    const block_length = computeBlockLength(unique_len) catch |e| return e;
    const array_length = block_length * 3;

    let sets = allocator.alloc(T2Set, array_length) catch return XorFilterError.OutOfMemory;
    defer allocator.free(sets);

    let alone = allocator.alloc(usize, array_length) catch return XorFilterError.OutOfMemory;
    defer allocator.free(alone);

    let reverse_order = allocator.alloc(u64, unique_len) catch return XorFilterError.OutOfMemory;
    errdefer allocator.free(reverse_order);

    let reverse_h = allocator.alloc(u8, unique_len) catch return XorFilterError.OutOfMemory;
    errdefer allocator.free(reverse_h);

    let seed: u64 = INITIAL_SEED;
    let retry: usize = 0;
    while (retry < MAX_BUILD_RETRIES) : (retry += 1) {
        let s_idx: usize = 0;
        while (s_idx < array_length) : (s_idx += 1) {
            sets[s_idx] = T2Set{ count: 0, mask: 0 };
        }

        let k_idx: usize = 0;
        while (k_idx < unique_len) : (k_idx += 1) {
            const k = unique_hashes[k_idx];
            const slots = computeSlots(k, seed, block_length);
            sets[slots.h0].count += 1;
            sets[slots.h0].mask ^= k;
            sets[slots.h1].count += 1;
            sets[slots.h1].mask ^= k;
            sets[slots.h2].count += 1;
            sets[slots.h2].mask ^= k;
        }

        let alone_pos: usize = 0;
        let i: usize = 0;
        while (i < array_length) : (i += 1) {
            if (sets[i].count == 1) {
                alone[alone_pos] = i;
                alone_pos += 1;
            }
        }

        let rev_pos: usize = 0;
        while (alone_pos > 0) {
            alone_pos -= 1;
            const idx = alone[alone_pos];
            if (sets[idx].count == 0) continue;

            const k = sets[idx].mask;
            const slots = computeSlots(k, seed, block_length);
            let found: u8 = 2;
            if (idx == slots.h0) {
                found = 0;
            } else if (idx == slots.h1) {
                found = 1;
            }

            reverse_order[rev_pos] = k;
            reverse_h[rev_pos] = found;
            rev_pos += 1;

            sets[slots.h0].count -= 1;
            sets[slots.h0].mask ^= k;
            if (sets[slots.h0].count == 1) {
                alone[alone_pos] = slots.h0;
                alone_pos += 1;
            }

            sets[slots.h1].count -= 1;
            sets[slots.h1].mask ^= k;
            if (sets[slots.h1].count == 1) {
                alone[alone_pos] = slots.h1;
                alone_pos += 1;
            }

            sets[slots.h2].count -= 1;
            sets[slots.h2].mask ^= k;
            if (sets[slots.h2].count == 1) {
                alone[alone_pos] = slots.h2;
                alone_pos += 1;
            }
        }

        if (rev_pos == unique_len) {
            return PeelPlan{
                seed: seed,
                block_length: block_length,
                unique_len: unique_len,
                unique_hashes: unique_hashes,
                reverse_order: reverse_order,
                reverse_h: reverse_h,
                alloc: allocator,
            };
        }

        seed = mix64(seed +% 0x9e3779b97f4a7c15);
    }

    return XorFilterError.ConstructionFailed;
}

fn writeU32LE(buf: []u8, offset: usize, val: u32) {
    buf[offset + 0] = (val & 0xff) as u8;
    buf[offset + 1] = ((val >> 8) & 0xff) as u8;
    buf[offset + 2] = ((val >> 16) & 0xff) as u8;
    buf[offset + 3] = ((val >> 24) & 0xff) as u8;
}

fn writeU64LE(buf: []u8, offset: usize, val: u64) {
    buf[offset + 0] = (val & 0xff) as u8;
    buf[offset + 1] = ((val >> 8) & 0xff) as u8;
    buf[offset + 2] = ((val >> 16) & 0xff) as u8;
    buf[offset + 3] = ((val >> 24) & 0xff) as u8;
    buf[offset + 4] = ((val >> 32) & 0xff) as u8;
    buf[offset + 5] = ((val >> 40) & 0xff) as u8;
    buf[offset + 6] = ((val >> 48) & 0xff) as u8;
    buf[offset + 7] = ((val >> 56) & 0xff) as u8;
}

fn readU32LE(buf: []const u8, offset: usize): u32 {
    return (buf[offset + 0] as u32)
        | ((buf[offset + 1] as u32) << 8)
        | ((buf[offset + 2] as u32) << 16)
        | ((buf[offset + 3] as u32) << 24);
}

fn readU64LE(buf: []const u8, offset: usize): u64 {
    return (buf[offset + 0] as u64)
        | ((buf[offset + 1] as u64) << 8)
        | ((buf[offset + 2] as u64) << 16)
        | ((buf[offset + 3] as u64) << 24)
        | ((buf[offset + 4] as u64) << 32)
        | ((buf[offset + 5] as u64) << 40)
        | ((buf[offset + 6] as u64) << 48)
        | ((buf[offset + 7] as u64) << 56);
}

fn writeHeader(buf: []u8, magic: u32, seed: u64, block_length: usize, item_count: usize) {
    writeU32LE(buf, 0, magic);
    writeU32LE(buf, 4, FORMAT_VERSION);
    writeU64LE(buf, 8, seed);
    writeU64LE(buf, 16, block_length as u64);
    writeU64LE(buf, 24, item_count as u64);
}

fn parseHeader(data: []const u8, expected_magic: u32, elem_bytes: usize): HeaderInfo ! XorFilterError {
    if (data.len < HEADER_SIZE) {
        return XorFilterError.InvalidSerializedData;
    }
    const magic = readU32LE(data, 0);
    if (magic != expected_magic) {
        return XorFilterError.InvalidSerializedData;
    }
    const version = readU32LE(data, 4);
    if (version != FORMAT_VERSION) {
        return XorFilterError.InvalidSerializedData;
    }
    const seed = readU64LE(data, 8);
    const bl_u64 = readU64LE(data, 16);
    const cnt_u64 = readU64LE(data, 24);

    const max_u = std.math.maxInt(usize) as u64;
    if (bl_u64 > max_u || cnt_u64 > max_u) {
        return XorFilterError.InvalidSerializedData;
    }
    const block_length = bl_u64 as usize;
    const item_count = cnt_u64 as usize;

    if (item_count > 0 && block_length == 0) {
        return XorFilterError.InvalidSerializedData;
    }
    const max_bl = std.math.maxInt(usize) / (3 * elem_bytes);
    if (block_length > max_bl) {
        return XorFilterError.InvalidSerializedData;
    }
    const array_length = block_length * 3;
    const payload_bytes = array_length * elem_bytes;
    if (data.len != HEADER_SIZE + payload_bytes) {
        return XorFilterError.InvalidSerializedData;
    }
    return HeaderInfo{
        seed: seed,
        block_length: block_length,
        item_count: item_count,
        array_length: array_length,
    };
}

/// Static 8-bit fingerprint Xor Filter (~9.84 bits/key, ~0.39% false positive rate).
pub struct Xor8Filter {
    alloc: std.mem.Allocator,
    seed: u64,
    block_length: usize,
    item_count: usize,
    fingerprints: []u8,

    /// Build an Xor8Filter from a slice of byte keys. Duplicate keys are automatically deduplicated.
    pub fn build(allocator: std.mem.Allocator, keys: []const []const u8): Xor8Filter ! XorFilterError {
        if (keys.len == 0) {
            return Xor8Filter{
                alloc: allocator,
                seed: INITIAL_SEED,
                block_length: 0,
                item_count: 0,
                fingerprints: [],
            };
        }
        let hashes = allocator.alloc(u64, keys.len) catch return XorFilterError.OutOfMemory;
        defer allocator.free(hashes);

        let i: usize = 0;
        while (i < keys.len) : (i += 1) {
            hashes[i] = hashBytes(keys[i]);
        }
        return Xor8Filter.buildHashes(allocator, hashes);
    }

    /// Build an Xor8Filter directly from 64-bit hashes. Duplicate hashes are automatically deduplicated.
    pub fn buildHashes(allocator: std.mem.Allocator, hashes: []const u64): Xor8Filter ! XorFilterError {
        if (hashes.len == 0) {
            return Xor8Filter{
                alloc: allocator,
                seed: INITIAL_SEED,
                block_length: 0,
                item_count: 0,
                fingerprints: [],
            };
        }

        let plan = peelHashes(allocator, hashes) catch |e| return e;
        defer plan.deinit();

        const array_length = plan.block_length * 3;
        let fps = allocator.alloc(u8, array_length) catch return XorFilterError.OutOfMemory;
        @memset(fps, 0);

        let i = plan.unique_len;
        while (i > 0) {
            i -= 1;
            const k = plan.reverse_order[i];
            const found = plan.reverse_h[i];
            const slots = computeSlots(k, plan.seed, plan.block_length);
            const fp = (slots.fp16 & 0xff) as u8;
            if (found == 0) {
                fps[slots.h0] = fp ^ fps[slots.h1] ^ fps[slots.h2];
            } else if (found == 1) {
                fps[slots.h1] = fp ^ fps[slots.h0] ^ fps[slots.h2];
            } else {
                fps[slots.h2] = fp ^ fps[slots.h0] ^ fps[slots.h1];
            }
        }

        return Xor8Filter{
            alloc: allocator,
            seed: plan.seed,
            block_length: plan.block_length,
            item_count: plan.unique_len,
            fingerprints: fps,
        };
    }

    /// Check whether `key` may be in the set (O(1), 3 array accesses, zero false negatives).
    pub fn contains(key: []const u8): bool {
        if (_.block_length == 0) return false;
        return _.containsHash(hashBytes(key));
    }

    /// Check whether a raw 64-bit hash `h` may be in the set (O(1), 3 array accesses).
    pub fn containsHash(h: u64): bool {
        if (_.block_length == 0) return false;
        const slots = computeSlots(h, _.seed, _.block_length);
        const fp = (slots.fp16 & 0xff) as u8;
        const val = _.fingerprints[slots.h0] ^ _.fingerprints[slots.h1] ^ _.fingerprints[slots.h2];
        return val == fp;
    }

    /// Serialize this filter into a self-contained binary buffer with magic & version header.
    pub fn serialize(allocator: std.mem.Allocator): []u8 ! XorFilterError {
        const total = HEADER_SIZE + _.fingerprints.len;
        let out = allocator.alloc(u8, total) catch return XorFilterError.OutOfMemory;
        writeHeader(out, XOR8_MAGIC, _.seed, _.block_length, _.item_count);
        if (_.fingerprints.len > 0) {
            @memcpy(out[HEADER_SIZE..total], _.fingerprints);
        }
        return out;
    }

    /// Deserialize an Xor8Filter from a binary buffer, validating magic, version, and exact byte length.
    pub fn deserialize(allocator: std.mem.Allocator, data: []const u8): Xor8Filter ! XorFilterError {
        const hdr = parseHeader(data, XOR8_MAGIC, 1) catch |e| return e;
        if (hdr.array_length == 0) {
            return Xor8Filter{
                alloc: allocator,
                seed: hdr.seed,
                block_length: 0,
                item_count: hdr.item_count,
                fingerprints: [],
            };
        }
        let fps = allocator.alloc(u8, hdr.array_length) catch return XorFilterError.OutOfMemory;
        @memcpy(fps, data[HEADER_SIZE..data.len]);
        return Xor8Filter{
            alloc: allocator,
            seed: hdr.seed,
            block_length: hdr.block_length,
            item_count: hdr.item_count,
            fingerprints: fps,
        };
    }

    /// Return the number of unique keys stored in the filter.
    pub fn len(): usize {
        return _.item_count;
    }

    /// Return the backing fingerprint storage size in bytes.
    pub fn byteSize(): usize {
        return _.fingerprints.len;
    }

    /// Return the average bits per unique key.
    pub fn bitsPerKey(): f64 {
        if (_.item_count == 0) return 0.0;
        return ((_.byteSize() * 8) as f64) / (_.item_count as f64);
    }

    /// Release backing fingerprint storage.
    pub fn deinit(*_) {
        if (_.fingerprints.len > 0) {
            _.alloc.free(_.fingerprints);
            _.fingerprints = [];
        }
        _.block_length = 0;
        _.item_count = 0;
    }
}

/// Static 16-bit fingerprint Xor Filter (~19.68 bits/key, ~0.0015% false positive rate).
pub struct Xor16Filter {
    alloc: std.mem.Allocator,
    seed: u64,
    block_length: usize,
    item_count: usize,
    fingerprints: []u16,

    /// Build an Xor16Filter from a slice of byte keys. Duplicate keys are automatically deduplicated.
    pub fn build(allocator: std.mem.Allocator, keys: []const []const u8): Xor16Filter ! XorFilterError {
        if (keys.len == 0) {
            return Xor16Filter{
                alloc: allocator,
                seed: INITIAL_SEED,
                block_length: 0,
                item_count: 0,
                fingerprints: [],
            };
        }
        let hashes = allocator.alloc(u64, keys.len) catch return XorFilterError.OutOfMemory;
        defer allocator.free(hashes);

        let i: usize = 0;
        while (i < keys.len) : (i += 1) {
            hashes[i] = hashBytes(keys[i]);
        }
        return Xor16Filter.buildHashes(allocator, hashes);
    }

    /// Build an Xor16Filter directly from 64-bit hashes. Duplicate hashes are automatically deduplicated.
    pub fn buildHashes(allocator: std.mem.Allocator, hashes: []const u64): Xor16Filter ! XorFilterError {
        if (hashes.len == 0) {
            return Xor16Filter{
                alloc: allocator,
                seed: INITIAL_SEED,
                block_length: 0,
                item_count: 0,
                fingerprints: [],
            };
        }

        let plan = peelHashes(allocator, hashes) catch |e| return e;
        defer plan.deinit();

        const array_length = plan.block_length * 3;
        let fps = allocator.alloc(u16, array_length) catch return XorFilterError.OutOfMemory;
        @memset(fps, 0);

        let i = plan.unique_len;
        while (i > 0) {
            i -= 1;
            const k = plan.reverse_order[i];
            const found = plan.reverse_h[i];
            const slots = computeSlots(k, plan.seed, plan.block_length);
            const fp = slots.fp16;
            if (found == 0) {
                fps[slots.h0] = fp ^ fps[slots.h1] ^ fps[slots.h2];
            } else if (found == 1) {
                fps[slots.h1] = fp ^ fps[slots.h0] ^ fps[slots.h2];
            } else {
                fps[slots.h2] = fp ^ fps[slots.h0] ^ fps[slots.h1];
            }
        }

        return Xor16Filter{
            alloc: allocator,
            seed: plan.seed,
            block_length: plan.block_length,
            item_count: plan.unique_len,
            fingerprints: fps,
        };
    }

    /// Check whether `key` may be in the set (O(1), 3 array accesses, zero false negatives).
    pub fn contains(key: []const u8): bool {
        if (_.block_length == 0) return false;
        return _.containsHash(hashBytes(key));
    }

    /// Check whether a raw 64-bit hash `h` may be in the set (O(1), 3 array accesses).
    pub fn containsHash(h: u64): bool {
        if (_.block_length == 0) return false;
        const slots = computeSlots(h, _.seed, _.block_length);
        const fp = slots.fp16;
        const val = _.fingerprints[slots.h0] ^ _.fingerprints[slots.h1] ^ _.fingerprints[slots.h2];
        return val == fp;
    }

    /// Serialize this filter into a self-contained binary buffer with magic & version header.
    pub fn serialize(allocator: std.mem.Allocator): []u8 ! XorFilterError {
        const payload_bytes = _.fingerprints.len * 2;
        const total = HEADER_SIZE + payload_bytes;
        let out = allocator.alloc(u8, total) catch return XorFilterError.OutOfMemory;
        writeHeader(out, XOR16_MAGIC, _.seed, _.block_length, _.item_count);
        let i: usize = 0;
        while (i < _.fingerprints.len) : (i += 1) {
            const v = _.fingerprints[i];
            const off = HEADER_SIZE + i * 2;
            out[off + 0] = (v & 0xff) as u8;
            out[off + 1] = ((v >> 8) & 0xff) as u8;
        }
        return out;
    }

    /// Deserialize an Xor16Filter from a binary buffer, validating magic, version, and exact byte length.
    pub fn deserialize(allocator: std.mem.Allocator, data: []const u8): Xor16Filter ! XorFilterError {
        const hdr = parseHeader(data, XOR16_MAGIC, 2) catch |e| return e;
        if (hdr.array_length == 0) {
            return Xor16Filter{
                alloc: allocator,
                seed: hdr.seed,
                block_length: 0,
                item_count: hdr.item_count,
                fingerprints: [],
            };
        }
        let fps = allocator.alloc(u16, hdr.array_length) catch return XorFilterError.OutOfMemory;
        let i: usize = 0;
        while (i < hdr.array_length) : (i += 1) {
            const off = HEADER_SIZE + i * 2;
            fps[i] = (data[off + 0] as u16) | ((data[off + 1] as u16) << 8);
        }
        return Xor16Filter{
            alloc: allocator,
            seed: hdr.seed,
            block_length: hdr.block_length,
            item_count: hdr.item_count,
            fingerprints: fps,
        };
    }

    /// Return the number of unique keys stored in the filter.
    pub fn len(): usize {
        return _.item_count;
    }

    /// Return the backing fingerprint storage size in bytes.
    pub fn byteSize(): usize {
        return _.fingerprints.len * 2;
    }

    /// Return the average bits per unique key.
    pub fn bitsPerKey(): f64 {
        if (_.item_count == 0) return 0.0;
        return ((_.byteSize() * 8) as f64) / (_.item_count as f64);
    }

    /// Release backing fingerprint storage.
    pub fn deinit(*_) {
        if (_.fingerprints.len > 0) {
            _.alloc.free(_.fingerprints);
            _.fingerprints = [];
        }
        _.block_length = 0;
        _.item_count = 0;
    }
}
