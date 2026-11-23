// Source: Absorbed from /data/dev/bun/src/install/lockfile/Buffers.cp | Synced: 2026-11-22T05:42:04Z
// ============================================================================
// lib/pkg/lockArenaBuf.cp — Zero-Fragmentation Contiguous Lockfile Arena Buffers
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven contiguous arena buffer cluster for
// `cap.lock` high-speed serialization and zero-copy read-only image views:
//   1. Five contiguous typed segments (`LockArenaBuffers`):
//        - `string_bytes`: deduplicated UTF-8 string pool backed by an
//          open-addressing FNV-1a intern table (zero duplicate package names,
//          URLs, or license identifiers);
//        - `extern_strings`: compact 8-byte `(offset, len)` string handles;
//        - `dependencies`: flat dependency declaration records;
//        - `resolutions`: parallel resolved package ID array (`len == deps.len`);
//        - `trees`: flat hoisted dependency tree nodes;
//   2. Alignment verification, fragmentation compaction & dedup telemetry
//      (`compactLockArenaBuffers`);
//   3. Zero-copy CRC32-verified binary image codec (`encodeLockArenaImage`,
//      `LockArenaImageView.open`, `decodeLockArenaImageInto`) hardened against
//      truncated buffers, misaligned offsets, overlapping sections, 0xDEADBEEF
//      unwritten sentinels, and out-of-bounds string handles;
//   4. `cap:simd`-accelerated Windows path separator guard and string matching.

import { Simd } from "cap:simd";

/// Four-byte magic prefix for binary lockfile arena images ("CPLA").
pub const LOCK_ARENA_MAGIC: [4]u8 = ['C', 'P', 'L', 'A'];

/// Binary format version for lockfile arena images.
pub const LOCK_ARENA_VERSION: u16 = 1;

/// Number of contiguous sections in a lockfile arena image.
pub const LOCK_ARENA_SECTION_COUNT: usize = 5;

/// Fixed byte length of the binary image header (24B base + 5 * 16B section table).
pub const LOCK_ARENA_HEADER_BYTES: usize = 104;

/// Mandatory section byte alignment in serialized images.
pub const LOCK_ARENA_ALIGNMENT: usize = 8;

/// Wire size of a serialized `LockExternString` handle.
pub const LOCK_ARENA_EXTERN_STR_BYTES: usize = 8;

/// Wire size of a serialized `LockArenaDep` record.
pub const LOCK_ARENA_DEP_BYTES: usize = 24;

/// Wire size of a serialized `LockArenaTreeNode` record.
pub const LOCK_ARENA_TREE_BYTES: usize = 16;

/// Sentinel indicating an unresolved or absent package / dependency / tree ID.
pub const LOCK_ARENA_INVALID_ID: u32 = 0xFFFFFFFF;

/// Sentinel indicating an empty slot in the open-addressing intern table.
pub const LOCK_ARENA_EMPTY_BUCKET: u32 = 0xFFFFFFFF;

/// Unwritten offset sentinel inherited from Bun's lockfile buffer codec.
pub const LOCK_ARENA_DEAD_SENTINEL: u64 = 0xDEADBEEF;

/// Error set for lockfile arena buffering, compaction, and binary image codec.
pub err LockArenaError {
    BufferTooSmall,
    CapacityExceeded,
    InvalidMagic,
    UnsupportedVersion,
    ChecksumMismatch,
    TruncatedData,
    MisalignedSection,
    InvalidSectionBounds,
    CorruptRecord,
    InvalidStringSlice,
    WindowsPathSeparator,
    MissingResolution,
}

// ============================================================================
// Internal Zero-Alloc Endian, Hash, CRC32, and Formatting Helpers
// ============================================================================

/// Nibble lookup table for IEEE 802.3 CRC32 (polynomial 0xEDB88320).
const ARENA_CRC32_TABLE: [16]u32 = [
    0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
    0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
    0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
    0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C,
];

fn laWriteU16Le(out: []u8, off: usize, v: u16) {
    out[off] = @as(u8, @truncate(v));
    out[off + 1] = @as(u8, @truncate(v >> 8));
}

fn laWriteU32Le(out: []u8, off: usize, v: u32) {
    out[off] = @as(u8, @truncate(v));
    out[off + 1] = @as(u8, @truncate(v >> 8));
    out[off + 2] = @as(u8, @truncate(v >> 16));
    out[off + 3] = @as(u8, @truncate(v >> 24));
}

fn laWriteU64Le(out: []u8, off: usize, v: u64) {
    out[off] = @as(u8, @truncate(v));
    out[off + 1] = @as(u8, @truncate(v >> 8));
    out[off + 2] = @as(u8, @truncate(v >> 16));
    out[off + 3] = @as(u8, @truncate(v >> 24));
    out[off + 4] = @as(u8, @truncate(v >> 32));
    out[off + 5] = @as(u8, @truncate(v >> 40));
    out[off + 6] = @as(u8, @truncate(v >> 48));
    out[off + 7] = @as(u8, @truncate(v >> 56));
}

fn laReadU16Le(data: []const u8, off: usize): u16 {
    return @as(u16, data[off]) | (@as(u16, data[off + 1]) << 8);
}

fn laReadU32Le(data: []const u8, off: usize): u32 {
    return @as(u32, data[off]) |
        (@as(u32, data[off + 1]) << 8) |
        (@as(u32, data[off + 2]) << 16) |
        (@as(u32, data[off + 3]) << 24);
}

fn laReadU64Le(data: []const u8, off: usize): u64 {
    return @as(u64, data[off]) |
        (@as(u64, data[off + 1]) << 8) |
        (@as(u64, data[off + 2]) << 16) |
        (@as(u64, data[off + 3]) << 24) |
        (@as(u64, data[off + 4]) << 32) |
        (@as(u64, data[off + 5]) << 40) |
        (@as(u64, data[off + 6]) << 48) |
        (@as(u64, data[off + 7]) << 56);
}

fn laBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn laCopyBytes(dst: []u8, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[i] = src[i];
        i += 1;
    }
}

fn laAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn laAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn laAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        laAppendByte(out, pos, '0')?;
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
        laAppendByte(out, pos, tmp[n])?;
    }
}

// ============================================================================
// Section 1: Public Hash, CRC32, Alignment & SIMD Scanning Primitives
// ============================================================================

/// Computes a 64-bit FNV-1a hash over `s` for open-addressing string interning.
pub fn hashLockArenaString64(s: []const u8): u64 {
    let h: u64 = 0xCBF29CE484222325;
    let i: usize = 0;
    while (i < s.len) {
        h ^= @as(u64, s[i]);
        h = h *% 0x100000001B3;
        i += 1;
    }
    return h;
}

/// Incrementally updates an IEEE 802.3 CRC32 checksum.
pub fn updateLockArenaCrc32(initial_crc: u32, data: []const u8): u32 {
    let crc: u32 = initial_crc ^ 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) {
        const b: u32 = @as(u32, data[i]);
        crc = (crc >> 4) ^ ARENA_CRC32_TABLE[@as(usize, (crc ^ b) & 0xF)];
        crc = (crc >> 4) ^ ARENA_CRC32_TABLE[@as(usize, (crc ^ (b >> 4)) & 0xF)];
        i += 1;
    }
    return crc ^ 0xFFFFFFFF;
}

/// Computes the IEEE 802.3 CRC32 checksum of `data`.
pub fn computeLockArenaCrc32(data: []const u8): u32 {
    return updateLockArenaCrc32(0, data);
}

/// Rounds `offset` upward to the nearest multiple of `alignment` (must be power of two >= 1).
pub fn alignLockArenaOffset(offset: usize, alignment: usize): usize {
    if (alignment <= 1) return offset;
    const mask = alignment - 1;
    return (offset + mask) & ~mask;
}

/// Checks whether `offset` is aligned to `alignment` (power of two >= 1).
pub fn isLockArenaAligned(offset: usize, alignment: usize): bool {
    if (alignment <= 1) return true;
    return (offset & (alignment - 1)) == 0;
}

/// Uses `cap:simd` (`Simd.indexOfByte`) to detect forbidden Windows backslash separators.
pub fn hasWindowsPathSeparator(path: []const u8): bool {
    return Simd.indexOfByte(path, '\\') != null;
}

/// Uses `cap:simd` (`Simd.indexOfByte`) to search for `needle` inside a contiguous string pool.
pub fn findInLockStringPool(pool: []const u8, needle: []const u8): usize? {
    if (needle.len == 0) return 0;
    if (needle.len > pool.len) return null;
    const first = needle[0];
    let pos: usize = 0;
    while (pos + needle.len <= pool.len) {
        if (rel := Simd.indexOfByte(pool[pos..], first)) {
            const idx = pos + rel;
            if (idx + needle.len > pool.len) return null;
            if (laBytesEql(pool[idx..idx + needle.len], needle)) {
                return idx;
            }
            pos = idx + 1;
        } else {
            return null;
        }
    }
    return null;
}

// ============================================================================
// Section 2: Compact Segment Element Types
// ============================================================================

/// Compact 8-byte `(offset, len)` handle addressing a slice inside `string_bytes`.
pub struct LockExternString {
    offset: u32 = 0,
    len: u32 = 0,

    static fn empty(): LockExternString {
        return LockExternString{ offset: 0, len: 0 };
    }

    fn isEmpty(): bool {
        return _.len == 0;
    }

    fn eql(other: LockExternString): bool {
        return _.offset == other.offset && _.len == other.len;
    }

    /// Resolves this handle against `string_bytes` with overflow-safe bounds checks.
    fn resolve(string_bytes: []const u8): LockArenaError![]const u8 {
        if (_.len == 0) return "";
        const off = @as(usize, _.offset);
        const l = @as(usize, _.len);
        if (off > string_bytes.len || l > string_bytes.len - off) {
            return err.InvalidStringSlice;
        }
        return string_bytes[off..off + l];
    }
}

/// Dependency source kind stored in `LockArenaDep`.
pub enum LockArenaDepKind {
    Npm,
    Workspace,
    Folder,
    Tarball,
    Git,
    Symlink,
    Alias,
}

/// Converts `LockArenaDepKind` to its stable wire byte tag.
pub fn lockArenaDepKindToByte(kind: LockArenaDepKind): u8 {
    return match (kind) {
        LockArenaDepKind.Npm => 0,
        LockArenaDepKind.Workspace => 1,
        LockArenaDepKind.Folder => 2,
        LockArenaDepKind.Tarball => 3,
        LockArenaDepKind.Git => 4,
        LockArenaDepKind.Symlink => 5,
        LockArenaDepKind.Alias => 6,
    };
}

/// Decodes a wire byte tag into `LockArenaDepKind`, or `null` if unknown.
pub fn lockArenaDepKindFromByte(tag: u8): LockArenaDepKind? {
    if (tag == 0) return LockArenaDepKind.Npm;
    if (tag == 1) return LockArenaDepKind.Workspace;
    if (tag == 2) return LockArenaDepKind.Folder;
    if (tag == 3) return LockArenaDepKind.Tarball;
    if (tag == 4) return LockArenaDepKind.Git;
    if (tag == 5) return LockArenaDepKind.Symlink;
    if (tag == 6) return LockArenaDepKind.Alias;
    return null;
}

/// Flat dependency declaration record (24 bytes on wire).
pub struct LockArenaDep {
    name_ref: LockExternString = LockExternString.empty(),
    spec_ref: LockExternString = LockExternString.empty(),
    behavior_flags: u16 = 0,
    kind: LockArenaDepKind = LockArenaDepKind.Npm,
    owner_pkg_id: u32 = 0,

    static fn empty(): LockArenaDep {
        return LockArenaDep{};
    }
}

/// Flat hoisted dependency tree node (16 bytes on wire).
pub struct LockArenaTreeNode {
    parent_tree_id: u32 = LOCK_ARENA_INVALID_ID,
    dep_id: u32 = LOCK_ARENA_INVALID_ID,
    deps_start: u32 = 0,
    deps_len: u32 = 0,

    static fn empty(): LockArenaTreeNode {
        return LockArenaTreeNode{};
    }
}

/// Section descriptor stored in the binary image header (16 bytes on wire).
pub struct LockArenaSectionDesc {
    offset: u64 = 0,
    byte_len: u32 = 0,
    elem_count: u32 = 0,

    static fn empty(): LockArenaSectionDesc {
        return LockArenaSectionDesc{};
    }
}

/// Runtime telemetry snapshot for `LockArenaBuffers`.
pub struct LockArenaStats {
    string_bytes_used: usize = 0,
    string_bytes_cap: usize = 0,
    extern_strings_count: usize = 0,
    dependencies_count: usize = 0,
    resolutions_count: usize = 0,
    trees_count: usize = 0,
    intern_requests: u64 = 0,
    intern_hits: u64 = 0,
    raw_input_string_bytes: u64 = 0,
    dedup_saved_bytes: u64 = 0,
}

// ============================================================================
// Section 3: Zero-Fragmentation Contiguous Arena Buffer Cluster
// ============================================================================

/// Five-segment contiguous lockfile arena backed exclusively by caller-provided slices.
pub struct LockArenaBuffers {
    string_bytes: []u8,
    string_bytes_len: usize = 0,
    intern_buckets: []u32,
    extern_strings: []LockExternString,
    extern_strings_len: usize = 0,
    dependencies: []LockArenaDep,
    dependencies_len: usize = 0,
    resolutions: []u32,
    resolutions_len: usize = 0,
    trees: []LockArenaTreeNode,
    trees_len: usize = 0,

    intern_requests: u64 = 0,
    intern_hits: u64 = 0,
    raw_input_string_bytes: u64 = 0,
    dedup_saved_bytes: u64 = 0,

    /// Initializes a clean five-segment arena over caller-provided slices.
    static fn init(
        string_bytes: []u8,
        intern_buckets: []u32,
        extern_strings: []LockExternString,
        dependencies: []LockArenaDep,
        resolutions: []u32,
        trees: []LockArenaTreeNode,
    ): LockArenaBuffers {
        let b: usize = 0;
        while (b < intern_buckets.len) {
            intern_buckets[b] = LOCK_ARENA_EMPTY_BUCKET;
            b += 1;
        }
        return LockArenaBuffers{
            string_bytes: string_bytes,
            intern_buckets: intern_buckets,
            extern_strings: extern_strings,
            dependencies: dependencies,
            resolutions: resolutions,
            trees: trees,
        };
    }

    /// Resets all five segments, clears the intern table, and zeroes telemetry counters.
    fn reset(*_) {
        let b: usize = 0;
        while (b < _.intern_buckets.len) {
            _.intern_buckets[b] = LOCK_ARENA_EMPTY_BUCKET;
            b += 1;
        }
        _.string_bytes_len = 0;
        _.extern_strings_len = 0;
        _.dependencies_len = 0;
        _.resolutions_len = 0;
        _.trees_len = 0;
        _.intern_requests = 0;
        _.intern_hits = 0;
        _.raw_input_string_bytes = 0;
        _.dedup_saved_bytes = 0;
    }

    /// Interns `s` into `string_bytes` via open-addressing FNV-1a deduplication,
    /// returning the 8-byte `LockExternString` handle.
    fn internString(*_, s: []const u8): LockArenaError!LockExternString {
        _.intern_requests += 1;
        _.raw_input_string_bytes += @as(u64, s.len);

        if (s.len == 0) {
            _.intern_hits += 1;
            return LockExternString.empty();
        }
        if (_.intern_buckets.len == 0) return err.CapacityExceeded;

        const hash = hashLockArenaString64(s);
        const cap = _.intern_buckets.len;
        let slot = @as(usize, @intCast(hash % @as(u64, cap)));
        let probes: usize = 0;

        while (probes < cap) {
            const entry_idx = _.intern_buckets[slot];
            if (entry_idx == LOCK_ARENA_EMPTY_BUCKET) {
                break;
            }
            const sref = _.extern_strings[@as(usize, entry_idx)];
            const existing = _.string_bytes[@as(usize, sref.offset)..@as(usize, sref.offset) + @as(usize, sref.len)];
            if (laBytesEql(existing, s)) {
                _.intern_hits += 1;
                _.dedup_saved_bytes += @as(u64, s.len);
                return sref;
            }
            slot += 1;
            if (slot == cap) slot = 0;
            probes += 1;
        }

        if (probes >= cap) return err.CapacityExceeded;
        if (_.extern_strings_len >= _.extern_strings.len) return err.CapacityExceeded;
        if (s.len > _.string_bytes.len - _.string_bytes_len) return err.BufferTooSmall;
        if (_.string_bytes_len > 0xFFFFFFFF || s.len > 0xFFFFFFFF) return err.CapacityExceeded;

        const off = _.string_bytes_len;
        laCopyBytes(_.string_bytes[off..off + s.len], s);
        _.string_bytes_len += s.len;

        const sref = LockExternString{
            offset: @as(u32, @intCast(off)),
            len: @as(u32, @intCast(s.len)),
        };
        const ext_id = _.extern_strings_len;
        _.extern_strings[ext_id] = sref;
        _.extern_strings_len += 1;
        _.intern_buckets[slot] = @as(u32, @intCast(ext_id));
        return sref;
    }

    /// Resolves a `LockExternString` handle against the active `string_bytes` pool.
    fn resolveString(sref: LockExternString): LockArenaError![]const u8 {
        return sref.resolve(_.string_bytes[0.._.string_bytes_len]);
    }

    /// Appends a dependency record and its parallel resolved package ID.
    /// Rejects Windows backslash separators for local path dependencies via `cap:simd`.
    fn addDependency(
        *_,
        name: []const u8,
        spec: []const u8,
        kind: LockArenaDepKind,
        behavior_flags: u16,
        owner_pkg_id: u32,
        resolved_pkg_id: u32,
    ): LockArenaError!u32 {
        if (kind == LockArenaDepKind.Folder ||
            kind == LockArenaDepKind.Workspace ||
            kind == LockArenaDepKind.Symlink)
        {
            if (hasWindowsPathSeparator(spec)) {
                return err.WindowsPathSeparator;
            }
        }

        if (_.dependencies_len >= _.dependencies.len || _.resolutions_len >= _.resolutions.len) {
            return err.CapacityExceeded;
        }

        const name_ref = _.internString(name)?;
        const spec_ref = _.internString(spec)?;

        const dep_idx = _.dependencies_len;
        _.dependencies[dep_idx] = LockArenaDep{
            name_ref: name_ref,
            spec_ref: spec_ref,
            behavior_flags: behavior_flags,
            kind: kind,
            owner_pkg_id: owner_pkg_id,
        };
        _.resolutions[_.resolutions_len] = resolved_pkg_id;
        _.dependencies_len += 1;
        _.resolutions_len += 1;
        return @as(u32, @intCast(dep_idx));
    }

    /// Updates the resolved package ID for an existing dependency slot.
    fn setResolution(*_, dep_id: u32, pkg_id: u32)! {
        const idx = @as(usize, dep_id);
        if (idx >= _.resolutions_len) return err.CorruptRecord;
        _.resolutions[idx] = pkg_id;
    }

    /// Appends a hoisted dependency tree node and returns its tree ID.
    fn addTreeNode(*_, parent_tree_id: u32, dep_id: u32, deps_start: u32, deps_len: u32): LockArenaError!u32 {
        if (_.trees_len >= _.trees.len) return err.CapacityExceeded;
        const tid = _.trees_len;
        _.trees[tid] = LockArenaTreeNode{
            parent_tree_id: parent_tree_id,
            dep_id: dep_id,
            deps_start: deps_start,
            deps_len: deps_len,
        };
        _.trees_len += 1;
        return @as(u32, @intCast(tid));
    }

    /// Maps a legacy `package_id` to the first unvisited matching `dependency_id`
    /// in `resolutions` (absorbed from Bun `Buffers.legacyPackageToDependencyID`).
    fn legacyPackageToDependencyId(package_id: u32, visited_flags: []u8): LockArenaError!u32 {
        if (package_id == 0) return 0;
        if (package_id == LOCK_ARENA_INVALID_ID) return LOCK_ARENA_INVALID_ID;
        let dep_id: usize = 0;
        while (dep_id < _.resolutions_len) {
            if (_.resolutions[dep_id] == package_id) {
                if (dep_id < visited_flags.len) {
                    if (visited_flags[dep_id] != 0) {
                        dep_id += 1;
                        continue;
                    }
                    visited_flags[dep_id] = 1;
                }
                return @as(u32, @intCast(dep_id));
            }
            dep_id += 1;
        }
        return err.MissingResolution;
    }

    /// Returns a snapshot of current buffer occupancy and string deduplication telemetry.
    fn stats(): LockArenaStats {
        return LockArenaStats{
            string_bytes_used: _.string_bytes_len,
            string_bytes_cap: _.string_bytes.len,
            extern_strings_count: _.extern_strings_len,
            dependencies_count: _.dependencies_len,
            resolutions_count: _.resolutions_len,
            trees_count: _.trees_len,
            intern_requests: _.intern_requests,
            intern_hits: _.intern_hits,
            raw_input_string_bytes: _.raw_input_string_bytes,
            dedup_saved_bytes: _.dedup_saved_bytes,
        };
    }
}

// ============================================================================
// Section 4: Fragmentation Compaction & Deduplication Statistics
// ============================================================================

/// Summary of space reclaimed during `compactLockArenaBuffers`.
pub struct LockArenaCompactStats {
    before_string_bytes: usize = 0,
    after_string_bytes: usize = 0,
    reclaimed_string_bytes: usize = 0,
    before_extern_strings: usize = 0,
    after_extern_strings: usize = 0,
    live_dependencies: usize = 0,
    live_trees: usize = 0,
    dedup_saved_bytes: u64 = 0,
}

/// Compacts `src` into a clean destination arena `dst`, discarding unreferenced
/// orphan strings in `src.string_bytes` while re-interning all live dependency
/// names and specifiers and preserving resolutions and hoist trees verbatim.
pub fn compactLockArenaBuffers(
    src: *const LockArenaBuffers,
    dst: *LockArenaBuffers,
): LockArenaError!LockArenaCompactStats {
    if (src.dependencies_len != src.resolutions_len) {
        return err.CorruptRecord;
    }

    dst.reset();

    let i: usize = 0;
    while (i < src.dependencies_len) {
        const dep = src.dependencies[i];
        const name_s = src.resolveString(dep.name_ref)?;
        const spec_s = src.resolveString(dep.spec_ref)?;
        const res_pkg = src.resolutions[i];
        * = dst.addDependency(name_s, spec_s, dep.kind, dep.behavior_flags, dep.owner_pkg_id, res_pkg)?;
        i += 1;
    }

    let t: usize = 0;
    while (t < src.trees_len) {
        const node = src.trees[t];
        * = dst.addTreeNode(node.parent_tree_id, node.dep_id, node.deps_start, node.deps_len)?;
        t += 1;
    }

    const reclaimed: usize = if (src.string_bytes_len >= dst.string_bytes_len)
        src.string_bytes_len - dst.string_bytes_len
    else
        0;

    return LockArenaCompactStats{
        before_string_bytes: src.string_bytes_len,
        after_string_bytes: dst.string_bytes_len,
        reclaimed_string_bytes: reclaimed,
        before_extern_strings: src.extern_strings_len,
        after_extern_strings: dst.extern_strings_len,
        live_dependencies: dst.dependencies_len,
        live_trees: dst.trees_len,
        dedup_saved_bytes: dst.dedup_saved_bytes,
    };
}

// ============================================================================
// Section 5: CRC32-Verified Zero-Copy Binary Image Codec & Read-Only View
// ============================================================================

/// Summary returned by `decodeLockArenaImageInto`.
pub struct LockArenaImageSummary {
    version: u16 = 0,
    total_image_bytes: usize = 0,
    string_bytes_len: usize = 0,
    extern_strings_count: usize = 0,
    dependencies_count: usize = 0,
    resolutions_count: usize = 0,
    trees_count: usize = 0,
    crc32: u32 = 0,
}

/// Computes the exact 8-byte-aligned serialized binary image size for `bufs`.
pub fn encodedLockArenaImageSize(bufs: *const LockArenaBuffers): usize {
    let pos: usize = LOCK_ARENA_HEADER_BYTES;
    // Section 0: string_bytes
    pos = alignLockArenaOffset(pos, LOCK_ARENA_ALIGNMENT);
    pos += bufs.string_bytes_len;
    // Section 1: extern_strings
    pos = alignLockArenaOffset(pos, LOCK_ARENA_ALIGNMENT);
    pos += bufs.extern_strings_len * LOCK_ARENA_EXTERN_STR_BYTES;
    // Section 2: dependencies
    pos = alignLockArenaOffset(pos, LOCK_ARENA_ALIGNMENT);
    pos += bufs.dependencies_len * LOCK_ARENA_DEP_BYTES;
    // Section 3: resolutions
    pos = alignLockArenaOffset(pos, LOCK_ARENA_ALIGNMENT);
    pos += bufs.resolutions_len * 4;
    // Section 4: trees
    pos = alignLockArenaOffset(pos, LOCK_ARENA_ALIGNMENT);
    pos += bufs.trees_len * LOCK_ARENA_TREE_BYTES;
    return alignLockArenaOffset(pos, LOCK_ARENA_ALIGNMENT);
}

fn laWriteSectionDesc(out: []u8, sec_idx: usize, desc: LockArenaSectionDesc) {
    const base = 24 + sec_idx * 16;
    laWriteU64Le(out, base + 0, desc.offset);
    laWriteU32Le(out, base + 8, desc.byte_len);
    laWriteU32Le(out, base + 12, desc.elem_count);
}

fn laReadSectionDesc(data: []const u8, sec_idx: usize): LockArenaSectionDesc {
    const base = 24 + sec_idx * 16;
    return LockArenaSectionDesc{
        offset: laReadU64Le(data, base + 0),
        byte_len: laReadU32Le(data, base + 8),
        elem_count: laReadU32Le(data, base + 12),
    };
}

/// Serializes all five contiguous arena segments into a deterministic, 8-byte-aligned,
/// zero-padded binary image protected by an IEEE 802.3 CRC32 checksum.
pub fn encodeLockArenaImage(bufs: *const LockArenaBuffers, out: []u8): LockArenaError!usize {
    if (bufs.dependencies_len != bufs.resolutions_len) return err.CorruptRecord;
    const total_len = encodedLockArenaImageSize(bufs);
    if (out.len < total_len) return err.BufferTooSmall;

    // Zero entire target slice to guarantee deterministic alignment padding
    let z: usize = 0;
    while (z < total_len) {
        out[z] = 0;
        z += 1;
    }

    // Header base [0..24]
    out[0] = LOCK_ARENA_MAGIC[0];
    out[1] = LOCK_ARENA_MAGIC[1];
    out[2] = LOCK_ARENA_MAGIC[2];
    out[3] = LOCK_ARENA_MAGIC[3];
    laWriteU16Le(out, 4, LOCK_ARENA_VERSION);
    laWriteU16Le(out, 6, 0);
    laWriteU32Le(out, 8, @as(u32, @intCast(LOCK_ARENA_ALIGNMENT)));
    laWriteU32Le(out, 12, 0); // crc32 placeholder
    laWriteU64Le(out, 16, @as(u64, total_len));

    let pos: usize = LOCK_ARENA_HEADER_BYTES;

    // Section 0: string_bytes
    pos = alignLockArenaOffset(pos, LOCK_ARENA_ALIGNMENT);
    const s0_off = pos;
    const s0_len = bufs.string_bytes_len;
    if (s0_len > 0) {
        laCopyBytes(out[pos..pos + s0_len], bufs.string_bytes[0..s0_len]);
        pos += s0_len;
    }
    laWriteSectionDesc(out, 0, LockArenaSectionDesc{
        offset: @as(u64, s0_off),
        byte_len: @as(u32, @intCast(s0_len)),
        elem_count: @as(u32, @intCast(s0_len)),
    });

    // Section 1: extern_strings
    pos = alignLockArenaOffset(pos, LOCK_ARENA_ALIGNMENT);
    const s1_off = pos;
    let idx1: usize = 0;
    while (idx1 < bufs.extern_strings_len) {
        const sref = bufs.extern_strings[idx1];
        laWriteU32Le(out, pos + 0, sref.offset);
        laWriteU32Le(out, pos + 4, sref.len);
        pos += LOCK_ARENA_EXTERN_STR_BYTES;
        idx1 += 1;
    }
    laWriteSectionDesc(out, 1, LockArenaSectionDesc{
        offset: @as(u64, s1_off),
        byte_len: @as(u32, @intCast(bufs.extern_strings_len * LOCK_ARENA_EXTERN_STR_BYTES)),
        elem_count: @as(u32, @intCast(bufs.extern_strings_len)),
    });

    // Section 2: dependencies
    pos = alignLockArenaOffset(pos, LOCK_ARENA_ALIGNMENT);
    const s2_off = pos;
    let idx2: usize = 0;
    while (idx2 < bufs.dependencies_len) {
        const dep = bufs.dependencies[idx2];
        laWriteU32Le(out, pos + 0, dep.name_ref.offset);
        laWriteU32Le(out, pos + 4, dep.name_ref.len);
        laWriteU32Le(out, pos + 8, dep.spec_ref.offset);
        laWriteU32Le(out, pos + 12, dep.spec_ref.len);
        laWriteU16Le(out, pos + 16, dep.behavior_flags);
        out[pos + 18] = lockArenaDepKindToByte(dep.kind);
        out[pos + 19] = 0;
        laWriteU32Le(out, pos + 20, dep.owner_pkg_id);
        pos += LOCK_ARENA_DEP_BYTES;
        idx2 += 1;
    }
    laWriteSectionDesc(out, 2, LockArenaSectionDesc{
        offset: @as(u64, s2_off),
        byte_len: @as(u32, @intCast(bufs.dependencies_len * LOCK_ARENA_DEP_BYTES)),
        elem_count: @as(u32, @intCast(bufs.dependencies_len)),
    });

    // Section 3: resolutions
    pos = alignLockArenaOffset(pos, LOCK_ARENA_ALIGNMENT);
    const s3_off = pos;
    let idx3: usize = 0;
    while (idx3 < bufs.resolutions_len) {
        laWriteU32Le(out, pos, bufs.resolutions[idx3]);
        pos += 4;
        idx3 += 1;
    }
    laWriteSectionDesc(out, 3, LockArenaSectionDesc{
        offset: @as(u64, s3_off),
        byte_len: @as(u32, @intCast(bufs.resolutions_len * 4)),
        elem_count: @as(u32, @intCast(bufs.resolutions_len)),
    });

    // Section 4: trees
    pos = alignLockArenaOffset(pos, LOCK_ARENA_ALIGNMENT);
    const s4_off = pos;
    let idx4: usize = 0;
    while (idx4 < bufs.trees_len) {
        const node = bufs.trees[idx4];
        laWriteU32Le(out, pos + 0, node.parent_tree_id);
        laWriteU32Le(out, pos + 4, node.dep_id);
        laWriteU32Le(out, pos + 8, node.deps_start);
        laWriteU32Le(out, pos + 12, node.deps_len);
        pos += LOCK_ARENA_TREE_BYTES;
        idx4 += 1;
    }
    laWriteSectionDesc(out, 4, LockArenaSectionDesc{
        offset: @as(u64, s4_off),
        byte_len: @as(u32, @intCast(bufs.trees_len * LOCK_ARENA_TREE_BYTES)),
        elem_count: @as(u32, @intCast(bufs.trees_len)),
    });

    // Compute IEEE 802.3 CRC32 over [0..12] + [16..total_len]
    let crc = updateLockArenaCrc32(0, out[0..12]);
    crc = updateLockArenaCrc32(crc, out[16..total_len]);
    laWriteU32Le(out, 12, crc);
    return total_len;
}

fn laValidateSection(
    desc: LockArenaSectionDesc,
    elem_size: usize,
    min_offset: usize,
    image_len: usize,
): LockArenaError!usize {
    if (desc.offset == LOCK_ARENA_DEAD_SENTINEL || desc.offset < @as(u64, LOCK_ARENA_HEADER_BYTES)) {
        return err.InvalidSectionBounds;
    }
    if (desc.offset > @as(u64, image_len)) {
        return err.InvalidSectionBounds;
    }
    const off = @as(usize, @intCast(desc.offset));
    if (!isLockArenaAligned(off, LOCK_ARENA_ALIGNMENT)) {
        return err.MisalignedSection;
    }
    if (off < min_offset) {
        return err.InvalidSectionBounds;
    }
    const blen = @as(usize, desc.byte_len);
    if (blen > image_len - off) {
        return err.TruncatedData;
    }
    const count = @as(usize, desc.elem_count);
    if (elem_size == 0 || count > blen / elem_size || count * elem_size != blen) {
        return err.CorruptRecord;
    }
    return off + blen;
}

/// Zero-copy read-only view over a verified binary lockfile arena image.
pub struct LockArenaImageView {
    image: []const u8,
    version: u16,
    crc32: u32,
    sections: [5]LockArenaSectionDesc,

    /// Validates header, section alignment/bounds, CRC32 checksum, and all internal
    /// string handles & enum tags before returning a zero-copy view.
    static fn open(image: []const u8): LockArenaError!LockArenaImageView {
        if (image.len < LOCK_ARENA_HEADER_BYTES) {
            return err.TruncatedData;
        }
        if (image[0] != LOCK_ARENA_MAGIC[0] ||
            image[1] != LOCK_ARENA_MAGIC[1] ||
            image[2] != LOCK_ARENA_MAGIC[2] ||
            image[3] != LOCK_ARENA_MAGIC[3])
        {
            return err.InvalidMagic;
        }
        const version = laReadU16Le(image, 4);
        if (version != LOCK_ARENA_VERSION) {
            return err.UnsupportedVersion;
        }
        const alignment = @as(usize, laReadU32Le(image, 8));
        if (alignment != LOCK_ARENA_ALIGNMENT) {
            return err.MisalignedSection;
        }
        const stored_crc = laReadU32Le(image, 12);
        const total_bytes = laReadU64Le(image, 16);
        if (total_bytes != @as(u64, image.len)) {
            return err.TruncatedData;
        }

        // Verify CRC32 over header[0..12] + [16..image.len]
        let computed_crc = updateLockArenaCrc32(0, image[0..12]);
        computed_crc = updateLockArenaCrc32(computed_crc, image[16..image.len]);
        if (computed_crc != stored_crc) {
            return err.ChecksumMismatch;
        }

        let secs: [5]LockArenaSectionDesc = [LockArenaSectionDesc.empty(); 5];
        secs[0] = laReadSectionDesc(image, 0);
        secs[1] = laReadSectionDesc(image, 1);
        secs[2] = laReadSectionDesc(image, 2);
        secs[3] = laReadSectionDesc(image, 3);
        secs[4] = laReadSectionDesc(image, 4);

        // Validate monotonic non-overlapping aligned section bounds
        let cursor: usize = LOCK_ARENA_HEADER_BYTES;
        cursor = laValidateSection(secs[0], 1, cursor, image.len)?;
        cursor = laValidateSection(secs[1], LOCK_ARENA_EXTERN_STR_BYTES, cursor, image.len)?;
        cursor = laValidateSection(secs[2], LOCK_ARENA_DEP_BYTES, cursor, image.len)?;
        cursor = laValidateSection(secs[3], 4, cursor, image.len)?;
        cursor = laValidateSection(secs[4], LOCK_ARENA_TREE_BYTES, cursor, image.len)?;
        * = cursor;

        // Enforce parallel length invariant: dependencies.len == resolutions.len
        if (secs[2].elem_count != secs[3].elem_count) {
            return err.CorruptRecord;
        }

        const str_off = @as(usize, @intCast(secs[0].offset));
        const str_len = @as(usize, secs[0].byte_len);
        const str_pool = image[str_off..str_off + str_len];

        // Validate every extern_string handle falls inside string_bytes
        const ext_off = @as(usize, @intCast(secs[1].offset));
        const ext_count = @as(usize, secs[1].elem_count);
        let e: usize = 0;
        while (e < ext_count) {
            const p = ext_off + e * LOCK_ARENA_EXTERN_STR_BYTES;
            const sref = LockExternString{
                offset: laReadU32Le(image, p + 0),
                len: laReadU32Le(image, p + 4),
            };
            * = sref.resolve(str_pool)?;
            e += 1;
        }

        // Validate every dependency record's string handles & enum tag
        const dep_off = @as(usize, @intCast(secs[2].offset));
        const dep_count = @as(usize, secs[2].elem_count);
        let d: usize = 0;
        while (d < dep_count) {
            const p = dep_off + d * LOCK_ARENA_DEP_BYTES;
            const nref = LockExternString{
                offset: laReadU32Le(image, p + 0),
                len: laReadU32Le(image, p + 4),
            };
            const sref = LockExternString{
                offset: laReadU32Le(image, p + 8),
                len: laReadU32Le(image, p + 12),
            };
            * = nref.resolve(str_pool)?;
            * = sref.resolve(str_pool)?;
            if (lockArenaDepKindFromByte(image[p + 18]) == null) {
                return err.CorruptRecord;
            }
            d += 1;
        }

        return LockArenaImageView{
            image: image,
            version: version,
            crc32: computed_crc,
            sections: secs,
        };
    }

    /// Returns a zero-copy slice of the deduplicated UTF-8 string pool.
    fn stringBytes(): []const u8 {
        const off = @as(usize, @intCast(_.sections[0].offset));
        const len = @as(usize, _.sections[0].byte_len);
        return _.image[off..off + len];
    }

    fn externStringCount(): usize {
        return @as(usize, _.sections[1].elem_count);
    }

    fn dependencyCount(): usize {
        return @as(usize, _.sections[2].elem_count);
    }

    fn resolutionCount(): usize {
        return @as(usize, _.sections[3].elem_count);
    }

    fn treeCount(): usize {
        return @as(usize, _.sections[4].elem_count);
    }

    fn getExternString(idx: usize): LockArenaError!LockExternString {
        if (idx >= _.externStringCount()) return err.CorruptRecord;
        const p = @as(usize, @intCast(_.sections[1].offset)) + idx * LOCK_ARENA_EXTERN_STR_BYTES;
        return LockExternString{
            offset: laReadU32Le(_.image, p + 0),
            len: laReadU32Le(_.image, p + 4),
        };
    }

    fn resolveString(sref: LockExternString): LockArenaError![]const u8 {
        return sref.resolve(_.stringBytes());
    }

    fn getDependency(idx: usize): LockArenaError!LockArenaDep {
        if (idx >= _.dependencyCount()) return err.CorruptRecord;
        const p = @as(usize, @intCast(_.sections[2].offset)) + idx * LOCK_ARENA_DEP_BYTES;
        const kind_opt = lockArenaDepKindFromByte(_.image[p + 18]);
        if (kind_opt == null) return err.CorruptRecord;
        return LockArenaDep{
            name_ref: LockExternString{
                offset: laReadU32Le(_.image, p + 0),
                len: laReadU32Le(_.image, p + 4),
            },
            spec_ref: LockExternString{
                offset: laReadU32Le(_.image, p + 8),
                len: laReadU32Le(_.image, p + 12),
            },
            behavior_flags: laReadU16Le(_.image, p + 16),
            kind: kind_opt ?? LockArenaDepKind.Npm,
            owner_pkg_id: laReadU32Le(_.image, p + 20),
        };
    }

    fn getResolution(idx: usize): LockArenaError!u32 {
        if (idx >= _.resolutionCount()) return err.CorruptRecord;
        const p = @as(usize, @intCast(_.sections[3].offset)) + idx * 4;
        return laReadU32Le(_.image, p);
    }

    fn getTreeNode(idx: usize): LockArenaError!LockArenaTreeNode {
        if (idx >= _.treeCount()) return err.CorruptRecord;
        const p = @as(usize, @intCast(_.sections[4].offset)) + idx * LOCK_ARENA_TREE_BYTES;
        return LockArenaTreeNode{
            parent_tree_id: laReadU32Le(_.image, p + 0),
            dep_id: laReadU32Le(_.image, p + 4),
            deps_start: laReadU32Le(_.image, p + 8),
            deps_len: laReadU32Le(_.image, p + 12),
        };
    }

    /// Searches for an exact interned string inside the read-only image using
    /// `cap:simd`-accelerated pool scanning (`findInLockStringPool`).
    fn containsString(needle: []const u8): bool {
        return findInLockStringPool(_.stringBytes(), needle) != null;
    }
}

/// Validates a binary lockfile arena image and hydrates it into a mutable `LockArenaBuffers`.
pub fn decodeLockArenaImageInto(
    image: []const u8,
    dst: *LockArenaBuffers,
): LockArenaError!LockArenaImageSummary {
    const view = LockArenaImageView.open(image)?;

    if (view.stringBytes().len > dst.string_bytes.len ||
        view.externStringCount() > dst.extern_strings.len ||
        view.dependencyCount() > dst.dependencies.len ||
        view.resolutionCount() > dst.resolutions.len ||
        view.treeCount() > dst.trees.len)
    {
        return err.CapacityExceeded;
    }

    dst.reset();

    // Re-intern all extern_strings so the open-addressing intern table is fully populated
    const ext_n = view.externStringCount();
    let e: usize = 0;
    while (e < ext_n) {
        const sref = view.getExternString(e)?;
        const s = view.resolveString(sref)?;
        * = dst.internString(s)?;
        e += 1;
    }

    // Hydrate dependencies and parallel resolutions
    const dep_n = view.dependencyCount();
    let d: usize = 0;
    while (d < dep_n) {
        const dep = view.getDependency(d)?;
        const name_s = view.resolveString(dep.name_ref)?;
        const spec_s = view.resolveString(dep.spec_ref)?;
        const pkg_id = view.getResolution(d)?;
        * = dst.addDependency(name_s, spec_s, dep.kind, dep.behavior_flags, dep.owner_pkg_id, pkg_id)?;
        d += 1;
    }

    // Hydrate hoist tree nodes
    const tree_n = view.treeCount();
    let t: usize = 0;
    while (t < tree_n) {
        const node = view.getTreeNode(t)?;
        * = dst.addTreeNode(node.parent_tree_id, node.dep_id, node.deps_start, node.deps_len)?;
        t += 1;
    }

    return LockArenaImageSummary{
        version: view.version,
        total_image_bytes: image.len,
        string_bytes_len: dst.string_bytes_len,
        extern_strings_count: dst.extern_strings_len,
        dependencies_count: dst.dependencies_len,
        resolutions_count: dst.resolutions_len,
        trees_count: dst.trees_len,
        crc32: view.crc32,
    };
}

// ============================================================================
// Section 6: Human-Readable Diagnostic Summary Formatter
// ============================================================================

/// Formats a concise diagnostic summary of arena occupancy and string deduplication savings.
pub fn formatLockArenaReport(bufs: *const LockArenaBuffers, out: []u8): LockArenaError!usize {
    const st = bufs.stats();
    const saved_pct: u64 = if (st.raw_input_string_bytes == 0) 0 else (st.dedup_saved_bytes * 100) / st.raw_input_string_bytes;

    let pos: usize = 0;
    laAppendBytes(out, &pos, "LockArena(strings=")?;
    laAppendU64(out, &pos, @as(u64, st.extern_strings_count))?;
    laAppendBytes(out, &pos, ", str_bytes=")?;
    laAppendU64(out, &pos, @as(u64, st.string_bytes_used))?;
    laAppendByte(out, &pos, '/')?;
    laAppendU64(out, &pos, @as(u64, st.string_bytes_cap))?;
    laAppendBytes(out, &pos, ", saved=")?;
    laAppendU64(out, &pos, st.dedup_saved_bytes)?;
    laAppendBytes(out, &pos, "B(")?;
    laAppendU64(out, &pos, saved_pct)?;
    laAppendBytes(out, &pos, "%), deps=")?;
    laAppendU64(out, &pos, @as(u64, st.dependencies_count))?;
    laAppendBytes(out, &pos, ", trees=")?;
    laAppendU64(out, &pos, @as(u64, st.trees_count))?;
    laAppendByte(out, &pos, ')')?;
    return pos;
}
