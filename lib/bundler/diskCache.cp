// Source: Absorbed from /data/dev/bun/src/bundler/cache.cp | Synced: 2026-11-23T14:41:07Z
// ============================================================================
// lib/bundler/diskCache.cp — Build Disk Cache Index, Content Keying & Snapshot Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven disk cache engine for `cap build`:
//   1. Deterministic 128-bit content-addressed build cache key derivation
//      (`computeBuildCacheKey`) fusing source hash, dependency tree hash,
//      compiler version/ABI, target, define hash, and loader flags, with
//      two-level sharded relative path formatting (`ab/cd1234...bin`);
//   2. Zero-heap caller-slice-backed cache catalog (`BuildDiskCacheIndex`)
//      supporting `lookup` (touching LRU timestamp + verifying mtime/size/ABI/TTL),
//      `upsert`, targeted `invalidate` (by key/source/domain/ABI), and precise
//      LRU + TTL + byte/entry quota `planEviction` / `applyEvictionPlan`;
//   3. IEEE 802.3 CRC32-verified binary catalog snapshot codec
//      (`encodeDiskCacheIndex` / `decodeDiskCacheIndex`) hardened against
//      truncated, corrupted, or overflowed inputs;
//   4. `cap:simd`-accelerated path normalization, ASCII key pre-validation, and
//      256-shard load distribution analysis (`analyzeShardDistribution`).

import { Simd } from "cap:simd";

/// Four-byte magic prefix for binary disk cache catalog snapshots ("CPDC").
pub const DISK_CACHE_INDEX_MAGIC: [4]u8 = ['C', 'P', 'D', 'C'];

/// Binary format version for disk cache catalog snapshots.
pub const DISK_CACHE_INDEX_VERSION: u16 = 1;

/// Fixed byte length of the binary catalog snapshot header.
pub const DISK_CACHE_HEADER_BYTES: usize = 32;

/// Fixed byte length of each serialized cache entry record.
pub const DISK_CACHE_RECORD_BYTES: usize = 96;

/// Character length of a 128-bit cache key encoded as lowercase hexadecimal.
pub const DISK_CACHE_KEY_HEX_LEN: usize = 32;

/// Number of top-level directory shards (00..ff).
pub const DISK_CACHE_SHARD_COUNT: usize = 256;

/// Error set for build disk cache keying, indexing, eviction, and codec operations.
pub err DiskCacheError {
    BufferTooSmall,
    CapacityExceeded,
    InvalidMagic,
    UnsupportedVersion,
    ChecksumMismatch,
    TruncatedData,
    CorruptRecord,
    InvalidKeyInput,
    InvalidShardPath,
    InvalidPolicy,
}

// ============================================================================
// Internal Zero-Alloc Byte, Endian, Hash, and Formatting Helpers
// ============================================================================

const HEX_LOWER: [16]u8 = [
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
];

/// Nibble lookup table for IEEE 802.3 CRC32 (polynomial 0xEDB88320).
const CRC32_NIBBLE_TABLE: [16]u32 = [
    0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
    0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
    0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
    0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C,
];

fn dcMix64(x: u64): u64 {
    let z = x +% 0x9E3779B97F4A7C15;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn dcHashUpdateU64(state: u64, value: u64): u64 {
    let h = state ^ dcMix64(value);
    h = (h *% 0x100000001B3) +% 0x517CC1B727220A95;
    return h ^ (h >> 29);
}

fn dcHashUpdateBytes(state: u64, data: []const u8): u64 {
    let h = dcHashUpdateU64(state, @as(u64, data.len));
    let i: usize = 0;
    while (i + 8 <= data.len) {
        const w = dcReadU64LeUnchecked(data, i);
        h = dcHashUpdateU64(h, w);
        i += 8;
    }
    if (i < data.len) {
        let tail: u64 = 0;
        let shift: u6 = 0;
        while (i < data.len) {
            tail |= @as(u64, data[i]) << shift;
            shift += 8;
            i += 1;
        }
        h = dcHashUpdateU64(h, tail ^ 0xA5A5A5A5A5A5A5A5);
    }
    return h;
}

fn dcWriteU16Le(out: []u8, offset: usize, v: u16) {
    out[offset] = @as(u8, @truncate(v));
    out[offset + 1] = @as(u8, @truncate(v >> 8));
}

fn dcWriteU32Le(out: []u8, offset: usize, v: u32) {
    out[offset] = @as(u8, @truncate(v));
    out[offset + 1] = @as(u8, @truncate(v >> 8));
    out[offset + 2] = @as(u8, @truncate(v >> 16));
    out[offset + 3] = @as(u8, @truncate(v >> 24));
}

fn dcWriteU64Le(out: []u8, offset: usize, v: u64) {
    out[offset] = @as(u8, @truncate(v));
    out[offset + 1] = @as(u8, @truncate(v >> 8));
    out[offset + 2] = @as(u8, @truncate(v >> 16));
    out[offset + 3] = @as(u8, @truncate(v >> 24));
    out[offset + 4] = @as(u8, @truncate(v >> 32));
    out[offset + 5] = @as(u8, @truncate(v >> 40));
    out[offset + 6] = @as(u8, @truncate(v >> 48));
    out[offset + 7] = @as(u8, @truncate(v >> 56));
}

fn dcReadU16Le(data: []const u8, offset: usize): u16 {
    return @as(u16, data[offset]) | (@as(u16, data[offset + 1]) << 8);
}

fn dcReadU32Le(data: []const u8, offset: usize): u32 {
    return @as(u32, data[offset]) |
        (@as(u32, data[offset + 1]) << 8) |
        (@as(u32, data[offset + 2]) << 16) |
        (@as(u32, data[offset + 3]) << 24);
}

fn dcReadU64LeUnchecked(data: []const u8, offset: usize): u64 {
    return @as(u64, data[offset]) |
        (@as(u64, data[offset + 1]) << 8) |
        (@as(u64, data[offset + 2]) << 16) |
        (@as(u64, data[offset + 3]) << 24) |
        (@as(u64, data[offset + 4]) << 32) |
        (@as(u64, data[offset + 5]) << 40) |
        (@as(u64, data[offset + 6]) << 48) |
        (@as(u64, data[offset + 7]) << 56);
}

fn dcDecodeHexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn dcAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn dcAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn dcAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        dcAppendByte(out, pos, '0')?;
        return;
    }
    while (v > 0) {
        const d: u8 = @as(u8, @intCast(v % 10));
        tmp[n] = @as(u8, '0') + d;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        dcAppendByte(out, pos, tmp[n])?;
    }
}

fn dcAppendHexU64(out: []u8, pos: *usize, val: u64)! {
    if (16 > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < 16) {
        const shift: u6 = @as(u6, @intCast(60 - i * 4));
        const nib: usize = @as(usize, @intCast((val >> shift) & 0xF));
        out[pos.* + i] = HEX_LOWER[nib];
        i += 1;
    }
    pos.* += 16;
}

fn dcAppendHexByte(out: []u8, pos: *usize, val: u8)! {
    if (2 > out.len - pos.*) return err.BufferTooSmall;
    out[pos.*] = HEX_LOWER[@as(usize, val >> 4)];
    out[pos.* + 1] = HEX_LOWER[@as(usize, val & 0xF)];
    pos.* += 2;
}

// ============================================================================
// Section 1: IEEE 802.3 CRC32 & SIMD-Accelerated Path / Key Primitives
// ============================================================================

/// Incrementally updates an IEEE 802.3 CRC32 state (pass `0` as initial value).
pub fn updateDiskCacheCrc32(initial_crc: u32, data: []const u8): u32 {
    let crc: u32 = initial_crc ^ 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) {
        const b: u32 = @as(u32, data[i]);
        crc = (crc >> 4) ^ CRC32_NIBBLE_TABLE[@as(usize, (crc ^ b) & 0xF)];
        crc = (crc >> 4) ^ CRC32_NIBBLE_TABLE[@as(usize, (crc ^ (b >> 4)) & 0xF)];
        i += 1;
    }
    return crc ^ 0xFFFFFFFF;
}

/// Computes the IEEE 802.3 CRC32 checksum of `data`.
pub fn computeDiskCacheCrc32(data: []const u8): u32 {
    return updateDiskCacheCrc32(0, data);
}

/// Computes a deterministic 64-bit content hash suitable for source or dependency digests.
pub fn computeDiskCacheHash64(seed: u64, data: []const u8): u64 {
    const h = dcHashUpdateBytes(seed ^ 0xCBF29CE484222325, data);
    return dcMix64(h);
}

/// Normalizes a source path into canonical forward-slash form using `cap:simd`
/// (`Simd.indexOfAny2`) to bulk-scan non-separator spans. Rejects empty paths,
/// NUL bytes, and parent-directory traversal (`..`) segments.
pub fn normalizeCacheRelPath(raw: []const u8, out: []u8): DiskCacheError!usize {
    if (raw.len == 0) return err.InvalidKeyInput;
    if (Simd.indexOfByte(raw, 0) != null) return err.InvalidKeyInput;

    let pos: usize = 0;
    let out_pos: usize = 0;

    while (pos < raw.len) {
        // Skip leading or repeated separators
        while (pos < raw.len && (raw[pos] == '/' || raw[pos] == '\\')) {
            pos += 1;
        }
        if (pos >= raw.len) break;

        // SIMD scan for the next path separator
        let seg_end = raw.len;
        if (rel := Simd.indexOfAny2(raw[pos..], '/', '\\')) {
            seg_end = pos + rel;
        }
        const seg = raw[pos..seg_end];
        pos = seg_end;

        if (seg.len == 1 && seg[0] == '.') {
            continue;
        }
        if (seg.len == 2 && seg[0] == '.' && seg[1] == '.') {
            return err.InvalidKeyInput;
        }

        if (out_pos > 0) {
            dcAppendByte(out, &out_pos, '/')?;
        }
        dcAppendBytes(out, &out_pos, seg)?;
    }

    if (out_pos == 0) return err.InvalidKeyInput;
    return out_pos;
}

/// Validates whether `hex` is a 32-character ASCII hexadecimal cache key,
/// using `Simd.isAscii` as a fast pre-filter.
pub fn isValidCacheKeyHex(hex: []const u8): bool {
    if (hex.len != DISK_CACHE_KEY_HEX_LEN) return false;
    if (!Simd.isAscii(hex)) return false;
    let i: usize = 0;
    while (i < hex.len) {
        if (dcDecodeHexNibble(hex[i]) == null) return false;
        i += 1;
    }
    return true;
}

// ============================================================================
// Section 2: Content-Addressed Build Cache Key Derivation & Shard Paths
// ============================================================================

/// Logical cache domain matching `bundler/cache.cp` partitions.
pub enum DiskCacheDomain {
    JavaScript,
    TypeScript,
    Json,
    Css,
    FileSystem,
    Asset,
    Macro,
}

/// Converts a `DiskCacheDomain` to its stable wire byte tag.
pub fn diskCacheDomainToByte(domain: DiskCacheDomain): u8 {
    return match (domain) {
        DiskCacheDomain.JavaScript => 0,
        DiskCacheDomain.TypeScript => 1,
        DiskCacheDomain.Json => 2,
        DiskCacheDomain.Css => 3,
        DiskCacheDomain.FileSystem => 4,
        DiskCacheDomain.Asset => 5,
        DiskCacheDomain.Macro => 6,
    };
}

/// Decodes a wire byte tag into `DiskCacheDomain`, returning `null` if unknown.
pub fn diskCacheDomainFromByte(tag: u8): DiskCacheDomain? {
    if (tag == 0) return DiskCacheDomain.JavaScript;
    if (tag == 1) return DiskCacheDomain.TypeScript;
    if (tag == 2) return DiskCacheDomain.Json;
    if (tag == 3) return DiskCacheDomain.Css;
    if (tag == 4) return DiskCacheDomain.FileSystem;
    if (tag == 5) return DiskCacheDomain.Asset;
    if (tag == 6) return DiskCacheDomain.Macro;
    return null;
}

/// Returns the canonical ASCII name of a `DiskCacheDomain`.
pub fn diskCacheDomainName(domain: DiskCacheDomain): []const u8 {
    return match (domain) {
        DiskCacheDomain.JavaScript => "js",
        DiskCacheDomain.TypeScript => "ts",
        DiskCacheDomain.Json => "json",
        DiskCacheDomain.Css => "css",
        DiskCacheDomain.FileSystem => "fs",
        DiskCacheDomain.Asset => "asset",
        DiskCacheDomain.Macro => "macro",
    };
}

/// Input parameters fused into a 128-bit deterministic build cache key.
pub struct BuildCacheKeyInput {
    domain: DiskCacheDomain = DiskCacheDomain.JavaScript,
    source_path: []const u8 = "",
    source_hash: u64 = 0,
    dep_tree_hash: u64 = 0,
    compiler_version: []const u8 = "",
    compiler_abi: u32 = 1,
    target_name: []const u8 = "",
    define_hash: u64 = 0,
    loader_flags: u64 = 0,
    minify: bool = false,
    sourcemap: bool = false,
    extra_salt: u64 = 0,
}

/// 128-bit content-addressed build cache key with two-level shard prefixes.
pub struct BuildCacheKey {
    hi: u64 = 0,
    lo: u64 = 0,

    static fn zero(): BuildCacheKey {
        return BuildCacheKey{ hi: 0, lo: 0 };
    }

    fn isZero(): bool {
        return _.hi == 0 && _.lo == 0;
    }

    fn eql(other: BuildCacheKey): bool {
        return _.hi == other.hi && _.lo == other.lo;
    }

    /// Returns a mixed 64-bit digest derived from both 64-bit halves.
    fn shortHash64(): u64 {
        return dcMix64(_.hi ^ dcMix64(_.lo));
    }

    /// Top-level shard directory index (`0..255`), taken from the most significant byte of `hi`.
    fn shardByte(): u8 {
        return @as(u8, @truncate(_.hi >> 56));
    }

    /// Second-level shard byte (`0..255`), taken from bits 48..55 of `hi`.
    fn subShardByte(): u8 {
        return @as(u8, @truncate(_.hi >> 48));
    }

    /// Formats the 128-bit key as 32 lowercase hexadecimal characters.
    fn formatHex(out: []u8): DiskCacheError!usize {
        let pos: usize = 0;
        dcAppendHexU64(out, &pos, _.hi)?;
        dcAppendHexU64(out, &pos, _.lo)?;
        return pos;
    }

    /// Formats a two-level sharded relative path such as `ab/cd1234...bin`.
    /// `ext` defaults to `".bin"` when empty; must not contain path separators.
    fn formatShardPath(ext: []const u8, out: []u8): DiskCacheError!usize {
        const suffix = if (ext.len == 0) ".bin" else ext;
        if (Simd.indexOfAny2(suffix, '/', '\\') != null) return err.InvalidShardPath;
        let pos: usize = 0;
        dcAppendHexByte(out, &pos, _.shardByte())?;
        dcAppendByte(out, &pos, '/')?;
        dcAppendHexU64(out, &pos, _.hi)?;
        dcAppendHexU64(out, &pos, _.lo)?;
        dcAppendBytes(out, &pos, suffix)?;
        return pos;
    }
}

/// Derives a deterministic 128-bit content-addressed cache key by fusing
/// source hash, dependency tree hash, compiler version/ABI, target, define hash,
/// loader flags, and normalized source path into two independent 64-bit lanes.
pub fn computeBuildCacheKey(input: BuildCacheKeyInput): DiskCacheError!BuildCacheKey {
    if (input.source_path.len == 0 || input.compiler_version.len == 0 || input.target_name.len == 0) {
        return err.InvalidKeyInput;
    }
    if (Simd.indexOfByte(input.source_path, 0) != null) {
        return err.InvalidKeyInput;
    }

    const domain_tag: u64 = @as(u64, diskCacheDomainToByte(input.domain));
    const opt_bits: u64 = (if (input.minify) @as(u64, 1) else @as(u64, 0)) |
        (if (input.sourcemap) @as(u64, 2) else @as(u64, 0)) |
        (@as(u64, input.compiler_abi) << 8) |
        (domain_tag << 40);

    // Lane 1 (hi): domain-separated seed A
    let h1: u64 = 0x243F6A8885A308D3;
    h1 = dcHashUpdateU64(h1, opt_bits);
    h1 = dcHashUpdateU64(h1, input.source_hash);
    h1 = dcHashUpdateU64(h1, input.dep_tree_hash);
    h1 = dcHashUpdateU64(h1, input.define_hash);
    h1 = dcHashUpdateU64(h1, input.loader_flags);
    h1 = dcHashUpdateU64(h1, input.extra_salt);
    h1 = dcHashUpdateBytes(h1, input.compiler_version);
    h1 = dcHashUpdateBytes(h1, input.target_name);
    h1 = dcHashUpdateBytes(h1, input.source_path);

    // Lane 2 (lo): domain-separated seed B with reversed field order for avalanche independence
    let h2: u64 = 0x13198A2E03707344;
    h2 = dcHashUpdateBytes(h2, input.source_path);
    h2 = dcHashUpdateBytes(h2, input.target_name);
    h2 = dcHashUpdateBytes(h2, input.compiler_version);
    h2 = dcHashUpdateU64(h2, input.extra_salt ^ 0xF0F0F0F0F0F0F0F0);
    h2 = dcHashUpdateU64(h2, input.loader_flags);
    h2 = dcHashUpdateU64(h2, input.define_hash);
    h2 = dcHashUpdateU64(h2, input.dep_tree_hash);
    h2 = dcHashUpdateU64(h2, input.source_hash);
    h2 = dcHashUpdateU64(h2, opt_bits ^ 0xA5A5A5A5A5A5A5A5);

    const hi = dcMix64(h1 ^ (h2 >> 17));
    const lo = dcMix64(h2 ^ (h1 << 13));

    // Guarantee non-zero key even in vanishingly rare hash-zero edge case
    if (hi == 0 && lo == 0) {
        return BuildCacheKey{ hi: 1, lo: 1 };
    }
    return BuildCacheKey{ hi: hi, lo: lo };
}

/// Parses a 32-character hexadecimal string back into a `BuildCacheKey`.
pub fn parseBuildCacheKeyHex(hex: []const u8): DiskCacheError!BuildCacheKey {
    if (!isValidCacheKeyHex(hex)) return err.InvalidKeyInput;
    let hi: u64 = 0;
    let lo: u64 = 0;
    let i: usize = 0;
    while (i < 16) {
        if (nib := dcDecodeHexNibble(hex[i])) {
            hi = (hi << 4) | @as(u64, nib);
        } else {
            return err.InvalidKeyInput;
        }
        i += 1;
    }
    while (i < 32) {
        if (nib := dcDecodeHexNibble(hex[i])) {
            lo = (lo << 4) | @as(u64, nib);
        } else {
            return err.InvalidKeyInput;
        }
        i += 1;
    }
    return BuildCacheKey{ hi: hi, lo: lo };
}

/// Parses a two-level sharded cache path (e.g. `ab/abcdef...bin`) using `Simd.indexOfByte`
/// and verifies that the directory prefix matches the key's top shard byte.
pub fn parseShardCachePath(rel_path: []const u8): DiskCacheError!BuildCacheKey {
    const slash_opt = Simd.indexOfByte(rel_path, '/');
    if (slash_opt == null) return err.InvalidShardPath;
    const slash = slash_opt ?? 0;
    if (slash != 2) return err.InvalidShardPath;
    if (rel_path.len < 3 + DISK_CACHE_KEY_HEX_LEN) return err.InvalidShardPath;

    const shard_hi_opt = dcDecodeHexNibble(rel_path[0]);
    const shard_lo_opt = dcDecodeHexNibble(rel_path[1]);
    if (shard_hi_opt == null || shard_lo_opt == null) return err.InvalidShardPath;
    const prefix_byte: u8 = ((shard_hi_opt ?? 0) << 4) | (shard_lo_opt ?? 0);

    const rest = rel_path[3..];
    const hex_slice = rest[0..DISK_CACHE_KEY_HEX_LEN];
    const key = parseBuildCacheKeyHex(hex_slice) catch return err.InvalidShardPath;
    if (key.shardByte() != prefix_byte) return err.InvalidShardPath;

    // Optional extension must start with '.' if present
    if (rest.len > DISK_CACHE_KEY_HEX_LEN && rest[DISK_CACHE_KEY_HEX_LEN] != '.') {
        return err.InvalidShardPath;
    }
    return key;
}

// ============================================================================
// Section 3: Zero-Heap Build Disk Cache Index & LRU/Quota Eviction Planner
// ============================================================================

/// Single metadata record tracked inside `BuildDiskCacheIndex`.
pub struct DiskCacheEntry {
    key: BuildCacheKey = BuildCacheKey.zero(),
    domain: DiskCacheDomain = DiskCacheDomain.JavaScript,
    source_hash: u64 = 0,
    dep_tree_hash: u64 = 0,
    path_hash: u64 = 0,
    mtime_ns: i64 = 0,
    source_size: u64 = 0,
    artifact_size: u64 = 0,
    created_at_sec: u64 = 0,
    last_access_sec: u64 = 0,
    compiler_abi: u32 = 0,
    hit_count: u32 = 0,
    artifact_crc32: u32 = 0,
    flags: u8 = 0,
    pinned: bool = false,
    occupied: bool = false,

    static fn empty(): DiskCacheEntry {
        return DiskCacheEntry{};
    }
}

/// Validation query passed to `BuildDiskCacheIndex.lookup`.
pub struct DiskCacheLookupQuery {
    key: BuildCacheKey = BuildCacheKey.zero(),
    expected_mtime_ns: i64 = 0,
    expected_source_size: u64 = 0,
    expected_compiler_abi: u32 = 0,
    now_sec: u64 = 0,
    ttl_secs: u64 = 0,
    check_mtime: bool = true,
    check_size: bool = true,
}

/// Outcome classification of a disk cache lookup.
pub enum DiskCacheMissReason {
    Hit,
    NotFound,
    CompilerAbiMismatch,
    MtimeMismatch,
    SizeMismatch,
    ExpiredTtl,
}

/// Result returned by `BuildDiskCacheIndex.lookup`.
pub struct DiskCacheLookupResult {
    status: DiskCacheMissReason = DiskCacheMissReason.NotFound,
    slot_index: usize = 0,
    entry: DiskCacheEntry = DiskCacheEntry.empty(),

    fn isHit(): bool {
        return _.status == DiskCacheMissReason.Hit;
    }
}

/// Policy thresholds governing `BuildDiskCacheIndex.planEviction`.
pub struct DiskCacheEvictionPolicy {
    max_bytes: u64 = 0,
    max_entries: usize = 0,
    ttl_secs: u64 = 0,
    now_sec: u64 = 0,
    respect_pinned: bool = true,
}

/// Reason a cache entry was selected for eviction.
pub enum DiskCacheEvictionReason {
    ExpiredTtl,
    OverByteQuota,
    OverEntryQuota,
}

/// Single planned eviction action returned in caller-provided buffer.
pub struct DiskCacheEvictionItem {
    key: BuildCacheKey = BuildCacheKey.zero(),
    slot_index: usize = 0,
    artifact_size: u64 = 0,
    last_access_sec: u64 = 0,
    reason: DiskCacheEvictionReason = DiskCacheEvictionReason.OverByteQuota,

    static fn empty(): DiskCacheEvictionItem {
        return DiskCacheEvictionItem{};
    }
}

/// Summary metrics produced by `BuildDiskCacheIndex.planEviction`.
pub struct DiskCacheEvictionPlan {
    evicted_count: usize = 0,
    reclaimed_bytes: u64 = 0,
    expired_count: usize = 0,
    quota_evicted_count: usize = 0,
    remaining_entries: usize = 0,
    remaining_bytes: u64 = 0,
}

/// Cumulative runtime telemetry for `BuildDiskCacheIndex`.
pub struct DiskCacheIndexStats {
    active_entries: usize = 0,
    capacity: usize = 0,
    total_artifact_bytes: u64 = 0,
    lookup_hits: u64 = 0,
    lookup_misses: u64 = 0,
    abi_mismatches: u64 = 0,
    mtime_mismatches: u64 = 0,
    size_mismatches: u64 = 0,
    ttl_expirations: u64 = 0,
    upserts: u64 = 0,
    invalidations: u64 = 0,
    evictions: u64 = 0,
}

/// SIMD-reduced shard distribution metrics across the 256 top-level shards.
pub struct DiskCacheShardDistribution {
    non_empty_shards: u32 = 0,
    min_per_shard: i32 = 0,
    max_per_shard: i32 = 0,
    total_entries: i32 = 0,
}

fn dcIsExpired(entry: DiskCacheEntry, now_sec: u64, ttl_secs: u64): bool {
    if (ttl_secs == 0) return false;
    if (now_sec < entry.last_access_sec) return false;
    return (now_sec - entry.last_access_sec) > ttl_secs;
}

/// Strict weak ordering for deterministic LRU victim selection:
/// older `last_access_sec` first, then lower `hit_count`, then larger `artifact_size`,
/// then smaller `key.lo` / `key.hi` as deterministic tie-breaker.
fn dcEvictBefore(a: DiskCacheEntry, b: DiskCacheEntry): bool {
    if (a.last_access_sec != b.last_access_sec) return a.last_access_sec < b.last_access_sec;
    if (a.hit_count != b.hit_count) return a.hit_count < b.hit_count;
    if (a.artifact_size != b.artifact_size) return a.artifact_size > b.artifact_size;
    if (a.key.lo != b.key.lo) return a.key.lo < b.key.lo;
    return a.key.hi < b.key.hi;
}

/// Zero-heap build disk cache catalog backed by a caller-provided slice.
pub struct BuildDiskCacheIndex {
    entries: []DiskCacheEntry,
    compiler_abi: u32 = 1,
    active_count: usize = 0,
    total_artifact_bytes: u64 = 0,
    lookup_hits: u64 = 0,
    lookup_misses: u64 = 0,
    abi_mismatches: u64 = 0,
    mtime_mismatches: u64 = 0,
    size_mismatches: u64 = 0,
    ttl_expirations: u64 = 0,
    upserts: u64 = 0,
    invalidations: u64 = 0,
    evictions: u64 = 0,

    /// Initializes a clean catalog over caller-provided `entries` storage.
    static fn init(entries: []DiskCacheEntry, compiler_abi: u32): BuildDiskCacheIndex {
        let i: usize = 0;
        while (i < entries.len) {
            entries[i] = DiskCacheEntry.empty();
            i += 1;
        }
        return BuildDiskCacheIndex{
            entries: entries,
            compiler_abi: compiler_abi,
        };
    }

    /// Clears all entries and resets runtime telemetry counters.
    fn reset(*_) {
        let i: usize = 0;
        while (i < _.entries.len) {
            _.entries[i] = DiskCacheEntry.empty();
            i += 1;
        }
        _.active_count = 0;
        _.total_artifact_bytes = 0;
        _.lookup_hits = 0;
        _.lookup_misses = 0;
        _.abi_mismatches = 0;
        _.mtime_mismatches = 0;
        _.size_mismatches = 0;
        _.ttl_expirations = 0;
        _.upserts = 0;
        _.invalidations = 0;
        _.evictions = 0;
    }

    /// Returns the number of active cached entries.
    fn count(): usize {
        return _.active_count;
    }

    /// Returns the maximum number of entries that fit in the backing slice.
    fn capacity(): usize {
        return _.entries.len;
    }

    /// Returns the total byte footprint of all active cached artifacts.
    fn totalArtifactBytes(): u64 {
        return _.total_artifact_bytes;
    }

    /// Locates the slot index of `key`, or `null` if absent.
    fn findSlot(key: BuildCacheKey): usize? {
        if (key.isZero()) return null;
        let i: usize = 0;
        while (i < _.entries.len) {
            if (_.entries[i].occupied && _.entries[i].key.eql(key)) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// Performs a validated cache lookup, verifying compiler ABI, TTL, source mtime,
    /// and source size. On hit, refreshes `last_access_sec` and increments `hit_count`.
    fn lookup(*_, query: DiskCacheLookupQuery): DiskCacheLookupResult {
        const slot_opt = _.findSlot(query.key);
        if (slot_opt == null) {
            _.lookup_misses += 1;
            return DiskCacheLookupResult{ status: DiskCacheMissReason.NotFound };
        }
        const idx = slot_opt ?? 0;
        const cur = _.entries[idx];

        const expected_abi = if (query.expected_compiler_abi != 0) query.expected_compiler_abi else _.compiler_abi;
        if (cur.compiler_abi != expected_abi) {
            _.lookup_misses += 1;
            _.abi_mismatches += 1;
            return DiskCacheLookupResult{
                status: DiskCacheMissReason.CompilerAbiMismatch,
                slot_index: idx,
                entry: cur,
            };
        }

        if (dcIsExpired(cur, query.now_sec, query.ttl_secs)) {
            _.lookup_misses += 1;
            _.ttl_expirations += 1;
            return DiskCacheLookupResult{
                status: DiskCacheMissReason.ExpiredTtl,
                slot_index: idx,
                entry: cur,
            };
        }

        if (query.check_mtime && cur.mtime_ns != query.expected_mtime_ns) {
            _.lookup_misses += 1;
            _.mtime_mismatches += 1;
            return DiskCacheLookupResult{
                status: DiskCacheMissReason.MtimeMismatch,
                slot_index: idx,
                entry: cur,
            };
        }

        if (query.check_size && cur.source_size != query.expected_source_size) {
            _.lookup_misses += 1;
            _.size_mismatches += 1;
            return DiskCacheLookupResult{
                status: DiskCacheMissReason.SizeMismatch,
                slot_index: idx,
                entry: cur,
            };
        }

        if (query.now_sec > _.entries[idx].last_access_sec) {
            _.entries[idx].last_access_sec = query.now_sec;
        }
        if (_.entries[idx].hit_count < 0xFFFFFFFF) {
            _.entries[idx].hit_count += 1;
        }
        _.lookup_hits += 1;

        return DiskCacheLookupResult{
            status: DiskCacheMissReason.Hit,
            slot_index: idx,
            entry: _.entries[idx],
        };
    }

    /// Inserts a new entry or updates an existing entry with the same `key`.
    /// Returns the assigned slot index.
    fn upsert(*_, entry: DiskCacheEntry): DiskCacheError!usize {
        if (entry.key.isZero()) return err.InvalidKeyInput;

        const abi = if (entry.compiler_abi == 0) _.compiler_abi else entry.compiler_abi;
        const last_acc = if (entry.last_access_sec < entry.created_at_sec) entry.created_at_sec else entry.last_access_sec;
        const rec = DiskCacheEntry{
            key: entry.key,
            domain: entry.domain,
            source_hash: entry.source_hash,
            dep_tree_hash: entry.dep_tree_hash,
            path_hash: entry.path_hash,
            mtime_ns: entry.mtime_ns,
            source_size: entry.source_size,
            artifact_size: entry.artifact_size,
            created_at_sec: entry.created_at_sec,
            last_access_sec: last_acc,
            compiler_abi: abi,
            hit_count: entry.hit_count,
            artifact_crc32: entry.artifact_crc32,
            flags: entry.flags,
            pinned: entry.pinned,
            occupied: true,
        };

        if (existing_idx := _.findSlot(rec.key)) {
            const old_bytes = _.entries[existing_idx].artifact_size;
            if (_.total_artifact_bytes >= old_bytes) {
                _.total_artifact_bytes -= old_bytes;
            } else {
                _.total_artifact_bytes = 0;
            }
            _.entries[existing_idx] = rec;
            _.total_artifact_bytes += rec.artifact_size;
            _.upserts += 1;
            return existing_idx;
        }

        let i: usize = 0;
        while (i < _.entries.len) {
            if (!_.entries[i].occupied) {
                _.entries[i] = rec;
                _.active_count += 1;
                _.total_artifact_bytes += rec.artifact_size;
                _.upserts += 1;
                return i;
            }
            i += 1;
        }
        return err.CapacityExceeded;
    }

    fn clearSlotAt(*_, idx: usize) {
        const old_bytes = _.entries[idx].artifact_size;
        _.entries[idx] = DiskCacheEntry.empty();
        if (_.active_count > 0) _.active_count -= 1;
        if (_.total_artifact_bytes >= old_bytes) {
            _.total_artifact_bytes -= old_bytes;
        } else {
            _.total_artifact_bytes = 0;
        }
    }

    /// Invalidates and removes a single entry by its 128-bit `key`.
    fn invalidate(*_, key: BuildCacheKey): bool {
        if (idx := _.findSlot(key)) {
            _.clearSlotAt(idx);
            _.invalidations += 1;
            return true;
        }
        return false;
    }

    /// Invalidates all entries matching `source_hash` (e.g. across multiple build targets).
    fn invalidateBySourceHash(*_, source_hash: u64): usize {
        let removed: usize = 0;
        let i: usize = 0;
        while (i < _.entries.len) {
            if (_.entries[i].occupied && _.entries[i].source_hash == source_hash) {
                _.clearSlotAt(i);
                _.invalidations += 1;
                removed += 1;
            }
            i += 1;
        }
        return removed;
    }

    /// Invalidates all entries belonging to a specific `DiskCacheDomain`.
    fn invalidateByDomain(*_, domain: DiskCacheDomain): usize {
        let removed: usize = 0;
        let i: usize = 0;
        while (i < _.entries.len) {
            if (_.entries[i].occupied && _.entries[i].domain == domain) {
                _.clearSlotAt(i);
                _.invalidations += 1;
                removed += 1;
            }
            i += 1;
        }
        return removed;
    }

    /// Invalidates all entries whose `compiler_abi` differs from `expected_abi`.
    fn invalidateMismatchedAbi(*_, expected_abi: u32): usize {
        let removed: usize = 0;
        let i: usize = 0;
        while (i < _.entries.len) {
            if (_.entries[i].occupied && _.entries[i].compiler_abi != expected_abi) {
                _.clearSlotAt(i);
                _.invalidations += 1;
                removed += 1;
            }
            i += 1;
        }
        return removed;
    }

    /// Generates a deterministic LRU + TTL + quota eviction plan without mutating the index.
    /// `scratch_indices` must have length >= `_.active_count` for in-place sorting.
    fn planEviction(
        policy: DiskCacheEvictionPolicy,
        out_items: []DiskCacheEvictionItem,
        scratch_indices: []usize,
    ): DiskCacheError!DiskCacheEvictionPlan {
        if (scratch_indices.len < _.active_count) {
            return err.BufferTooSmall;
        }

        let plan = DiskCacheEvictionPlan{
            remaining_entries: _.active_count,
            remaining_bytes: _.total_artifact_bytes,
        };

        let cand_count: usize = 0;
        let i: usize = 0;
        while (i < _.entries.len) {
            if (_.entries[i].occupied) {
                const e = _.entries[i];
                const protected = policy.respect_pinned && e.pinned;
                if (!protected && dcIsExpired(e, policy.now_sec, policy.ttl_secs)) {
                    if (plan.evicted_count >= out_items.len) return err.BufferTooSmall;
                    out_items[plan.evicted_count] = DiskCacheEvictionItem{
                        key: e.key,
                        slot_index: i,
                        artifact_size: e.artifact_size,
                        last_access_sec: e.last_access_sec,
                        reason: DiskCacheEvictionReason.ExpiredTtl,
                    };
                    plan.evicted_count += 1;
                    plan.expired_count += 1;
                    plan.reclaimed_bytes += e.artifact_size;
                    if (plan.remaining_entries > 0) plan.remaining_entries -= 1;
                    if (plan.remaining_bytes >= e.artifact_size) {
                        plan.remaining_bytes -= e.artifact_size;
                    } else {
                        plan.remaining_bytes = 0;
                    }
                } else if (!protected) {
                    scratch_indices[cand_count] = i;
                    cand_count += 1;
                }
            }
            i += 1;
        }

        // Check if quota thresholds are already satisfied after TTL expiration
        const need_bytes = policy.max_bytes > 0 && plan.remaining_bytes > policy.max_bytes;
        const need_entries = policy.max_entries > 0 && plan.remaining_entries > policy.max_entries;
        if (!need_bytes && !need_entries) {
            return plan;
        }

        // Deterministic insertion sort over active candidate indices by LRU priority
        let s: usize = 1;
        while (s < cand_count) {
            const key_slot = scratch_indices[s];
            const key_entry = _.entries[key_slot];
            let j = s;
            while (j > 0 && dcEvictBefore(key_entry, _.entries[scratch_indices[j - 1]])) {
                scratch_indices[j] = scratch_indices[j - 1];
                j -= 1;
            }
            scratch_indices[j] = key_slot;
            s += 1;
        }

        // Select victims until both max_bytes and max_entries are satisfied
        let k: usize = 0;
        while (k < cand_count) {
            const over_bytes = policy.max_bytes > 0 && plan.remaining_bytes > policy.max_bytes;
            const over_entries = policy.max_entries > 0 && plan.remaining_entries > policy.max_entries;
            if (!over_bytes && !over_entries) break;

            if (plan.evicted_count >= out_items.len) return err.BufferTooSmall;
            const slot = scratch_indices[k];
            const e = _.entries[slot];
            const reason = if (over_bytes) DiskCacheEvictionReason.OverByteQuota else DiskCacheEvictionReason.OverEntryQuota;

            out_items[plan.evicted_count] = DiskCacheEvictionItem{
                key: e.key,
                slot_index: slot,
                artifact_size: e.artifact_size,
                last_access_sec: e.last_access_sec,
                reason: reason,
            };
            plan.evicted_count += 1;
            plan.quota_evicted_count += 1;
            plan.reclaimed_bytes += e.artifact_size;
            if (plan.remaining_entries > 0) plan.remaining_entries -= 1;
            if (plan.remaining_bytes >= e.artifact_size) {
                plan.remaining_bytes -= e.artifact_size;
            } else {
                plan.remaining_bytes = 0;
            }
            k += 1;
        }

        return plan;
    }

    /// Applies a previously generated eviction plan to the index, returning removed count.
    fn applyEvictionPlan(*_, items: []const DiskCacheEvictionItem): usize {
        let removed: usize = 0;
        let i: usize = 0;
        while (i < items.len) {
            const item = items[i];
            if (item.slot_index < _.entries.len &&
                _.entries[item.slot_index].occupied &&
                _.entries[item.slot_index].key.eql(item.key))
            {
                const old_bytes = _.entries[item.slot_index].artifact_size;
                _.entries[item.slot_index] = DiskCacheEntry.empty();
                if (_.active_count > 0) _.active_count -= 1;
                if (_.total_artifact_bytes >= old_bytes) {
                    _.total_artifact_bytes -= old_bytes;
                } else {
                    _.total_artifact_bytes = 0;
                }
                _.evictions += 1;
                removed += 1;
            } else if (_.invalidate(item.key)) {
                _.evictions += 1;
                removed += 1;
            }
            i += 1;
        }
        return removed;
    }

    /// Analyzes entry spread across all 256 top-level directory shards using
    /// `cap:simd` (`Simd.reduceAddI32`, `Simd.reduceMinI32`, `Simd.reduceMaxI32`).
    fn analyzeShardDistribution(): DiskCacheShardDistribution {
        let counts: [256]i32 = [0; 256];
        let i: usize = 0;
        while (i < _.entries.len) {
            if (_.entries[i].occupied) {
                const shard_idx = @as(usize, _.entries[i].key.shardByte());
                counts[shard_idx] += 1;
            }
            i += 1;
        }

        const total = Simd.reduceAddI32(counts[0..]);
        const min_c = Simd.reduceMinI32(counts[0..]);
        const max_c = Simd.reduceMaxI32(counts[0..]);

        let non_empty: u32 = 0;
        let b: usize = 0;
        while (b < 256) {
            if (counts[b] > 0) non_empty += 1;
            b += 1;
        }

        return DiskCacheShardDistribution{
            non_empty_shards: non_empty,
            min_per_shard: min_c,
            max_per_shard: max_c,
            total_entries: total,
        };
    }

    /// Returns a snapshot of cumulative index statistics.
    fn stats(): DiskCacheIndexStats {
        return DiskCacheIndexStats{
            active_entries: _.active_count,
            capacity: _.entries.len,
            total_artifact_bytes: _.total_artifact_bytes,
            lookup_hits: _.lookup_hits,
            lookup_misses: _.lookup_misses,
            abi_mismatches: _.abi_mismatches,
            mtime_mismatches: _.mtime_mismatches,
            size_mismatches: _.size_mismatches,
            ttl_expirations: _.ttl_expirations,
            upserts: _.upserts,
            invalidations: _.invalidations,
            evictions: _.evictions,
        };
    }
}

// ============================================================================
// Section 4: CRC32-Verified Binary Catalog Snapshot Codec
// ============================================================================

/// Summary returned upon successful decoding of a binary catalog snapshot.
pub struct DiskCacheDecodeSummary {
    version: u16 = 0,
    flags: u16 = 0,
    compiler_abi: u32 = 0,
    entry_count: usize = 0,
    total_payload_bytes: u64 = 0,
    crc32: u32 = 0,
}

/// Computes the exact serialized byte size for a catalog containing `entry_count` entries.
pub fn encodedDiskCacheIndexSize(entry_count: usize): usize {
    return DISK_CACHE_HEADER_BYTES + entry_count * DISK_CACHE_RECORD_BYTES;
}

/// Serializes all active entries of `index` into a deterministic little-endian
/// binary snapshot protected by an IEEE 802.3 CRC32 checksum.
pub fn encodeDiskCacheIndex(index: *const BuildDiskCacheIndex, out: []u8): DiskCacheError!usize {
    const active = index.active_count;
    const needed = encodedDiskCacheIndexSize(active);
    if (out.len < needed) return err.BufferTooSmall;
    if (active > 0xFFFFFFFF) return err.CapacityExceeded;

    // Header [0..32]
    out[0] = DISK_CACHE_INDEX_MAGIC[0];
    out[1] = DISK_CACHE_INDEX_MAGIC[1];
    out[2] = DISK_CACHE_INDEX_MAGIC[2];
    out[3] = DISK_CACHE_INDEX_MAGIC[3];
    dcWriteU16Le(out, 4, DISK_CACHE_INDEX_VERSION);
    dcWriteU16Le(out, 6, 0); // reserved header flags
    dcWriteU32Le(out, 8, index.compiler_abi);
    dcWriteU32Le(out, 12, @as(u32, @intCast(active)));
    dcWriteU64Le(out, 16, index.total_artifact_bytes);
    dcWriteU32Le(out, 24, 0); // reserved
    dcWriteU32Le(out, 28, 0); // crc32 placeholder

    let off: usize = DISK_CACHE_HEADER_BYTES;
    let written: usize = 0;
    let i: usize = 0;
    while (i < index.entries.len) {
        if (index.entries[i].occupied) {
            if (written >= active) return err.CorruptRecord;
            const e = index.entries[i];
            dcWriteU64Le(out, off + 0, e.key.lo);
            dcWriteU64Le(out, off + 8, e.key.hi);
            dcWriteU64Le(out, off + 16, e.source_hash);
            dcWriteU64Le(out, off + 24, e.dep_tree_hash);
            dcWriteU64Le(out, off + 32, @as(u64, @bitCast(e.mtime_ns)));
            dcWriteU64Le(out, off + 40, e.source_size);
            dcWriteU64Le(out, off + 48, e.artifact_size);
            dcWriteU64Le(out, off + 56, e.created_at_sec);
            dcWriteU64Le(out, off + 64, e.last_access_sec);
            dcWriteU64Le(out, off + 72, e.path_hash);
            dcWriteU32Le(out, off + 80, e.compiler_abi);
            dcWriteU32Le(out, off + 84, e.hit_count);
            dcWriteU32Le(out, off + 88, e.artifact_crc32);
            out[off + 92] = diskCacheDomainToByte(e.domain);
            out[off + 93] = e.flags;
            out[off + 94] = if (e.pinned) 1 else 0;
            out[off + 95] = 0;
            off += DISK_CACHE_RECORD_BYTES;
            written += 1;
        }
        i += 1;
    }

    if (written != active) return err.CorruptRecord;

    // Compute CRC32 over header[0..28] + payload[32..needed]
    let crc = updateDiskCacheCrc32(0, out[0..28]);
    if (needed > DISK_CACHE_HEADER_BYTES) {
        crc = updateDiskCacheCrc32(crc, out[DISK_CACHE_HEADER_BYTES..needed]);
    }
    dcWriteU32Le(out, 28, crc);
    return needed;
}

/// Decodes and validates a binary catalog snapshot into `target`.
/// Hardened against truncated buffers, invalid magic/version, CRC mismatches,
/// overflowed record counts, invalid enum tags, duplicate/zero keys, and
/// mismatched payload byte totals. Resets `target` cleanly on any failure.
pub fn decodeDiskCacheIndex(data: []const u8, target: *BuildDiskCacheIndex): DiskCacheError!DiskCacheDecodeSummary {
    if (data.len < DISK_CACHE_HEADER_BYTES) {
        return err.TruncatedData;
    }

    if (data[0] != DISK_CACHE_INDEX_MAGIC[0] ||
        data[1] != DISK_CACHE_INDEX_MAGIC[1] ||
        data[2] != DISK_CACHE_INDEX_MAGIC[2] ||
        data[3] != DISK_CACHE_INDEX_MAGIC[3])
    {
        return err.InvalidMagic;
    }

    const version = dcReadU16Le(data, 4);
    if (version != DISK_CACHE_INDEX_VERSION) {
        return err.UnsupportedVersion;
    }

    const flags = dcReadU16Le(data, 6);
    const compiler_abi = dcReadU32Le(data, 8);
    const entry_count_u32 = dcReadU32Le(data, 12);
    const header_total_bytes = dcReadU64LeUnchecked(data, 16);
    const stored_crc = dcReadU32Le(data, 28);

    const entry_count = @as(usize, entry_count_u32);
    const payload_len = data.len - DISK_CACHE_HEADER_BYTES;
    const max_fit_records = payload_len / DISK_CACHE_RECORD_BYTES;
    if (entry_count > max_fit_records) {
        return err.TruncatedData;
    }
    const expected_len = DISK_CACHE_HEADER_BYTES + entry_count * DISK_CACHE_RECORD_BYTES;
    if (data.len != expected_len) {
        return err.TruncatedData;
    }
    if (entry_count > target.entries.len) {
        return err.CapacityExceeded;
    }

    // Verify CRC32 over header[0..28] + payload[32..expected_len]
    let computed_crc = updateDiskCacheCrc32(0, data[0..28]);
    if (expected_len > DISK_CACHE_HEADER_BYTES) {
        computed_crc = updateDiskCacheCrc32(computed_crc, data[DISK_CACHE_HEADER_BYTES..expected_len]);
    }
    if (computed_crc != stored_crc) {
        return err.ChecksumMismatch;
    }

    target.reset();
    target.compiler_abi = compiler_abi;

    let off: usize = DISK_CACHE_HEADER_BYTES;
    let sum_artifact_bytes: u64 = 0;
    let i: usize = 0;
    while (i < entry_count) {
        const key_lo = dcReadU64LeUnchecked(data, off + 0);
        const key_hi = dcReadU64LeUnchecked(data, off + 8);
        const key = BuildCacheKey{ hi: key_hi, lo: key_lo };
        if (key.isZero()) {
            target.reset();
            return err.CorruptRecord;
        }

        const domain_opt = diskCacheDomainFromByte(data[off + 92]);
        if (domain_opt == null) {
            target.reset();
            return err.CorruptRecord;
        }
        const pinned_byte = data[off + 94];
        if (pinned_byte > 1) {
            target.reset();
            return err.CorruptRecord;
        }

        const artifact_size = dcReadU64LeUnchecked(data, off + 48);
        // Overflow guard on cumulative artifact bytes
        if (artifact_size > 0xFFFFFFFFFFFFFFFF - sum_artifact_bytes) {
            target.reset();
            return err.CorruptRecord;
        }
        sum_artifact_bytes += artifact_size;

        // Reject duplicate keys within the same snapshot
        if (target.findSlot(key) != null) {
            target.reset();
            return err.CorruptRecord;
        }

        const mtime_bits = dcReadU64LeUnchecked(data, off + 32);
        target.entries[i] = DiskCacheEntry{
            key: key,
            domain: domain_opt ?? DiskCacheDomain.JavaScript,
            source_hash: dcReadU64LeUnchecked(data, off + 16),
            dep_tree_hash: dcReadU64LeUnchecked(data, off + 24),
            mtime_ns: @as(i64, @bitCast(mtime_bits)),
            source_size: dcReadU64LeUnchecked(data, off + 40),
            artifact_size: artifact_size,
            created_at_sec: dcReadU64LeUnchecked(data, off + 56),
            last_access_sec: dcReadU64LeUnchecked(data, off + 64),
            path_hash: dcReadU64LeUnchecked(data, off + 72),
            compiler_abi: dcReadU32Le(data, off + 80),
            hit_count: dcReadU32Le(data, off + 84),
            artifact_crc32: dcReadU32Le(data, off + 88),
            flags: data[off + 93],
            pinned: pinned_byte == 1,
            occupied: true,
        };
        target.active_count += 1;
        off += DISK_CACHE_RECORD_BYTES;
        i += 1;
    }

    if (sum_artifact_bytes != header_total_bytes) {
        target.reset();
        return err.CorruptRecord;
    }

    target.total_artifact_bytes = sum_artifact_bytes;

    return DiskCacheDecodeSummary{
        version: version,
        flags: flags,
        compiler_abi: compiler_abi,
        entry_count: entry_count,
        total_payload_bytes: sum_artifact_bytes,
        crc32: computed_crc,
    };
}

// ============================================================================
// Section 5: Human-Readable Diagnostic Summary Formatter
// ============================================================================

/// Formats a concise multi-line diagnostic report for `cap build` telemetry.
pub fn formatDiskCacheReport(index: *const BuildDiskCacheIndex, out: []u8): DiskCacheError!usize {
    const st = index.stats();
    const dist = index.analyzeShardDistribution();
    const total_lookups = st.lookup_hits + st.lookup_misses;
    const hit_pct: u64 = if (total_lookups == 0) 0 else (st.lookup_hits * 100) / total_lookups;

    let pos: usize = 0;
    dcAppendBytes(out, &pos, "BuildDiskCache(abi=")?;
    dcAppendU64(out, &pos, @as(u64, index.compiler_abi))?;
    dcAppendBytes(out, &pos, ", entries=")?;
    dcAppendU64(out, &pos, @as(u64, st.active_entries))?;
    dcAppendByte(out, &pos, '/')?;
    dcAppendU64(out, &pos, @as(u64, st.capacity))?;
    dcAppendBytes(out, &pos, ", bytes=")?;
    dcAppendU64(out, &pos, st.total_artifact_bytes)?;
    dcAppendBytes(out, &pos, ", hit_rate=")?;
    dcAppendU64(out, &pos, hit_pct)?;
    dcAppendBytes(out, &pos, "%, hits=")?;
    dcAppendU64(out, &pos, st.lookup_hits)?;
    dcAppendBytes(out, &pos, ", misses=")?;
    dcAppendU64(out, &pos, st.lookup_misses)?;
    dcAppendBytes(out, &pos, ", shards=")?;
    dcAppendU64(out, &pos, @as(u64, dist.non_empty_shards))?;
    dcAppendBytes(out, &pos, ", max_shard=")?;
    dcAppendU64(out, &pos, @as(u64, @intCast(dist.max_per_shard)))?;
    dcAppendByte(out, &pos, ')')?;
    return pos;
}
