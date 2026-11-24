// Source: Absorbed from /data/dev/bun/src/install/lockfile/Package.cp | Synced: 2026-11-23T20:29:24Z
// ============================================================================
// lib/pkg/lockPackageStore.cp — Compact Lockfile Package Store, BFS Pruner & CRC32 Snapshot
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven in-memory lockfile package index for `cap.lock`:
//   1. Contiguous byte-arena string pool + 8-byte `(offset, len)` `LockStrRef`
//      handles + open-addressing `name_hash` index (`LockPackageStore`) supporting
//      O(1) lookup of multi-version packages and deduplicated insertion;
//   2. Full resolution tag model (`LockResolutionKind`: `Root`, `Npm`, `Workspace`,
//      `Folder`, `LocalTarball`, `RemoteTarball`, `Git`, `GitHub`, `Symlink`)
//      with `os_mask`/`cpu_mask`/`bin_kind`/`has_scripts` metadata flags;
//   3. Zero-heap BFS reachability pruner (`pruneUnreachablePackages`) and
//      multi-version duplicate package auditor (`findDuplicatePackageVersions`);
//   4. IEEE CRC32-verified compact binary snapshot codec (`encodeLockStoreSnapshot`
//      & `decodeLockStoreSnapshot`) with strict bounds validation.

import { Simd } from "cap:simd";

/// 4-byte binary snapshot magic header ("CLPS" = Cap Lock Package Store).
pub const LOCK_STORE_SNAPSHOT_MAGIC: [4]u8 = ['C', 'L', 'P', 'S'];

/// Current binary snapshot wire format version.
pub const LOCK_STORE_SNAPSHOT_VERSION: u16 = 1;

/// Sentinel package ID indicating an unassigned or invalid package.
pub const LOCK_STORE_INVALID_ID: u32 = 0xFFFFFFFF;

/// Sentinel bucket marker indicating an empty open-addressing hash slot.
pub const LOCK_STORE_EMPTY_BUCKET: u32 = 0xFFFFFFFF;

/// Error set for compact package store indexing, pruning, and binary snapshots.
pub err LockPackageStoreError {
    InvalidPackageId,
    StringBufferFull,
    SliceCapacityExceeded,
    HashTableFull,
    CorruptSnapshotMagic,
    UnsupportedSnapshotVersion,
    SnapshotChecksumMismatch,
    TruncatedSnapshot,
    InvalidRecordBounds,
    BufferTooSmall,
}

// ============================================================================
// Internal Zero-Alloc Hashing, CRC32, and Little-Endian Codec Helpers
// ============================================================================

const LS_FNV1A_OFFSET: u64 = 14695981039346656037;
const LS_FNV1A_PRIME: u64 = 1099511628211;
const LS_HEADER_BYTES: usize = 24;
const LS_RECORD_WIRE_BYTES: usize = 56;
const LS_TRAILER_BYTES: usize = 4;

fn lsBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// Computes a deterministic 64-bit hash for a package name (FNV-1a + SplitMix64).
pub fn hashLockPackageName64(name: []const u8): u64 {
    let h = LS_FNV1A_OFFSET;
    let i: usize = 0;
    while (i < name.len) {
        h = (h ^ @as(u64, name[i])) *% LS_FNV1A_PRIME;
        i += 1;
    }
    h = (h ^ (h >> 30)) *% 0xbf58476d1ce4e5b9;
    h = (h ^ (h >> 27)) *% 0x94d049bb133111eb;
    return h ^ (h >> 31);
}

/// Computes standard IEEE 802.3 CRC32 checksum over `data`.
pub fn computeLockStoreCrc32(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) {
        crc = crc ^ @as(u32, data[i]);
        let bit: u32 = 0;
        while (bit < 8) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
            bit += 1;
        }
        i += 1;
    }
    return ~crc;
}

fn lsWriteU16Le(out: []u8, pos: *usize, v: u16)! {
    if (2 > out.len - pos.*) return err.BufferTooSmall;
    out[pos.*] = @as(u8, @truncate(v));
    out[pos.* + 1] = @as(u8, @truncate(v >> 8));
    pos.* += 2;
}

fn lsWriteU32Le(out: []u8, pos: *usize, v: u32)! {
    if (4 > out.len - pos.*) return err.BufferTooSmall;
    out[pos.*] = @as(u8, @truncate(v));
    out[pos.* + 1] = @as(u8, @truncate(v >> 8));
    out[pos.* + 2] = @as(u8, @truncate(v >> 16));
    out[pos.* + 3] = @as(u8, @truncate(v >> 24));
    pos.* += 4;
}

fn lsWriteU64Le(out: []u8, pos: *usize, v: u64)! {
    if (8 > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < 8) {
        const sh: u6 = @as(u6, @intCast(i * 8));
        out[pos.* + i] = @as(u8, @truncate(v >> sh));
        i += 1;
    }
    pos.* += 8;
}

fn lsReadU16Le(src: []const u8, pos: *usize): LockPackageStoreError!u16 {
    if (2 > src.len - pos.*) return err.TruncatedSnapshot;
    const v: u16 = @as(u16, src[pos.*]) | (@as(u16, src[pos.* + 1]) << 8);
    pos.* += 2;
    return v;
}

fn lsReadU32Le(src: []const u8, pos: *usize): LockPackageStoreError!u32 {
    if (4 > src.len - pos.*) return err.TruncatedSnapshot;
    const v: u32 = @as(u32, src[pos.*]) |
        (@as(u32, src[pos.* + 1]) << 8) |
        (@as(u32, src[pos.* + 2]) << 16) |
        (@as(u32, src[pos.* + 3]) << 24);
    pos.* += 4;
    return v;
}

fn lsReadU64Le(src: []const u8, pos: *usize): LockPackageStoreError!u64 {
    if (8 > src.len - pos.*) return err.TruncatedSnapshot;
    let v: u64 = 0;
    let i: usize = 0;
    while (i < 8) {
        const sh: u6 = @as(u6, @intCast(i * 8));
        v = v | (@as(u64, src[pos.* + i]) << sh);
        i += 1;
    }
    pos.* += 8;
    return v;
}

// ============================================================================
// Section 1: Compact String Refs, Resolution Tags & Open-Addressing Store
// ============================================================================

/// 8-byte compact offset/length reference into `LockPackageStore.string_buf`.
pub struct LockStrRef {
    offset: u32 = 0,
    len: u32 = 0,

    fn slice(string_buf: []const u8): []const u8 {
        const start: usize = @as(usize, _.offset);
        const count: usize = @as(usize, _.len);
        if (start > string_buf.len || count > string_buf.len - start) return "";
        return string_buf[start..start + count];
    }
}

/// Resolution source kind for a locked package record.
pub enum LockResolutionKind: u8 {
    Root = 0,
    Npm = 1,
    Workspace = 2,
    Folder = 3,
    LocalTarball = 4,
    RemoteTarball = 5,
    Git = 6,
    GitHub = 7,
    Symlink = 8,
}

fn lockResolutionFromU8(raw: u8): LockResolutionKind? {
    if (raw == 0) return LockResolutionKind.Root;
    if (raw == 1) return LockResolutionKind.Npm;
    if (raw == 2) return LockResolutionKind.Workspace;
    if (raw == 3) return LockResolutionKind.Folder;
    if (raw == 4) return LockResolutionKind.LocalTarball;
    if (raw == 5) return LockResolutionKind.RemoteTarball;
    if (raw == 6) return LockResolutionKind.Git;
    if (raw == 7) return LockResolutionKind.GitHub;
    if (raw == 8) return LockResolutionKind.Symlink;
    return null;
}

/// Returns the canonical lowercase label for `kind`.
pub fn lockResolutionKindName(kind: LockResolutionKind): []const u8 {
    if (kind == LockResolutionKind.Root) return "root";
    if (kind == LockResolutionKind.Npm) return "npm";
    if (kind == LockResolutionKind.Workspace) return "workspace";
    if (kind == LockResolutionKind.Folder) return "folder";
    if (kind == LockResolutionKind.LocalTarball) return "local_tarball";
    if (kind == LockResolutionKind.RemoteTarball) return "remote_tarball";
    if (kind == LockResolutionKind.Git) return "git";
    if (kind == LockResolutionKind.GitHub) return "github";
    return "symlink";
}

/// Executable bin layout kind for a locked package.
pub enum LockBinKind: u8 {
    None = 0,
    SingleFile = 1,
    NamedMap = 2,
    Directory = 3,
}

fn lockBinKindFromU8(raw: u8): LockBinKind? {
    if (raw == 0) return LockBinKind.None;
    if (raw == 1) return LockBinKind.SingleFile;
    if (raw == 2) return LockBinKind.NamedMap;
    if (raw == 3) return LockBinKind.Directory;
    return null;
}

/// Compact platform & lifecycle metadata flags stored per package record.
pub struct LockPackageMeta {
    os_mask: u16 = 0xFFFF,
    cpu_mask: u16 = 0xFFFF,
    bin_kind: LockBinKind = LockBinKind.None,
    has_scripts: bool = false,
    is_trusted: bool = false,
    is_patched: bool = false,

    static fn defaults(): LockPackageMeta {
        return LockPackageMeta{};
    }
}

/// Compact fixed-size package record inside `LockPackageStore`.
pub struct LockPackageRecord {
    id: u32 = 0,
    name_hash: u64 = 0,
    name_ref: LockStrRef = LockStrRef{},
    version_ref: LockStrRef = LockStrRef{},
    url_ref: LockStrRef = LockStrRef{},
    integrity_ref: LockStrRef = LockStrRef{},
    resolution: LockResolutionKind = LockResolutionKind.Npm,
    meta: LockPackageMeta = LockPackageMeta{},
    deps_start: u32 = 0,
    deps_len: u32 = 0,

    static fn empty(): LockPackageRecord {
        return LockPackageRecord{};
    }
}

/// Structured input for inserting a package into `LockPackageStore`.
pub struct LockPackageInput {
    name: []const u8 = "",
    version: []const u8 = "",
    tarball_url: []const u8 = "",
    integrity: []const u8 = "",
    resolution: LockResolutionKind = LockResolutionKind.Npm,
    meta: LockPackageMeta = LockPackageMeta{},
}

/// Zero-heap, caller-slice-backed compact lockfile package store with contiguous
/// string arena and open-addressing `name_hash` index.
pub struct LockPackageStore {
    records: []LockPackageRecord,
    record_count: usize = 0,
    dep_edges: []u32,
    edge_count: usize = 0,
    string_buf: []u8,
    string_used: usize = 0,
    hash_buckets: []u32,

    static fn init(
        records_buf: []LockPackageRecord,
        dep_edges_buf: []u32,
        string_buf: []u8,
        hash_buckets: []u32,
    ): LockPackageStore {
        let i: usize = 0;
        while (i < hash_buckets.len) {
            hash_buckets[i] = LOCK_STORE_EMPTY_BUCKET;
            i += 1;
        }
        return LockPackageStore{
            records: records_buf,
            record_count: 0,
            dep_edges: dep_edges_buf,
            edge_count: 0,
            string_buf: string_buf,
            string_used: 0,
            hash_buckets: hash_buckets,
        };
    }

    /// Interns `s` into the contiguous `string_buf` and returns its 8-byte `LockStrRef`.
    fn internString(*_, s: []const u8): LockPackageStoreError!LockStrRef {
        if (s.len == 0) return LockStrRef{ offset: 0, len: 0 };
        if (s.len > _.string_buf.len - _.string_used) {
            return err.StringBufferFull;
        }
        const off = _.string_used;
        let i: usize = 0;
        while (i < s.len) {
            _.string_buf[off + i] = s[i];
            i += 1;
        }
        _.string_used += s.len;
        return LockStrRef{
            offset: @as(u32, @intCast(off)),
            len: @as(u32, @intCast(s.len)),
        };
    }

    fn insertHashBucket(*_, name_hash: u64, pkg_id: u32)! {
        const cap = _.hash_buckets.len;
        if (cap == 0) return err.HashTableFull;
        let slot: usize = @as(usize, @intCast(name_hash % @as(u64, cap)));
        let probes: usize = 0;
        while (probes < cap) {
            if (_.hash_buckets[slot] == LOCK_STORE_EMPTY_BUCKET) {
                _.hash_buckets[slot] = pkg_id;
                return;
            }
            slot = (slot + 1) % cap;
            probes += 1;
        }
        return err.HashTableFull;
    }

    /// Inserts a package and its dependency IDs into the store.
    /// If an identical `(name, version, resolution)` record already exists, returns
    /// its existing `package_id` without duplicating storage.
    fn addPackage(
        *_,
        input: LockPackageInput,
        dep_ids: []const u32,
    ): LockPackageStoreError!u32 {
        const nh = hashLockPackageName64(input.name);

        // Check existing entries in open-addressing probe chain for exact dedup
        if (_.hash_buckets.len > 0) {
            const cap = _.hash_buckets.len;
            let slot: usize = @as(usize, @intCast(nh % @as(u64, cap)));
            let probes: usize = 0;
            while (probes < cap) {
                const entry = _.hash_buckets[slot];
                if (entry == LOCK_STORE_EMPTY_BUCKET) break;
                const rec = _.records[@as(usize, entry)];
                if (rec.name_hash == nh &&
                    rec.resolution == input.resolution &&
                    lsBytesEql(rec.name_ref.slice(_.string_buf[0.._.string_used]), input.name) &&
                    lsBytesEql(rec.version_ref.slice(_.string_buf[0.._.string_used]), input.version))
                {
                    return rec.id;
                }
                slot = (slot + 1) % cap;
                probes += 1;
            }
        }

        if (_.record_count >= _.records.len) return err.SliceCapacityExceeded;
        if (dep_ids.len > _.dep_edges.len - _.edge_count) return err.SliceCapacityExceeded;

        const name_ref = _.internString(input.name)?;
        const ver_ref = _.internString(input.version)?;
        const url_ref = _.internString(input.tarball_url)?;
        const int_ref = _.internString(input.integrity)?;

        const deps_start = _.edge_count;
        let d: usize = 0;
        while (d < dep_ids.len) {
            _.dep_edges[deps_start + d] = dep_ids[d];
            d += 1;
        }
        _.edge_count += dep_ids.len;

        const id: u32 = @as(u32, @intCast(_.record_count));
        _.records[_.record_count] = LockPackageRecord{
            id: id,
            name_hash: nh,
            name_ref: name_ref,
            version_ref: ver_ref,
            url_ref: url_ref,
            integrity_ref: int_ref,
            resolution: input.resolution,
            meta: input.meta,
            deps_start: @as(u32, @intCast(deps_start)),
            deps_len: @as(u32, @intCast(dep_ids.len)),
        };
        _.record_count += 1;
        _.insertHashBucket(nh, id)?;
        return id;
    }

    /// Appends additional dependency edge IDs to an existing package record (useful
    /// when resolving cyclic or forward-referenced package graphs).
    fn setPackageDeps(*_, pkg_id: u32, dep_ids: []const u32)! {
        const idx: usize = @as(usize, pkg_id);
        if (idx >= _.record_count) return err.InvalidPackageId;
        if (dep_ids.len > _.dep_edges.len - _.edge_count) return err.SliceCapacityExceeded;

        const deps_start = _.edge_count;
        let d: usize = 0;
        while (d < dep_ids.len) {
            _.dep_edges[deps_start + d] = dep_ids[d];
            d += 1;
        }
        _.edge_count += dep_ids.len;

        const prev = _.records[idx];
        _.records[idx] = LockPackageRecord{
            id: prev.id,
            name_hash: prev.name_hash,
            name_ref: prev.name_ref,
            version_ref: prev.version_ref,
            url_ref: prev.url_ref,
            integrity_ref: prev.integrity_ref,
            resolution: prev.resolution,
            meta: prev.meta,
            deps_start: @as(u32, @intCast(deps_start)),
            deps_len: @as(u32, @intCast(dep_ids.len)),
        };
    }

    /// O(1) amortized lookup returning all package IDs matching `name` (supporting
    /// multiple co-installed versions) into `out_ids`.
    fn findAllByName(name: []const u8, out_ids: []u32): usize {
        if (_.hash_buckets.len == 0) return 0;
        const nh = hashLockPackageName64(name);
        const cap = _.hash_buckets.len;
        let slot: usize = @as(usize, @intCast(nh % @as(u64, cap)));
        let probes: usize = 0;
        let found: usize = 0;

        while (probes < cap) {
            const entry = _.hash_buckets[slot];
            if (entry == LOCK_STORE_EMPTY_BUCKET) break;
            const rec = _.records[@as(usize, entry)];
            if (rec.name_hash == nh && lsBytesEql(rec.name_ref.slice(_.string_buf[0.._.string_used]), name)) {
                if (found < out_ids.len) {
                    out_ids[found] = rec.id;
                    found += 1;
                }
            }
            slot = (slot + 1) % cap;
            probes += 1;
        }
        return found;
    }

    /// Returns the package name slice for `pkg_id`.
    fn getName(pkg_id: u32): []const u8 {
        const idx: usize = @as(usize, pkg_id);
        if (idx >= _.record_count) return "";
        return _.records[idx].name_ref.slice(_.string_buf[0.._.string_used]);
    }

    /// Returns the package version slice for `pkg_id`.
    fn getVersion(pkg_id: u32): []const u8 {
        const idx: usize = @as(usize, pkg_id);
        if (idx >= _.record_count) return "";
        return _.records[idx].version_ref.slice(_.string_buf[0.._.string_used]);
    }

    /// Returns the tarball/git URL slice for `pkg_id`.
    fn getUrl(pkg_id: u32): []const u8 {
        const idx: usize = @as(usize, pkg_id);
        if (idx >= _.record_count) return "";
        return _.records[idx].url_ref.slice(_.string_buf[0.._.string_used]);
    }

    /// Returns the SRI integrity slice for `pkg_id`.
    fn getIntegrity(pkg_id: u32): []const u8 {
        const idx: usize = @as(usize, pkg_id);
        if (idx >= _.record_count) return "";
        return _.records[idx].integrity_ref.slice(_.string_buf[0.._.string_used]);
    }

    /// Returns the dependency target package IDs slice for `pkg_id`.
    fn getDepIds(pkg_id: u32): []const u32 {
        const idx: usize = @as(usize, pkg_id);
        if (idx >= _.record_count) return [];
        const rec = _.records[idx];
        const start: usize = @as(usize, rec.deps_start);
        const len: usize = @as(usize, rec.deps_len);
        if (start > _.edge_count || len > _.edge_count - start) return [];
        return _.dep_edges[start..start + len];
    }
}

// ============================================================================
// Section 2: BFS Reachability Pruning & Duplicate Multi-Version Auditor
// ============================================================================

/// Summary returned by `pruneUnreachablePackages`.
pub struct LockPruneSummary {
    total_packages: usize = 0,
    reachable_count: usize = 0,
    unreachable_count: usize = 0,
    roots_processed: usize = 0,
}

/// Performs zero-heap BFS from `root_ids` across `store`, marking `reachable_flags[id] = 1`
/// for reachable packages and collecting unreachable orphan package IDs into `out_orphan_ids`.
/// Caller provides `reachable_flags` (`len >= store.record_count`) and `bfs_queue` (`len >= store.record_count`).
pub fn pruneUnreachablePackages(
    store: LockPackageStore,
    root_ids: []const u32,
    reachable_flags: []u8,
    bfs_queue: []u32,
    out_orphan_ids: []u32,
): LockPackageStoreError!LockPruneSummary {
    const n = store.record_count;
    if (reachable_flags.len < n || bfs_queue.len < n) {
        return err.SliceCapacityExceeded;
    }

    let i: usize = 0;
    while (i < n) {
        reachable_flags[i] = 0;
        i += 1;
    }

    let q_head: usize = 0;
    let q_tail: usize = 0;
    let roots_ok: usize = 0;

    let r: usize = 0;
    while (r < root_ids.len) {
        const rid: usize = @as(usize, root_ids[r]);
        if (rid >= n) return err.InvalidPackageId;
        if (reachable_flags[rid] == 0) {
            reachable_flags[rid] = 1;
            bfs_queue[q_tail] = root_ids[r];
            q_tail += 1;
            roots_ok += 1;
        }
        r += 1;
    }

    // BFS traversal
    while (q_head < q_tail) {
        const cur = bfs_queue[q_head];
        q_head += 1;
        const deps = store.getDepIds(cur);
        let d: usize = 0;
        while (d < deps.len) {
            const target = deps[d];
            const tidx: usize = @as(usize, target);
            if (tidx < n && reachable_flags[tidx] == 0) {
                reachable_flags[tidx] = 1;
                bfs_queue[q_tail] = target;
                q_tail += 1;
            }
            d += 1;
        }
    }

    // Use SIMD countByte to tally reachable flags quickly and collect orphan IDs
    const reachable_count = Simd.countByte(reachable_flags[0..n], 1);
    let orphan_count: usize = 0;
    let k: usize = 0;
    while (k < n) {
        if (reachable_flags[k] == 0) {
            if (orphan_count >= out_orphan_ids.len) return err.SliceCapacityExceeded;
            out_orphan_ids[orphan_count] = @as(u32, @intCast(k));
            orphan_count += 1;
        }
        k += 1;
    }

    return LockPruneSummary{
        total_packages: n,
        reachable_count: reachable_count,
        unreachable_count: orphan_count,
        roots_processed: roots_ok,
    };
}

/// Audit record describing a package name with multiple co-installed versions.
pub struct DuplicatePkgVersionEntry {
    name_ref: LockStrRef = LockStrRef{},
    version_count: u32 = 0,
    first_pkg_id: u32 = 0,

    static fn empty(): DuplicatePkgVersionEntry {
        return DuplicatePkgVersionEntry{};
    }
}

/// Summary returned by `findDuplicatePackageVersions`.
pub struct DuplicatePkgAuditSummary {
    unique_names: usize = 0,
    duplicated_names: usize = 0,
    extra_version_instances: u32 = 0,
}

/// Audits `store` to identify package names installed at >=2 distinct versions.
/// `visited_flags` must have length `>= store.record_count`.
pub fn findDuplicatePackageVersions(
    store: LockPackageStore,
    visited_flags: []u8,
    out_dups: []DuplicatePkgVersionEntry,
): LockPackageStoreError!DuplicatePkgAuditSummary {
    const n = store.record_count;
    if (visited_flags.len < n) return err.SliceCapacityExceeded;

    let z: usize = 0;
    while (z < n) {
        visited_flags[z] = 0;
        z += 1;
    }

    let sum = DuplicatePkgAuditSummary{};
    let i: usize = 0;
    while (i < n) {
        if (visited_flags[i] != 0) {
            i += 1;
            continue;
        }
        visited_flags[i] = 1;
        sum.unique_names += 1;

        const base = store.records[i];
        const base_name = base.name_ref.slice(store.string_buf[0..store.string_used]);
        let count: u32 = 1;

        let j = i + 1;
        while (j < n) {
            if (visited_flags[j] == 0 && store.records[j].name_hash == base.name_hash) {
                const cand_name = store.records[j].name_ref.slice(store.string_buf[0..store.string_used]);
                if (lsBytesEql(base_name, cand_name)) {
                    visited_flags[j] = 1;
                    count += 1;
                }
            }
            j += 1;
        }

        if (count > 1) {
            if (sum.duplicated_names >= out_dups.len) return err.SliceCapacityExceeded;
            out_dups[sum.duplicated_names] = DuplicatePkgVersionEntry{
                name_ref: base.name_ref,
                version_count: count,
                first_pkg_id: base.id,
            };
            sum.duplicated_names += 1;
            sum.extra_version_instances += count - 1;
        }
        i += 1;
    }

    return sum;
}

// ============================================================================
// Section 3: CRC32-Verified Compact Binary Snapshot Codec
// ============================================================================

/// Encodes `store` into a compact, little-endian binary snapshot with a 24-byte
/// header and trailing IEEE CRC32 checksum into caller-owned `out_buf`.
pub fn encodeLockStoreSnapshot(
    store: LockPackageStore,
    out_buf: []u8,
): LockPackageStoreError!usize {
    const body_records_bytes = store.record_count * LS_RECORD_WIRE_BYTES;
    const body_edges_bytes = store.edge_count * 4;
    const total_bytes = LS_HEADER_BYTES + body_records_bytes + body_edges_bytes + store.string_used + LS_TRAILER_BYTES;
    if (total_bytes > out_buf.len) return err.BufferTooSmall;

    let pos: usize = 0;
    // Header (24 bytes)
    out_buf[0] = LOCK_STORE_SNAPSHOT_MAGIC[0];
    out_buf[1] = LOCK_STORE_SNAPSHOT_MAGIC[1];
    out_buf[2] = LOCK_STORE_SNAPSHOT_MAGIC[2];
    out_buf[3] = LOCK_STORE_SNAPSHOT_MAGIC[3];
    pos = 4;
    lsWriteU16Le(out_buf, &pos, LOCK_STORE_SNAPSHOT_VERSION)?;
    lsWriteU16Le(out_buf, &pos, 0)?; // reserved flags
    lsWriteU32Le(out_buf, &pos, @as(u32, @intCast(store.record_count)))?;
    lsWriteU32Le(out_buf, &pos, @as(u32, @intCast(store.edge_count)))?;
    lsWriteU32Le(out_buf, &pos, @as(u32, @intCast(store.string_used)))?;
    lsWriteU32Le(out_buf, &pos, 0)?; // reserved

    // Records (56 bytes each)
    let i: usize = 0;
    while (i < store.record_count) {
        const rec = store.records[i];
        lsWriteU32Le(out_buf, &pos, rec.id)?;
        lsWriteU64Le(out_buf, &pos, rec.name_hash)?;
        lsWriteU32Le(out_buf, &pos, rec.name_ref.offset)?;
        lsWriteU32Le(out_buf, &pos, rec.name_ref.len)?;
        lsWriteU32Le(out_buf, &pos, rec.version_ref.offset)?;
        lsWriteU32Le(out_buf, &pos, rec.version_ref.len)?;
        lsWriteU32Le(out_buf, &pos, rec.url_ref.offset)?;
        lsWriteU32Le(out_buf, &pos, rec.url_ref.len)?;
        lsWriteU32Le(out_buf, &pos, rec.integrity_ref.offset)?;
        lsWriteU32Le(out_buf, &pos, rec.integrity_ref.len)?;
        lsWriteU16Le(out_buf, &pos, rec.meta.os_mask)?;
        lsWriteU16Le(out_buf, &pos, rec.meta.cpu_mask)?;

        const res_byte: u8 = @as(u8, @intFromEnum(rec.resolution));
        const bin_byte: u8 = @as(u8, @intFromEnum(rec.meta.bin_kind));
        let flag_byte: u8 = 0;
        if (rec.meta.has_scripts) flag_byte = flag_byte | 1;
        if (rec.meta.is_trusted) flag_byte = flag_byte | 2;
        if (rec.meta.is_patched) flag_byte = flag_byte | 4;
        out_buf[pos] = res_byte;
        out_buf[pos + 1] = bin_byte;
        out_buf[pos + 2] = flag_byte;
        out_buf[pos + 3] = 0;
        pos += 4;

        lsWriteU32Le(out_buf, &pos, rec.deps_start)?;
        lsWriteU32Le(out_buf, &pos, rec.deps_len)?;
        i += 1;
    }

    // Dependency edge table
    let e: usize = 0;
    while (e < store.edge_count) {
        lsWriteU32Le(out_buf, &pos, store.dep_edges[e])?;
        e += 1;
    }

    // Contiguous string pool
    let s: usize = 0;
    while (s < store.string_used) {
        out_buf[pos + s] = store.string_buf[s];
        s += 1;
    }
    pos += store.string_used;

    // Trailer: IEEE CRC32 over header + body
    const crc = computeLockStoreCrc32(out_buf[0..pos]);
    lsWriteU32Le(out_buf, &pos, crc)?;
    return pos;
}

/// Decodes and strictly validates a binary snapshot produced by `encodeLockStoreSnapshot`,
/// verifying magic, format version, IEEE CRC32 checksum, all string/edge bounds, and
/// rebuilding the open-addressing `name_hash` index with zero heap allocation.
pub fn decodeLockStoreSnapshot(
    data: []const u8,
    records_buf: []LockPackageRecord,
    dep_edges_buf: []u32,
    string_buf: []u8,
    hash_buckets: []u32,
): LockPackageStoreError!LockPackageStore {
    if (data.len < LS_HEADER_BYTES + LS_TRAILER_BYTES) {
        return err.TruncatedSnapshot;
    }

    if (data[0] != LOCK_STORE_SNAPSHOT_MAGIC[0] ||
        data[1] != LOCK_STORE_SNAPSHOT_MAGIC[1] ||
        data[2] != LOCK_STORE_SNAPSHOT_MAGIC[2] ||
        data[3] != LOCK_STORE_SNAPSHOT_MAGIC[3])
    {
        return err.CorruptSnapshotMagic;
    }

    let pos: usize = 4;
    const version = lsReadU16Le(data, &pos)?;
    if (version != LOCK_STORE_SNAPSHOT_VERSION) {
        return err.UnsupportedSnapshotVersion;
    }
    * = lsReadU16Le(data, &pos)?; // flags
    const rec_count_u32 = lsReadU32Le(data, &pos)?;
    const edge_count_u32 = lsReadU32Le(data, &pos)?;
    const str_bytes_u32 = lsReadU32Le(data, &pos)?;
    * = lsReadU32Le(data, &pos)?; // reserved

    const rec_count: usize = @as(usize, rec_count_u32);
    const edge_count: usize = @as(usize, edge_count_u32);
    const str_bytes: usize = @as(usize, str_bytes_u32);

    // Overflow-safe total size check before reading body
    if (rec_count > records_buf.len || edge_count > dep_edges_buf.len || str_bytes > string_buf.len) {
        return err.SliceCapacityExceeded;
    }
    const expected_payload = LS_HEADER_BYTES + rec_count * LS_RECORD_WIRE_BYTES + edge_count * 4 + str_bytes;
    if (data.len != expected_payload + LS_TRAILER_BYTES) {
        return err.TruncatedSnapshot;
    }

    // Verify IEEE CRC32 over header + body
    let crc_pos = expected_payload;
    const stored_crc = lsReadU32Le(data, &crc_pos)?;
    const actual_crc = computeLockStoreCrc32(data[0..expected_payload]);
    if (stored_crc != actual_crc) {
        return err.SnapshotChecksumMismatch;
    }

    let store = LockPackageStore.init(records_buf, dep_edges_buf, string_buf, hash_buckets);

    // Decode records with strict bounds validation
    let i: usize = 0;
    while (i < rec_count) {
        const id = lsReadU32Le(data, &pos)?;
        const name_hash = lsReadU64Le(data, &pos)?;
        const name_off = lsReadU32Le(data, &pos)?;
        const name_len = lsReadU32Le(data, &pos)?;
        const ver_off = lsReadU32Le(data, &pos)?;
        const ver_len = lsReadU32Le(data, &pos)?;
        const url_off = lsReadU32Le(data, &pos)?;
        const url_len = lsReadU32Le(data, &pos)?;
        const int_off = lsReadU32Le(data, &pos)?;
        const int_len = lsReadU32Le(data, &pos)?;
        const os_mask = lsReadU16Le(data, &pos)?;
        const cpu_mask = lsReadU16Le(data, &pos)?;

        if (4 > data.len - pos) return err.TruncatedSnapshot;
        const res_opt = lockResolutionFromU8(data[pos]);
        const bin_opt = lockBinKindFromU8(data[pos + 1]);
        const flag_byte = data[pos + 2];
        pos += 4;

        let res_kind = LockResolutionKind.Npm;
        if (rk := res_opt) {
            res_kind = rk;
        } else {
            return err.InvalidRecordBounds;
        }
        let bin_kind = LockBinKind.None;
        if (bk := bin_opt) {
            bin_kind = bk;
        } else {
            return err.InvalidRecordBounds;
        }

        const deps_start = lsReadU32Le(data, &pos)?;
        const deps_len = lsReadU32Le(data, &pos)?;

        // Validate string & edge slice bounds
        if (id != @as(u32, @intCast(i))) return err.InvalidRecordBounds;
        if (@as(usize, name_off) > str_bytes || @as(usize, name_len) > str_bytes - @as(usize, name_off)) return err.InvalidRecordBounds;
        if (@as(usize, ver_off) > str_bytes || @as(usize, ver_len) > str_bytes - @as(usize, ver_off)) return err.InvalidRecordBounds;
        if (@as(usize, url_off) > str_bytes || @as(usize, url_len) > str_bytes - @as(usize, url_off)) return err.InvalidRecordBounds;
        if (@as(usize, int_off) > str_bytes || @as(usize, int_len) > str_bytes - @as(usize, int_off)) return err.InvalidRecordBounds;
        if (@as(usize, deps_start) > edge_count || @as(usize, deps_len) > edge_count - @as(usize, deps_start)) return err.InvalidRecordBounds;

        store.records[i] = LockPackageRecord{
            id: id,
            name_hash: name_hash,
            name_ref: LockStrRef{ offset: name_off, len: name_len },
            version_ref: LockStrRef{ offset: ver_off, len: ver_len },
            url_ref: LockStrRef{ offset: url_off, len: url_len },
            integrity_ref: LockStrRef{ offset: int_off, len: int_len },
            resolution: res_kind,
            meta: LockPackageMeta{
                os_mask: os_mask,
                cpu_mask: cpu_mask,
                bin_kind: bin_kind,
                has_scripts: (flag_byte & 1) != 0,
                is_trusted: (flag_byte & 2) != 0,
                is_patched: (flag_byte & 4) != 0,
            },
            deps_start: deps_start,
            deps_len: deps_len,
        };
        store.insertHashBucket(name_hash, id)?;
        i += 1;
    }
    store.record_count = rec_count;

    // Decode dependency edge table
    let e: usize = 0;
    while (e < edge_count) {
        const target = lsReadU32Le(data, &pos)?;
        if (@as(usize, target) >= rec_count) return err.InvalidRecordBounds;
        store.dep_edges[e] = target;
        e += 1;
    }
    store.edge_count = edge_count;

    // Copy string pool
    let s: usize = 0;
    while (s < str_bytes) {
        store.string_buf[s] = data[pos + s];
        s += 1;
    }
    store.string_used = str_bytes;

    return store;
}
