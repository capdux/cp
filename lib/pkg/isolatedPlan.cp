// Source: Absorbed from /data/dev/bun/src/install/isolated_install.cp | Synced: 2026-11-23T20:24:05Z
// ============================================================================
// lib/pkg/isolatedPlan.cp — Production-Grade pnpm-Style Isolated Install Graph
// Planner, Iterative Tarjan SCC Analyzer, Leaking-Peer Context Deduplicator &
// Deterministic Symlink Tree Generator
// ============================================================================
// Absorbed from Bun `src/install/isolated_install.cp`. Designed with strict
// external-slice-driven zero-allocation architecture (no hardcoded small
// package/edge/entry limits, no recursion stack overflow risk, full path
// traversal protection, cycle-safe iterative Tarjan SCC + iterative DFS).
//
// Pipeline phases:
//   1. Security & Path Hygiene (`validateIsoPkgName`, `validateIsoRelPath`,
//      `formatIsoStoreDirKey`, `formatIsoRelativeLinkTarget`)
//   2. Iterative Tarjan Strongly Connected Components (`computeIsoSccs`)
//   3. Fixed-point Leaking Peer-Dependency Closure (`computeLeakingPeerMasks`)
//   4. Peer-Context Deduplicating Store Entry Materializer (`IsoInstallPlanner`)
//   5. Deterministic Symlink Emission, Lifecycle State Machine & Plan Reporting

pub const ISO_INVALID_ID: u32 = 0xFFFFFFFF;
pub const ISO_ROOT_ENTRY: u32 = 0xFFFFFFFE;
pub const ISO_MAX_PKG_NAME_LEN: usize = 214;
pub const ISO_DEFAULT_STORE_DIR: []const u8 = ".bun";

pub err IsolatedPlanError {
    EmptyPackageName,
    UnsafePackageName,
    UnsafeRelativePath,
    InvalidPackageId,
    InvalidEntryId,
    InvalidEdgeSlice,
    ScratchTooSmall,
    StoreFull,
    SymlinkListFull,
    StackOverflow,
    InvalidStateTransition,
    BufferTooSmall,
}

// ============================================================================
// Internal Zero-Alloc String & Hashing Primitives
// ============================================================================

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn strCmp(a: []const u8, b: []const u8): i32 {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
        i += 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

fn fnv1a64(s: []const u8): u64 {
    let h: u64 = 14695981039346656037;
    let i: usize = 0;
    while (i < s.len) {
        h ^= @as(u64, s[i]);
        h = h *% 1099511628211;
        i += 1;
    }
    return h;
}

fn mixHash64(acc: u64, v: u64): u64 {
    let z = (acc ^ v) +% 0x9E3779B97F4A7C15;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn appendByte(out: []u8, pos: usize, ch: u8): IsolatedPlanError!usize {
    if (pos >= out.len) return IsolatedPlanError.BufferTooSmall;
    out[pos] = ch;
    return pos + 1;
}

fn appendSlice(out: []u8, pos: usize, src: []const u8): IsolatedPlanError!usize {
    if (src.len > out.len - pos) return IsolatedPlanError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn writeU32(out: []u8, pos: usize, val_in: u32): IsolatedPlanError!usize {
    let tmp: [12]u8 = [0; 12];
    let n: usize = 0;
    let v = val_in;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = @as(u8, @intCast('0' + (v % 10)));
            v /= 10;
            n += 1;
        }
    }
    if (n > out.len - pos) return IsolatedPlanError.BufferTooSmall;
    let i: usize = 0;
    while (i < n) {
        out[pos + i] = tmp[n - 1 - i];
        i += 1;
    }
    return pos + n;
}

fn writeHex64(out: []u8, pos: usize, val: u64): IsolatedPlanError!usize {
    if (16 > out.len - pos) return IsolatedPlanError.BufferTooSmall;
    const hex_chars = "0123456789abcdef";
    let i: usize = 0;
    while (i < 16) {
        const shift: u6 = @as(u6, @intCast((15 - i) * 4));
        const nibble: usize = @as(usize, @truncate((val >> shift) & 0xF));
        out[pos + i] = hex_chars[nibble];
        i += 1;
    }
    return pos + 16;
}

// ============================================================================
// Phase 0: Package Name & Path Traversal Security Guards
// ============================================================================

fn isSafeSegmentChar(c: u8): bool {
    if (c >= 'a' && c <= 'z') return true;
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= '0' && c <= '9') return true;
    return c == '-' || c == '_' || c == '.' || c == '~';
}

fn isValidPkgSegment(seg: []const u8): bool {
    if (seg.len == 0) return false;
    if (strEql(seg, ".") || strEql(seg, "..")) return false;
    if (seg[0] == '.') return false;
    let i: usize = 0;
    while (i < seg.len) {
        if (!isSafeSegmentChar(seg[i])) return false;
        i += 1;
    }
    return true;
}

/// Validates an npm/cap package name against path traversal, null bytes, backslashes,
/// hidden segments, and malformed scope syntax (`@scope/pkg` or `pkg`).
pub fn validateIsoPkgName(name: []const u8): IsolatedPlanError!void {
    if (name.len == 0) return IsolatedPlanError.EmptyPackageName;
    if (name.len > ISO_MAX_PKG_NAME_LEN) return IsolatedPlanError.UnsafePackageName;

    if (name[0] == '@') {
        let slash: usize = name.len;
        let i: usize = 1;
        while (i < name.len) {
            if (name[i] == '/') {
                if (slash != name.len) return IsolatedPlanError.UnsafePackageName;
                slash = i;
            }
            i += 1;
        }
        if (slash <= 1 || slash + 1 >= name.len) {
            return IsolatedPlanError.UnsafePackageName;
        }
        const scope = name[1..slash];
        const pkg = name[(slash + 1)..];
        if (!isValidPkgSegment(scope) || !isValidPkgSegment(pkg)) {
            return IsolatedPlanError.UnsafePackageName;
        }
        return;
    }

    if (!isValidPkgSegment(name)) {
        return IsolatedPlanError.UnsafePackageName;
    }
}

/// Validates a relative workspace path, rejecting absolute paths, backslashes,
/// null bytes, and `..` parent-directory escape segments.
pub fn validateIsoRelPath(rel_path: []const u8): IsolatedPlanError!void {
    if (rel_path.len == 0) return IsolatedPlanError.UnsafeRelativePath;
    if (rel_path[0] == '/' || rel_path[0] == '\\') return IsolatedPlanError.UnsafeRelativePath;

    let i: usize = 0;
    while (i < rel_path.len) {
        const seg_start = i;
        while (i < rel_path.len && rel_path[i] != '/') {
            const c = rel_path[i];
            if (c == 0 || c == '\\' || c < 0x20) return IsolatedPlanError.UnsafeRelativePath;
            i += 1;
        }
        const seg = rel_path[seg_start..i];
        if (seg.len == 0 || strEql(seg, ".") || strEql(seg, "..")) {
            return IsolatedPlanError.UnsafeRelativePath;
        }
        if (i < rel_path.len && rel_path[i] == '/') {
            i += 1;
            if (i == rel_path.len) return IsolatedPlanError.UnsafeRelativePath;
        }
    }
}

/// Formats a canonical isolated store directory key into `out`:
/// - Unscoped: `<name>@<version>` (or `<name>@<version>_<16-hex-peer-hash>` when `peer_ctx_hash != 0`)
/// - Scoped: `@scope+pkg@<version>[_<16-hex-peer-hash>]`
pub fn formatIsoStoreDirKey(
    pkg_name: []const u8,
    version: []const u8,
    peer_ctx_hash: u64,
    out: []u8,
): IsolatedPlanError!usize {
    validateIsoPkgName(pkg_name)?;
    if (version.len == 0) return IsolatedPlanError.UnsafePackageName;

    let pos: usize = 0;
    let i: usize = 0;
    while (i < pkg_name.len) {
        const c = pkg_name[i];
        pos = appendByte(out, pos, if (c == '/') '+' else c)?;
        i += 1;
    }
    pos = appendByte(out, pos, '@')?;
    i = 0;
    while (i < version.len) {
        const c = version[i];
        if (c == 0 || c == '/' || c == '\\' || c < 0x20) {
            return IsolatedPlanError.UnsafePackageName;
        }
        pos = appendByte(out, pos, c)?;
        i += 1;
    }
    if (peer_ctx_hash != 0) {
        pos = appendByte(out, pos, '_')?;
        pos = writeHex64(out, pos, peer_ctx_hash)?;
    }
    return pos;
}

/// Computes a POSIX relative symlink target from `from_dir` to `to_path` into `out`
/// with zero dynamic allocation (e.g. `node_modules/.bun/a@1/node_modules/@s` ->
/// `../../../b@2/node_modules/b`).
pub fn formatIsoRelativeLinkTarget(
    from_dir: []const u8,
    to_path: []const u8,
    out: []u8,
): IsolatedPlanError!usize {
    validateIsoRelPath(from_dir)?;
    validateIsoRelPath(to_path)?;

    // Walk common prefix segment-by-segment
    let fi: usize = 0;
    let ti: usize = 0;
    while (fi < from_dir.len && ti < to_path.len) {
        let f_end = fi;
        while (f_end < from_dir.len && from_dir[f_end] != '/') f_end += 1;
        let t_end = ti;
        while (t_end < to_path.len && to_path[t_end] != '/') t_end += 1;

        if (!strEql(from_dir[fi..f_end], to_path[ti..t_end])) {
            break;
        }
        fi = if (f_end < from_dir.len) (f_end + 1) else f_end;
        ti = if (t_end < to_path.len) (t_end + 1) else t_end;
    }

    // Count remaining segments in `from_dir` to emit `../`
    let up_count: usize = 0;
    let k = fi;
    while (k < from_dir.len) {
        while (k < from_dir.len && from_dir[k] != '/') k += 1;
        up_count += 1;
        if (k < from_dir.len && from_dir[k] == '/') k += 1;
    }

    let pos: usize = 0;
    let u: usize = 0;
    while (u < up_count) {
        if (u > 0) pos = appendByte(out, pos, '/')?;
        pos = appendSlice(out, pos, "..")?;
        u += 1;
    }

    if (ti < to_path.len) {
        if (pos > 0) pos = appendByte(out, pos, '/')?;
        pos = appendSlice(out, pos, to_path[ti..])?;
    } else if (pos == 0) {
        pos = appendByte(out, pos, '.')?;
    }
    return pos;
}

// ============================================================================
// Core Graph & Store Data Types (External-Slice-Driven)
// ============================================================================

/// Dependency edge kind in the lockfile graph.
pub enum IsoDepKind: u8 {
    normal = 0,
    peer = 1,
    optional_peer = 2,
    workspace = 3,
    optional = 4,

    fn isPeer(): bool {
        return _ == IsoDepKind.peer || _ == IsoDepKind.optional_peer;
    }
}

/// Lifecycle state of an isolated store entry (mirrors Bun `InstallState_`).
pub enum IsoInstallState: u8 {
    pending = 0,
    extracting = 1,
    extracted = 2,
    linking = 3,
    linked = 4,
    failed = 5,

    fn asStr(): []const u8 {
        if (_ == IsoInstallState.pending) return "pending";
        if (_ == IsoInstallState.extracting) return "extracting";
        if (_ == IsoInstallState.extracted) return "extracted";
        if (_ == IsoInstallState.linking) return "linking";
        if (_ == IsoInstallState.linked) return "linked";
        return "failed";
    }
}

/// Input package node in the resolved lockfile graph.
pub struct IsoPkgInput {
    name: []const u8 = "",
    version: []const u8 = "0.0.0",
    is_workspace: bool = false,
    workspace_rel_path: []const u8 = "",
    dep_start: u32 = 0,
    dep_len: u32 = 0,

    static fn empty(): IsoPkgInput {
        return IsoPkgInput{};
    }
}

/// Directed dependency edge from a package to a resolved target package.
pub struct IsoDepEdge {
    dep_name: []const u8 = "",
    target_pkg_id: u32 = ISO_INVALID_ID,
    kind: IsoDepKind = IsoDepKind.normal,
    is_aliased: bool = false,

    static fn normal(dep_name: []const u8, target_pkg_id: u32): IsoDepEdge {
        return IsoDepEdge{
            dep_name: dep_name,
            target_pkg_id: target_pkg_id,
            kind: IsoDepKind.normal,
            is_aliased: false,
        };
    }

    static fn peer(dep_name: []const u8, target_pkg_id: u32): IsoDepEdge {
        return IsoDepEdge{
            dep_name: dep_name,
            target_pkg_id: target_pkg_id,
            kind: IsoDepKind.peer,
            is_aliased: false,
        };
    }

    static fn workspace(dep_name: []const u8, target_pkg_id: u32): IsoDepEdge {
        return IsoDepEdge{
            dep_name: dep_name,
            target_pkg_id: target_pkg_id,
            kind: IsoDepKind.workspace,
            is_aliased: false,
        };
    }
}

// ============================================================================
// Phase 1: Iterative Tarjan Strongly Connected Components (Zero Recursion)
// ============================================================================

/// Caller-provided scratch slices for iterative Tarjan SCC (each slice >= `pkgs.len`).
pub struct IsoSccScratch {
    disc: []u32,
    low: []u32,
    on_stack: []bool,
    tarjan_stack: []u32,
    work_pkg: []u32,
    work_edge: []u32,
    scc_id: []u32,
}

/// Summary metrics returned by `computeIsoSccs`.
pub struct IsoSccResult {
    scc_count: u32 = 0,
    cyclic_scc_count: u32 = 0,
    max_scc_size: u32 = 0,
    has_cycles: bool = false,
}

/// Computes Strongly Connected Components over `(pkgs, edges)` using iterative
/// Tarjan's algorithm (zero recursion, zero heap allocation).
/// Populates `scratch.scc_id[pkg_id]` for every package in `0..pkgs.len`.
pub fn computeIsoSccs(
    pkgs: []const IsoPkgInput,
    edges: []const IsoDepEdge,
    scratch: IsoSccScratch,
): IsolatedPlanError!IsoSccResult {
    const n = pkgs.len;
    let disc = scratch.disc;
    let low = scratch.low;
    let on_stack = scratch.on_stack;
    let tarjan_stack = scratch.tarjan_stack;
    let work_pkg = scratch.work_pkg;
    let work_edge = scratch.work_edge;
    let scc_id = scratch.scc_id;

    if (disc.len < n || low.len < n || on_stack.len < n
        || tarjan_stack.len < n || work_pkg.len < n
        || work_edge.len < n || scc_id.len < n)
    {
        return IsolatedPlanError.ScratchTooSmall;
    }

    let i: usize = 0;
    while (i < n) {
        const p = pkgs[i];
        const ds: usize = @as(usize, p.dep_start);
        const dl: usize = @as(usize, p.dep_len);
        if (ds > edges.len || dl > edges.len - ds) {
            return IsolatedPlanError.InvalidEdgeSlice;
        }
        disc[i] = ISO_INVALID_ID;
        low[i] = ISO_INVALID_ID;
        on_stack[i] = false;
        scc_id[i] = ISO_INVALID_ID;
        i += 1;
    }

    let timer: u32 = 0;
    let t_top: usize = 0;
    let w_top: usize = 0;
    let res = IsoSccResult{};

    let start: usize = 0;
    while (start < n) {
        if (disc[start] != ISO_INVALID_ID) {
            start += 1;
            continue;
        }

        disc[start] = timer;
        low[start] = timer;
        timer += 1;
        tarjan_stack[t_top] = @as(u32, @intCast(start));
        t_top += 1;
        on_stack[start] = true;

        work_pkg[0] = @as(u32, @intCast(start));
        work_edge[0] = 0;
        w_top = 1;

        while (w_top > 0) {
            const top = w_top - 1;
            const u: usize = @as(usize, work_pkg[top]);
            const ei: u32 = work_edge[top];
            const pkg = pkgs[u];

            if (ei < pkg.dep_len) {
                work_edge[top] = ei + 1;
                const edge = edges[@as(usize, pkg.dep_start + ei)];
                if (edge.target_pkg_id == ISO_INVALID_ID) {
                    continue;
                }
                const v: usize = @as(usize, edge.target_pkg_id);
                if (v >= n) return IsolatedPlanError.InvalidPackageId;

                if (disc[v] == ISO_INVALID_ID) {
                    disc[v] = timer;
                    low[v] = timer;
                    timer += 1;
                    tarjan_stack[t_top] = @as(u32, @intCast(v));
                    t_top += 1;
                    on_stack[v] = true;

                    if (w_top >= work_pkg.len) {
                        return IsolatedPlanError.StackOverflow;
                    }
                    work_pkg[w_top] = @as(u32, @intCast(v));
                    work_edge[w_top] = 0;
                    w_top += 1;
                } else if (on_stack[v]) {
                    if (disc[v] < low[u]) {
                        low[u] = disc[v];
                    }
                }
            } else {
                // Post-order: pop work frame
                w_top -= 1;
                if (low[u] == disc[u]) {
                    const comp_id = res.scc_count;
                    res.scc_count += 1;
                    let comp_size: u32 = 0;
                    let has_self_loop = false;

                    while (t_top > 0) {
                        t_top -= 1;
                        const w: usize = @as(usize, tarjan_stack[t_top]);
                        on_stack[w] = false;
                        scc_id[w] = comp_id;
                        comp_size += 1;
                        if (w == u) break;
                    }

                    // Check single-node self-loop
                    if (comp_size == 1) {
                        let k: u32 = 0;
                        while (k < pkg.dep_len) {
                            if (edges[@as(usize, pkg.dep_start + k)].target_pkg_id == @as(u32, @intCast(u))) {
                                has_self_loop = true;
                                break;
                            }
                            k += 1;
                        }
                    }

                    if (comp_size > res.max_scc_size) {
                        res.max_scc_size = comp_size;
                    }
                    if (comp_size > 1 || has_self_loop) {
                        res.cyclic_scc_count += 1;
                        res.has_cycles = true;
                    }
                }

                // Propagate low-link to parent on work stack
                if (w_top > 0) {
                    const parent: usize = @as(usize, work_pkg[w_top - 1]);
                    if (low[u] < low[parent]) {
                        low[parent] = low[u];
                    }
                }
            }
        }
        start += 1;
    }

    return res;
}

// ============================================================================
// Phase 2: Leaking Peer-Dependency Closure (Fixed-Point Bitset Propagation)
// ============================================================================

/// Computes a 64-bit leaking-peer fingerprint mask for each package:
/// For every peer dependency name declared anywhere in `pkg`'s transitive closure
/// that is not satisfied by a direct non-peer dependency of `pkg`, sets its bit
/// `(fnv1a64(peer_name) & 63)` in `leaking_masks[pkg]`.
/// Mirrors Bun's `leaking_peers` fixed-point pass in `isolated_install.cp`.
pub fn computeLeakingPeerMasks(
    pkgs: []const IsoPkgInput,
    edges: []const IsoDepEdge,
    leaking_masks: []u64,
    own_peer_masks: []u64,
    own_non_peer_masks: []u64,
): IsolatedPlanError!void {
    const n = pkgs.len;
    if (leaking_masks.len < n || own_peer_masks.len < n || own_non_peer_masks.len < n) {
        return IsolatedPlanError.ScratchTooSmall;
    }

    let i: usize = 0;
    while (i < n) {
        const p = pkgs[i];
        const ds: usize = @as(usize, p.dep_start);
        const dl: usize = @as(usize, p.dep_len);
        if (ds > edges.len || dl > edges.len - ds) {
            return IsolatedPlanError.InvalidEdgeSlice;
        }
        let pm: u64 = 0;
        let npm: u64 = 0;
        let k: u32 = 0;
        while (k < p.dep_len) {
            const e = edges[@as(usize, p.dep_start + k)];
            const bit: u6 = @as(u6, @truncate(fnv1a64(e.dep_name) & 63));
            const mask: u64 = @as(u64, 1) << bit;
            if (e.kind.isPeer()) {
                pm |= mask;
            } else {
                npm |= mask;
            }
            k += 1;
        }
        own_peer_masks[i] = pm;
        own_non_peer_masks[i] = npm;
        leaking_masks[i] = pm;
        i += 1;
    }

    let changed = true;
    while (changed) {
        changed = false;
        let u: usize = 0;
        while (u < n) {
            const p = pkgs[u];
            let acc: u64 = own_peer_masks[u];
            let k: u32 = 0;
            while (k < p.dep_len) {
                const e = edges[@as(usize, p.dep_start + k)];
                if (e.target_pkg_id != ISO_INVALID_ID) {
                    const v: usize = @as(usize, e.target_pkg_id);
                    if (v >= n) return IsolatedPlanError.InvalidPackageId;
                    acc |= leaking_masks[v];
                }
                k += 1;
            }
            // Subtract peers satisfied internally by `u`'s own non-peer deps
            acc = (acc & ~own_non_peer_masks[u]) | own_peer_masks[u];
            if (acc != leaking_masks[u]) {
                leaking_masks[u] = acc;
                changed = true;
            }
            u += 1;
        }
    }
}

// ============================================================================
// Phase 3: Isolated Store Entry Materialization & Symlink Tree Planner
// ============================================================================

/// Materialized entry inside `node_modules/.bun/<store_key>/node_modules/<pkg>`.
pub struct IsoStoreEntry {
    entry_id: u32 = 0,
    pkg_id: u32 = 0,
    peer_ctx_hash: u64 = 0,
    scc_id: u32 = ISO_INVALID_ID,
    is_workspace: bool = false,
    state: IsoInstallState = IsoInstallState.pending,
    ref_count: u32 = 0,

    static fn empty(): IsoStoreEntry {
        return IsoStoreEntry{};
    }
}

/// Planned symlink operation inside the isolated `node_modules` tree.
pub struct IsoSymlinkOp {
    /// `ISO_ROOT_ENTRY` for top-level `node_modules/<dep_name>`, or a store `entry_id`.
    from_entry_id: u32 = ISO_ROOT_ENTRY,
    dep_name: []const u8 = "",
    target_entry_id: u32 = ISO_INVALID_ID,
    is_direct_root: bool = false,
    is_peer_link: bool = false,
    is_workspace_link: bool = false,

    static fn empty(): IsoSymlinkOp {
        return IsoSymlinkOp{};
    }
}

/// Summary counters of a built isolated install plan.
pub struct IsoPlanSummary {
    entry_count: usize = 0,
    deduped_hits: usize = 0,
    symlink_count: usize = 0,
    root_link_count: usize = 0,
    peer_link_count: usize = 0,
    workspace_link_count: usize = 0,
    cyclic_scc_count: u32 = 0,
    max_depth: usize = 0,
}

/// Scratch DFS frame for cycle-safe iterative isolated store expansion.
pub struct IsoDfsFrame {
    entry_id: u32 = ISO_INVALID_ID,
    pkg_id: u32 = ISO_INVALID_ID,
    next_edge: u32 = 0,

    static fn empty(): IsoDfsFrame {
        return IsoDfsFrame{};
    }
}

fn symlinkOpLess(a: IsoSymlinkOp, b: IsoSymlinkOp): bool {
    if (a.from_entry_id != b.from_entry_id) {
        // Place ISO_ROOT_ENTRY first for deterministic readability
        if (a.from_entry_id == ISO_ROOT_ENTRY) return true;
        if (b.from_entry_id == ISO_ROOT_ENTRY) return false;
        return a.from_entry_id < b.from_entry_id;
    }
    const cmp = strCmp(a.dep_name, b.dep_name);
    if (cmp != 0) return cmp < 0;
    return a.target_entry_id < b.target_entry_id;
}

/// External-slice-driven pnpm-style Isolated Install Graph Planner.
pub struct IsoInstallPlanner {
    entries: []IsoStoreEntry,
    symlinks: []IsoSymlinkOp,
    entry_len: usize = 0,
    symlink_len: usize = 0,
    summary: IsoPlanSummary = IsoPlanSummary{},

    static fn init(entries: []IsoStoreEntry, symlinks: []IsoSymlinkOp): IsoInstallPlanner {
        return IsoInstallPlanner{
            entries: entries,
            symlinks: symlinks,
            entry_len: 0,
            symlink_len: 0,
            summary: IsoPlanSummary{},
        };
    }

    fn entryCount(): usize {
        return _.entry_len;
    }

    fn symlinkCount(): usize {
        return _.symlink_len;
    }

    fn getEntry(entry_id: u32): IsolatedPlanError!IsoStoreEntry {
        const idx: usize = @as(usize, entry_id);
        if (idx >= _.entry_len) return IsolatedPlanError.InvalidEntryId;
        return _.entries[idx];
    }

    fn findDedupedEntry(pkg_id: u32, peer_ctx_hash: u64): u32? {
        let i: usize = 0;
        while (i < _.entry_len) {
            if (_.entries[i].pkg_id == pkg_id && _.entries[i].peer_ctx_hash == peer_ctx_hash) {
                return _.entries[i].entry_id;
            }
            i += 1;
        }
        return null;
    }

    fn hasSymlink(from_entry_id: u32, dep_name: []const u8): bool {
        let i: usize = 0;
        while (i < _.symlink_len) {
            const s = _.symlinks[i];
            if (s.from_entry_id == from_entry_id && strEql(s.dep_name, dep_name)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn addSymlink(*_, op: IsoSymlinkOp): IsolatedPlanError!void {
        if (_.hasSymlink(op.from_entry_id, op.dep_name)) return;
        if (_.symlink_len >= _.symlinks.len) return IsolatedPlanError.SymlinkListFull;
        _.symlinks[_.symlink_len] = op;
        _.symlink_len += 1;
        if (op.is_direct_root) _.summary.root_link_count += 1;
        if (op.is_peer_link) _.summary.peer_link_count += 1;
        if (op.is_workspace_link) _.summary.workspace_link_count += 1;
    }

    /// Resolves a peer dependency name by walking up the active DFS ancestor chain,
    /// mirroring Bun's ancestor peer-resolution walk in `isolated_install.cp`.
    fn resolvePeerInAncestors(
        pkgs: []const IsoPkgInput,
        edges: []const IsoDepEdge,
        dfs_stack: []const IsoDfsFrame,
        dfs_len: usize,
        peer_name: []const u8,
    ): u32? {
        let depth = dfs_len;
        while (depth > 0) {
            depth -= 1;
            const anc_pkg_id = dfs_stack[depth].pkg_id;
            const anc = pkgs[@as(usize, anc_pkg_id)];
            let k: u32 = 0;
            while (k < anc.dep_len) {
                const e = edges[@as(usize, anc.dep_start + k)];
                if (!e.kind.isPeer() && e.target_pkg_id != ISO_INVALID_ID && strEql(e.dep_name, peer_name)) {
                    return e.target_pkg_id;
                }
                k += 1;
            }
        }
        return null;
    }

    /// Computes a deterministic 64-bit peer-context hash for `pkg_id` under the current
    /// ancestor chain. Returns `0` when the package has no leaking peers or is a workspace.
    fn computePeerCtxHash(
        pkgs: []const IsoPkgInput,
        edges: []const IsoDepEdge,
        leaking_masks: []const u64,
        dfs_stack: []const IsoDfsFrame,
        dfs_len: usize,
        pkg_id: u32,
    ): u64 {
        const p = pkgs[@as(usize, pkg_id)];
        if (p.is_workspace) return 0;
        const leak_mask = leaking_masks[@as(usize, pkg_id)];
        if (leak_mask == 0) return 0;

        let h: u64 = 0;
        // Hash direct peer resolutions from ancestor chain
        let k: u32 = 0;
        while (k < p.dep_len) {
            const e = edges[@as(usize, p.dep_start + k)];
            if (e.kind.isPeer()) {
                const resolved = _.resolvePeerInAncestors(pkgs, edges, dfs_stack, dfs_len, e.dep_name)
                    ?? e.target_pkg_id;
                if (resolved != ISO_INVALID_ID) {
                    const entry_h = mixHash64(fnv1a64(e.dep_name), @as(u64, resolved) + 1);
                    h ^= entry_h;
                }
            }
            k += 1;
        }

        // Also hash ancestor-provided resolutions matching any bit in `leak_mask`
        // for transitively leaking peers
        let d = dfs_len;
        while (d > 0) {
            d -= 1;
            const anc = pkgs[@as(usize, dfs_stack[d].pkg_id)];
            let j: u32 = 0;
            while (j < anc.dep_len) {
                const ae = edges[@as(usize, anc.dep_start + j)];
                if (!ae.kind.isPeer() && ae.target_pkg_id != ISO_INVALID_ID) {
                    const bit: u6 = @as(u6, @truncate(fnv1a64(ae.dep_name) & 63));
                    if ((leak_mask & (@as(u64, 1) << bit)) != 0) {
                        const pair_h = mixHash64(fnv1a64(ae.dep_name), @as(u64, ae.target_pkg_id) + 1);
                        h = mixHash64(h, pair_h);
                    }
                }
                j += 1;
            }
        }
        return h;
    }

    fn sortSymlinksDeterministic(*_) {
        let i: usize = 1;
        while (i < _.symlink_len) {
            const key = _.symlinks[i];
            let j = i;
            while (j > 0 && symlinkOpLess(key, _.symlinks[j - 1])) {
                _.symlinks[j] = _.symlinks[j - 1];
                j -= 1;
            }
            _.symlinks[j] = key;
            i += 1;
        }
    }

    /// Executes the full isolated install planning pipeline starting at `root_pkg_id`:
    /// 1. Validates all package names, workspace relative paths, and edge ranges
    /// 2. Runs iterative Tarjan SCC to classify cyclic dependency groups
    /// 3. Computes leaking peer-dependency closure masks
    /// 4. Expands the isolated store graph via iterative DFS with `(pkg_id, peer_ctx_hash)` dedupe
    /// 5. Sorts emitted symlinks deterministically
    fn build(
        *_,
        pkgs: []const IsoPkgInput,
        edges: []const IsoDepEdge,
        root_pkg_id: u32,
        scc_scratch: IsoSccScratch,
        leaking_masks: []u64,
        own_peer_masks: []u64,
        own_non_peer_masks: []u64,
        dfs_stack: []IsoDfsFrame,
    ): IsolatedPlanError!IsoPlanSummary {
        if (pkgs.len == 0 || @as(usize, root_pkg_id) >= pkgs.len) {
            return IsolatedPlanError.InvalidPackageId;
        }
        if (dfs_stack.len < 2) return IsolatedPlanError.ScratchTooSmall;

        // Validate package names & workspace relative paths upfront
        let i: usize = 0;
        while (i < pkgs.len) {
            const p = pkgs[i];
            validateIsoPkgName(p.name)?;
            if (p.is_workspace && p.workspace_rel_path.len > 0) {
                validateIsoRelPath(p.workspace_rel_path)?;
            }
            const ds: usize = @as(usize, p.dep_start);
            const dl: usize = @as(usize, p.dep_len);
            if (ds > edges.len || dl > edges.len - ds) {
                return IsolatedPlanError.InvalidEdgeSlice;
            }
            let k: u32 = 0;
            while (k < p.dep_len) {
                const e = edges[@as(usize, p.dep_start + k)];
                validateIsoPkgName(e.dep_name)?;
                if (e.target_pkg_id != ISO_INVALID_ID && @as(usize, e.target_pkg_id) >= pkgs.len) {
                    return IsolatedPlanError.InvalidPackageId;
                }
                k += 1;
            }
            i += 1;
        }

        const scc_res = computeIsoSccs(pkgs, edges, scc_scratch)?;
        computeLeakingPeerMasks(pkgs, edges, leaking_masks, own_peer_masks, own_non_peer_masks)?;

        _.entry_len = 0;
        _.symlink_len = 0;
        _.summary = IsoPlanSummary{
            cyclic_scc_count: scc_res.cyclic_scc_count,
        };

        // Seed DFS with root package (ISO_ROOT_ENTRY does not materialize into .bun store)
        dfs_stack[0] = IsoDfsFrame{
            entry_id: ISO_ROOT_ENTRY,
            pkg_id: root_pkg_id,
            next_edge: 0,
        };
        let dfs_len: usize = 1;

        while (dfs_len > 0) {
            if (dfs_len > _.summary.max_depth) {
                _.summary.max_depth = dfs_len;
            }
            const top = dfs_len - 1;
            const cur_entry_id = dfs_stack[top].entry_id;
            const cur_pkg_id = dfs_stack[top].pkg_id;
            const ei = dfs_stack[top].next_edge;
            const cur_pkg = pkgs[@as(usize, cur_pkg_id)];

            if (ei >= cur_pkg.dep_len) {
                dfs_len -= 1;
                continue;
            }
            let frame = dfs_stack[top];
            frame.next_edge = ei + 1;
            dfs_stack[top] = frame;

            const edge = edges[@as(usize, cur_pkg.dep_start + ei)];
            let target_pkg_id = edge.target_pkg_id;
            if (edge.kind.isPeer()) {
                if (resolved := _.resolvePeerInAncestors(pkgs, edges, dfs_stack[0..dfs_len], dfs_len, edge.dep_name)) {
                    target_pkg_id = resolved;
                }
            }
            if (target_pkg_id == ISO_INVALID_ID) {
                continue;
            }

            // Guard against direct cycle back to an active DFS ancestor with same pkg_id
            let ancestor_entry_hit: u32 = ISO_INVALID_ID;
            let a_idx: usize = 0;
            while (a_idx < dfs_len) {
                if (dfs_stack[a_idx].pkg_id == target_pkg_id && dfs_stack[a_idx].entry_id != ISO_ROOT_ENTRY) {
                    ancestor_entry_hit = dfs_stack[a_idx].entry_id;
                    break;
                }
                a_idx += 1;
            }

            let target_entry_id: u32 = ISO_INVALID_ID;
            let need_expand = false;

            if (ancestor_entry_hit != ISO_INVALID_ID) {
                target_entry_id = ancestor_entry_hit;
                _.entries[@as(usize, target_entry_id)].ref_count += 1;
                _.summary.deduped_hits += 1;
            } else {
                const peer_hash = _.computePeerCtxHash(
                    pkgs,
                    edges,
                    leaking_masks,
                    dfs_stack[0..dfs_len],
                    dfs_len,
                    target_pkg_id,
                );
                if (existing_id := _.findDedupedEntry(target_pkg_id, peer_hash)) {
                    target_entry_id = existing_id;
                    _.entries[@as(usize, target_entry_id)].ref_count += 1;
                    _.summary.deduped_hits += 1;
                } else {
                    if (_.entry_len >= _.entries.len) return IsolatedPlanError.StoreFull;
                    target_entry_id = @as(u32, @intCast(_.entry_len));
                    const tp = pkgs[@as(usize, target_pkg_id)];
                    _.entries[_.entry_len] = IsoStoreEntry{
                        entry_id: target_entry_id,
                        pkg_id: target_pkg_id,
                        peer_ctx_hash: peer_hash,
                        scc_id: scc_scratch.scc_id[@as(usize, target_pkg_id)],
                        is_workspace: tp.is_workspace,
                        state: IsoInstallState.pending,
                        ref_count: 1,
                    };
                    _.entry_len += 1;
                    need_expand = true;
                }
            }

            const target_pkg = pkgs[@as(usize, target_pkg_id)];
            _.addSymlink(IsoSymlinkOp{
                from_entry_id: cur_entry_id,
                dep_name: edge.dep_name,
                target_entry_id: target_entry_id,
                is_direct_root: cur_entry_id == ISO_ROOT_ENTRY,
                is_peer_link: edge.kind.isPeer(),
                is_workspace_link: target_pkg.is_workspace || edge.kind == IsoDepKind.workspace,
            })?;

            if (need_expand) {
                if (dfs_len >= dfs_stack.len) return IsolatedPlanError.StackOverflow;
                dfs_stack[dfs_len] = IsoDfsFrame{
                    entry_id: target_entry_id,
                    pkg_id: target_pkg_id,
                    next_edge: 0,
                };
                dfs_len += 1;
            }
        }

        _.sortSymlinksDeterministic();
        _.summary.entry_count = _.entry_len;
        _.summary.symlink_count = _.symlink_len;
        return _.summary;
    }

    /// Validates and applies a materialization lifecycle transition on a store entry.
    /// Legal transitions:
    ///   pending -> extracting | failed
    ///   extracting -> extracted | failed
    ///   extracted -> linking | failed
    ///   linking -> linked | failed
    fn transitionEntryState(*_, entry_id: u32, next_state: IsoInstallState): IsolatedPlanError!void {
        const idx: usize = @as(usize, entry_id);
        if (idx >= _.entry_len) return IsolatedPlanError.InvalidEntryId;
        const cur = _.entries[idx].state;
        const ok = (next_state == IsoInstallState.failed && cur != IsoInstallState.linked)
            || (cur == IsoInstallState.pending && next_state == IsoInstallState.extracting)
            || (cur == IsoInstallState.extracting && next_state == IsoInstallState.extracted)
            || (cur == IsoInstallState.extracted && next_state == IsoInstallState.linking)
            || (cur == IsoInstallState.linking && next_state == IsoInstallState.linked);
        if (!ok) return IsolatedPlanError.InvalidStateTransition;
        _.entries[idx].state = next_state;
    }

    /// Formats the full store package path for `entry_id` into `out`:
    /// `node_modules/.bun/<store_key>/node_modules/<pkg_name>` (or workspace rel path).
    fn formatEntryStorePath(
        pkgs: []const IsoPkgInput,
        entry_id: u32,
        store_dir_name: []const u8,
        out: []u8,
    ): IsolatedPlanError!usize {
        const entry = _.getEntry(entry_id)?;
        const pkg = pkgs[@as(usize, entry.pkg_id)];
        if (entry.is_workspace && pkg.workspace_rel_path.len > 0) {
            return appendSlice(out, 0, pkg.workspace_rel_path);
        }
        let pos = appendSlice(out, 0, "node_modules/")?;
        pos = appendSlice(out, pos, store_dir_name)?;
        pos = appendByte(out, pos, '/')?;
        const key_len = formatIsoStoreDirKey(pkg.name, pkg.version, entry.peer_ctx_hash, out[pos..])?;
        pos += key_len;
        pos = appendSlice(out, pos, "/node_modules/")?;
        return appendSlice(out, pos, pkg.name);
    }

    /// Formats a deterministic human-readable report of all planned symlinks into `out`.
    fn formatPlanReport(pkgs: []const IsoPkgInput, out: []u8): IsolatedPlanError!usize {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.symlink_len) {
            const op = _.symlinks[i];
            if (op.from_entry_id == ISO_ROOT_ENTRY) {
                pos = appendSlice(out, pos, "root/")?;
            } else {
                pos = appendSlice(out, pos, "entry#")?;
                pos = writeU32(out, pos, op.from_entry_id)?;
                pos = appendByte(out, pos, '/')?;
            }
            pos = appendSlice(out, pos, op.dep_name)?;
            pos = appendSlice(out, pos, " -> entry#")?;
            pos = writeU32(out, pos, op.target_entry_id)?;
            const target_entry = _.entries[@as(usize, op.target_entry_id)];
            const target_pkg = pkgs[@as(usize, target_entry.pkg_id)];
            pos = appendSlice(out, pos, " (")?;
            pos = appendSlice(out, pos, target_pkg.name)?;
            pos = appendByte(out, pos, '@')?;
            pos = appendSlice(out, pos, target_pkg.version)?;
            if (op.is_peer_link) {
                pos = appendSlice(out, pos, ",peer")?;
            }
            if (op.is_workspace_link) {
                pos = appendSlice(out, pos, ",ws")?;
            }
            pos = appendSlice(out, pos, ")\n")?;
            i += 1;
        }
        return pos;
    }
}
