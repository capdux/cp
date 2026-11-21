// Source: /data/dev/bun/src/resolver/dir_info.cp
// ============================================================================
// lib/resolver/dir_cache.cp — Directory Metadata Cache & Symlink Realpath Resolver
// ============================================================================

const std = @import("std");
import { DirEntryKind, DirIterator } from "../sys/dir";
import { linuxSyscallErrno, EINTR } from "../sys/error";
import { PosixPath, normalizePosix, isAbsolutePosix } from "cap:path";
import { StatHash } from "./resolver";

fn sysLstatExistsAndIsSymlink(path: []const u8): !bool {
    let path_buf: [4096]u8 = undefined;
    if (path.len >= path_buf.len) return error.NameTooLong;
    @memcpy(path_buf[0..path.len], path);
    path_buf[path.len] = 0;
    const path_z: [*:0]const u8 = path_buf[0..path.len :0].ptr;
    let is_sym = false;
    let ok = false;
    zig {
        var stx: std.os.linux.Statx = undefined;
        const mask = std.os.linux.STATX{ .TYPE = true, .MODE = true };
        const rc = std.c.statx(std.os.linux.AT.FDCWD, path_z, std.os.linux.AT.SYMLINK_NOFOLLOW, mask, &stx);
        if (rc == 0) {
            is_sym = (@as(u32, stx.mode) & 0o170000) == 0o120000;
            ok = true;
        }
    }
    if (!ok) return error.NotFound;
    return is_sym;
}

fn sysReadlink(path: []const u8, buf: []u8): ![]const u8 {
    if (buf.len == 0) return error.BufferTooSmall;
    let path_buf: [4096]u8 = undefined;
    if (path.len >= path_buf.len) return error.NameTooLong;
    @memcpy(path_buf[0..path.len], path);
    path_buf[path.len] = 0;
    const path_z: [*:0]const u8 = path_buf[0..path.len :0].ptr;
    while (true) {
        const rc = std.os.linux.readlinkat(std.os.linux.AT.FDCWD, path_z, buf.ptr, buf.len);
        const err_code = linuxSyscallErrno(rc);
        if (err_code == 0) return buf[0..rc];
        if (err_code == EINTR) continue;
        return error.IoError;
    }
}

fn sysGetcwd(buf: []u8): ![]const u8 {
    if (buf.len == 0) return error.BufferTooSmall;
    const ptr = std.c.getcwd(buf.ptr, buf.len);
    if (ptr == null) return error.IoError;
    const c_ptr: [*:0]const u8 = unsafe { @ptrCast(buf.ptr) };
    return std.mem.span(c_ptr);
}

pub const MAX_SYMLINK_DEPTH: usize = 40;

pub err RealpathError {
    ELOOP,
    TooManySymlinks,
    NameTooLong,
    NotFound,
    IoError,
    OutOfMemory,
}

pub struct CachedDirEntry {
    name: []const u8 = "",
    kind: DirEntryKind = .unknown,
    ino: u64 = 0,
}

/// Cached directory metadata snapshot including entries, manifest flags,
/// `node_modules` / `cap_modules` presence, and a 64-bit `StatHash` for staleness detection.
pub struct DirInfo {
    abs_path: []const u8 = "",
    abs_real_path: []const u8 = "",
    entries: []CachedDirEntry = [],
    has_package_json: bool = false,
    has_tsconfig_json: bool = false,
    has_node_modules: bool = false,
    has_cap_modules: bool = false,
    is_node_modules: bool = false,
    is_cap_modules: bool = false,
    inside_node_modules: bool = false,
    node_modules_dir: []const u8 = "",
    cap_modules_dir: []const u8 = "",
    dir_stat_hash: StatHash = StatHash{},
    generation: u64 = 0,

    pub fn entryCount(): usize {
        return _.entries.len;
    }

    pub fn getEntry(name: []const u8): CachedDirEntry? {
        for (e of _.entries) {
            if (std.mem.eql(u8, e.name, name)) return e;
        }
        return null;
    }

    pub fn hasEntry(name: []const u8): bool {
        return _.getEntry(name) != null;
    }

    pub fn hasPackageJson(): bool {
        return _.has_package_json;
    }

    pub fn hasTsConfigJson(): bool {
        return _.has_tsconfig_json;
    }

    pub fn hasNodeModules(): bool {
        return _.has_node_modules;
    }

    pub fn hasCapModules(): bool {
        return _.has_cap_modules;
    }

    pub fn isNodeModules(): bool {
        return _.is_node_modules;
    }

    pub fn isCapModules(): bool {
        return _.is_cap_modules;
    }

    pub fn isInsideNodeModules(): bool {
        return _.inside_node_modules;
    }

    /// Re-stats the directory and compares its current 64-bit `StatHash` against the cached hash.
    /// Returns `true` if the directory was modified, replaced, or deleted.
    pub fn isStale(): bool {
        const current = StatHash.fromPath(_.abs_path) catch return true;
        return !current.eql(_.dir_stat_hash);
    }
}

pub struct DirCacheStats {
    hits: u64 = 0,
    misses: u64 = 0,
    invalidations: u64 = 0,
    cached_dirs: usize = 0,

    pub fn hitRate(): f64 {
        const total = _.hits + _.misses;
        if (total == 0) return 0.0;
        return (_.hits as f64) / (total as f64);
    }
}

fn joinDirPathAlloc(allocator: std.mem.Allocator, base: []const u8, child: []const u8): ([]const u8)! {
    if (std.mem.eql(u8, base, "/")) {
        return std.fmt.allocPrint(allocator, "/{s}", .{child});
    }
    if (std.mem.endsWith(u8, base, "/")) {
        return std.fmt.allocPrint(allocator, "{s}{s}", .{ base, child });
    }
    return std.fmt.allocPrint(allocator, "{s}/{s}", .{ base, child });
}

fn containsSegment(path: []const u8, target: []const u8): bool {
    let it = std.mem.splitScalar(u8, path, '/');
    while (seg := it.next()) {
        if (std.mem.eql(u8, seg, target)) return true;
    }
    return false;
}

/// Industrial-grade directory metadata cache backed by `cap:sys` `DirIterator` (`getdents64`)
/// and `StatHash` staleness verification.
pub struct DirEntryCache {
    allocator: std.mem.Allocator,
    index_map: std.StringHashMapUnmanaged(usize) = .empty,
    items: std.ArrayList(DirInfo) = .empty,
    hits: u64 = 0,
    misses: u64 = 0,
    invalidations: u64 = 0,
    generation: u64 = 1,
    auto_check_stale: bool = false,

    pub static fn init(allocator: std.mem.Allocator): DirEntryCache {
        return DirEntryCache{
            allocator: allocator,
        };
    }

    pub static fn initWithStaleCheck(allocator: std.mem.Allocator, auto_check_stale: bool): DirEntryCache {
        return DirEntryCache{
            allocator: allocator,
            auto_check_stale: auto_check_stale,
        };
    }

    fn freeDirInfo(*_, info: DirInfo) {
        if (info.abs_path.len > 0) _.allocator.free(info.abs_path);
        if (info.abs_real_path.len > 0 && !std.mem.eql(u8, info.abs_real_path, info.abs_path)) {
            _.allocator.free(info.abs_real_path);
        }
        if (info.node_modules_dir.len > 0) _.allocator.free(info.node_modules_dir);
        if (info.cap_modules_dir.len > 0) _.allocator.free(info.cap_modules_dir);
        for (e of info.entries) {
            if (e.name.len > 0) _.allocator.free(e.name);
        }
        if (info.entries.len > 0) _.allocator.free(info.entries);
    }

    pub fn deinit(*_) {
        for (info of _.items.items) {
            _.freeDirInfo(info);
        }
        _.items.deinit(_.allocator);
        _.index_map.deinit(_.allocator);
    }

    static fn normalizeKeyAlloc(allocator: std.mem.Allocator, dir_path: []const u8): ([]u8)! {
        let out_buf: [4096]u8 = undefined;
        if (isAbsolutePosix(dir_path)) {
            const normed = normalizePosix(dir_path, &out_buf);
            return allocator.dupe(u8, normed);
        }
        let cwd_buf: [2048]u8 = undefined;
        const cwd = sysGetcwd(&cwd_buf) catch ".";
        let raw_buf: [4096]u8 = undefined;
        const combined = if (std.mem.eql(u8, dir_path, "."))
            cwd
        else
            std.fmt.bufPrint(&raw_buf, "{s}/{s}", .{ cwd, dir_path }) catch dir_path;
        const normed = normalizePosix(combined, &out_buf);
        return allocator.dupe(u8, normed);
    }

    fn loadFromDisk(*_, norm_path: []const u8): DirInfo! {
        let iter = DirIterator.open(norm_path)?;
        defer iter.close();

        let entries: std.ArrayList(CachedDirEntry) = .empty;
        errdefer {
            for (e of entries.items) _.allocator.free(e.name);
            entries.deinit(_.allocator);
        }

        let has_pkg = false;
        let has_tsc = false;
        let has_nm = false;
        let has_cm = false;

        while (entry := iter.next()?) {
            const name_dup = _.allocator.dupe(u8, entry.name)?;
            if (std.mem.eql(u8, name_dup, "package.json") && entry.kind != .directory) {
                has_pkg = true;
            } else if (std.mem.eql(u8, name_dup, "tsconfig.json") && entry.kind != .directory) {
                has_tsc = true;
            } else if (std.mem.eql(u8, name_dup, "node_modules") && (entry.kind == .directory || entry.kind == .sym_link)) {
                has_nm = true;
            } else if (std.mem.eql(u8, name_dup, "cap_modules") && (entry.kind == .directory || entry.kind == .sym_link)) {
                has_cm = true;
            }
            entries.append(_.allocator, CachedDirEntry{
                name: name_dup,
                kind: entry.kind,
                ino: entry.ino,
            })?;
        }

        const abs_owned = _.allocator.dupe(u8, norm_path)?;
        const real_owned = RealpathResolver.resolve(_.allocator, norm_path) catch abs_owned;
        const st_hash = StatHash.fromPath(norm_path) catch StatHash{};

        const base = PosixPath.basename(norm_path, null);
        const is_nm = std.mem.eql(u8, base, "node_modules");
        const is_cm = std.mem.eql(u8, base, "cap_modules");
        const inside_nm = is_nm || containsSegment(norm_path, "node_modules") || containsSegment(norm_path, "cap_modules");

        const nm_dir = if (has_nm) joinDirPathAlloc(_.allocator, norm_path, "node_modules")? else "";
        const cm_dir = if (has_cm) joinDirPathAlloc(_.allocator, norm_path, "cap_modules")? else "";

        const gen = _.generation;
        _.generation += 1;

        return DirInfo{
            abs_path: abs_owned,
            abs_real_path: real_owned,
            entries: entries.toOwnedSlice(_.allocator)?,
            has_package_json: has_pkg,
            has_tsconfig_json: has_tsc,
            has_node_modules: has_nm,
            has_cap_modules: has_cm,
            is_node_modules: is_nm,
            is_cap_modules: is_cm,
            inside_node_modules: inside_nm,
            node_modules_dir: nm_dir,
            cap_modules_dir: cm_dir,
            dir_stat_hash: st_hash,
            generation: gen,
        };
    }

    /// Returns cached `DirInfo` for `dir_path`, loading from disk on cache miss
    /// (or when `auto_check_stale` detects `isStale()`).
    pub fn getOrLoad(*_, dir_path: []const u8): DirInfo! {
        const norm_path = DirEntryCache.normalizeKeyAlloc(_.allocator, dir_path)?;
        defer _.allocator.free(norm_path);

        if (idx := _.index_map.get(norm_path)) {
            const existing = _.items.items[idx];
            if (!_.auto_check_stale || !existing.isStale()) {
                _.hits += 1;
                return existing;
            }
            // Stale entry detected: invalidate and reload in place
            _.invalidations += 1;
            _.misses += 1;
            const reloaded = _.loadFromDisk(norm_path)?;
            _.freeDirInfo(existing);
            _.items.items[idx] = reloaded;
            * = _.index_map.remove(norm_path);
            _.index_map.put(_.allocator, reloaded.abs_path, idx)?;
            return reloaded;
        }

        _.misses += 1;
        const loaded = _.loadFromDisk(norm_path)?;
        const new_idx = _.items.items.len;
        _.items.append(_.allocator, loaded)?;
        _.index_map.put(_.allocator, loaded.abs_path, new_idx)?;
        return loaded;
    }

    /// Like `getOrLoad`, but always verifies `isStale()` via `StatHash` and reloads if changed.
    pub fn getFresh(*_, dir_path: []const u8): DirInfo! {
        const norm_path = DirEntryCache.normalizeKeyAlloc(_.allocator, dir_path)?;
        defer _.allocator.free(norm_path);

        if (idx := _.index_map.get(norm_path)) {
            const existing = _.items.items[idx];
            if (!existing.isStale()) {
                _.hits += 1;
                return existing;
            }
            _.invalidations += 1;
            _.misses += 1;
            const reloaded = _.loadFromDisk(norm_path)?;
            _.freeDirInfo(existing);
            _.items.items[idx] = reloaded;
            * = _.index_map.remove(norm_path);
            _.index_map.put(_.allocator, reloaded.abs_path, idx)?;
            return reloaded;
        }

        _.misses += 1;
        const loaded = _.loadFromDisk(norm_path)?;
        const new_idx = _.items.items.len;
        _.items.append(_.allocator, loaded)?;
        _.index_map.put(_.allocator, loaded.abs_path, new_idx)?;
        return loaded;
    }

    /// Pure cache lookup without touching disk.
    pub fn lookup(*_, dir_path: []const u8): DirInfo? {
        const norm_path = DirEntryCache.normalizeKeyAlloc(_.allocator, dir_path) catch return null;
        defer _.allocator.free(norm_path);
        if (idx := _.index_map.get(norm_path)) {
            _.hits += 1;
            return _.items.items[idx];
        }
        return null;
    }

    /// Explicitly invalidates a cached directory entry. Returns `true` if an entry was present.
    pub fn invalidate(*_, dir_path: []const u8): bool {
        const norm_path = DirEntryCache.normalizeKeyAlloc(_.allocator, dir_path) catch return false;
        defer _.allocator.free(norm_path);
        if (kv := _.index_map.fetchRemove(norm_path)) {
            const idx = kv.value;
            const last_idx = _.items.items.len - 1;
            const removed = _.items.items[idx];
            if (idx != last_idx) {
                const moved = _.items.items[last_idx];
                _.items.items[idx] = moved;
                * = _.index_map.remove(moved.abs_path);
                _.index_map.put(_.allocator, moved.abs_path, idx) catch {};
            }
            * = _.items.pop();
            _.freeDirInfo(removed);
            _.invalidations += 1;
            return true;
        }
        return false;
    }

    pub fn clear(*_) {
        for (info of _.items.items) {
            _.freeDirInfo(info);
        }
        _.items.clearRetainingCapacity();
        _.index_map.clearRetainingCapacity();
    }

    pub fn count(): usize {
        return _.items.items.len;
    }

    pub fn stats(): DirCacheStats {
        return DirCacheStats{
            hits: _.hits,
            misses: _.misses,
            invalidations: _.invalidations,
            cached_dirs: _.items.items.len,
        };
    }

    /// Walks upward from `start_dir` using cached `DirInfo` entries to locate the nearest
    /// directory containing `package.json`.
    pub fn findNearestPackageDir(*_, start_dir: []const u8): ([]const u8)? {
        let cur = DirEntryCache.normalizeKeyAlloc(_.allocator, start_dir) catch return null;
        defer _.allocator.free(cur);
        let depth: usize = 0;
        while (depth < 64) : (depth += 1) {
            const info = _.getOrLoad(cur) catch return null;
            if (info.has_package_json) {
                return info.abs_path;
            }
            const parent = PosixPath.dirname(info.abs_path);
            if (parent.len == 0 || std.mem.eql(u8, parent, info.abs_path)) break;
            const next_cur = _.allocator.dupe(u8, parent) catch return null;
            _.allocator.free(cur);
            cur = next_cur;
        }
        return null;
    }

    /// Walks upward from `start_dir` using cached `DirInfo` entries to locate the nearest
    /// `cap_modules` (when `prefer_cap_modules == true`) or `node_modules` directory.
    pub fn findNearestModulesDir(*_, start_dir: []const u8, prefer_cap_modules: bool): ([]const u8)? {
        let cur = DirEntryCache.normalizeKeyAlloc(_.allocator, start_dir) catch return null;
        defer _.allocator.free(cur);
        let depth: usize = 0;
        while (depth < 64) : (depth += 1) {
            const info = _.getOrLoad(cur) catch return null;
            if (prefer_cap_modules && info.has_cap_modules) {
                return info.cap_modules_dir;
            }
            if (info.has_node_modules) {
                return info.node_modules_dir;
            }
            const parent = PosixPath.dirname(info.abs_path);
            if (parent.len == 0 || std.mem.eql(u8, parent, info.abs_path)) break;
            const next_cur = _.allocator.dupe(u8, parent) catch return null;
            _.allocator.free(cur);
            cur = next_cur;
        }
        return null;
    }
}

// ============================================================================
// 2. RealpathResolver — Symlink Traversal with ELOOP Cycle Protection
// ============================================================================

pub struct RealpathResult {
    path: []u8 = "",
    symlinks_followed: usize = 0,
}

/// POSIX symlink-penetrating canonical path resolver (`lstat` + `readlink` component walk)
/// with `MAX_SYMLINK_DEPTH = 40` (`error.ELOOP`) cycle interception.
pub struct RealpathResolver {
    pub static fn resolve(allocator: std.mem.Allocator, raw_path: []const u8): ([]u8)! {
        const res = RealpathResolver.resolveDetailed(allocator, raw_path, MAX_SYMLINK_DEPTH)?;
        return res.path;
    }

    pub static fn resolveWithMaxDepth(
        allocator: std.mem.Allocator,
        raw_path: []const u8,
        max_depth: usize,
    ): ([]u8)! {
        const res = RealpathResolver.resolveDetailed(allocator, raw_path, max_depth)?;
        return res.path;
    }

    pub static fn resolveDetailed(
        allocator: std.mem.Allocator,
        raw_path: []const u8,
        max_depth: usize,
    ): RealpathResult! {
        if (raw_path.len == 0) return error.NotFound;

        let resolved: std.ArrayList(u8) = .empty;
        errdefer resolved.deinit(allocator);

        let pending: std.ArrayList(u8) = .empty;
        defer pending.deinit(allocator);

        if (raw_path[0] == '/') {
            resolved.append(allocator, '/')?;
            pending.appendSlice(allocator, raw_path[1..])?;
        } else {
            let cwd_buf: [2048]u8 = undefined;
            const cwd = sysGetcwd(&cwd_buf) catch return error.IoError;
            if (cwd.len == 0) {
                resolved.append(allocator, '/')?;
            } else {
                resolved.appendSlice(allocator, cwd)?;
            }
            pending.appendSlice(allocator, raw_path)?;
        }

        let pos: usize = 0;
        let symlink_depth: usize = 0;

        while (pos < pending.items.len) {
            // Skip consecutive '/'
            while (pos < pending.items.len && pending.items[pos] == '/') {
                pos += 1;
            }
            if (pos >= pending.items.len) break;

            const seg_start = pos;
            while (pos < pending.items.len && pending.items[pos] != '/') {
                pos += 1;
            }
            const seg = pending.items[seg_start..pos];

            if (std.mem.eql(u8, seg, ".")) {
                continue;
            }
            if (std.mem.eql(u8, seg, "..")) {
                if (resolved.items.len > 1) {
                    while (resolved.items.len > 1 && resolved.items[resolved.items.len - 1] != '/') {
                        * = resolved.pop();
                    }
                    if (resolved.items.len > 1 && resolved.items[resolved.items.len - 1] == '/') {
                        * = resolved.pop();
                    }
                }
                continue;
            }

            const prev_len = resolved.items.len;
            if (resolved.items.len > 1 || resolved.items[0] != '/') {
                resolved.append(allocator, '/')?;
            }
            resolved.appendSlice(allocator, seg)?;

            const is_sym = sysLstatExistsAndIsSymlink(resolved.items) catch {
                return error.NotFound;
            };

            if (is_sym) {
                symlink_depth += 1;
                if (symlink_depth > max_depth) {
                    return error.ELOOP;
                }

                let link_buf: [4096]u8 = undefined;
                const target = sysReadlink(resolved.items, &link_buf) catch {
                    return error.IoError;
                };
                if (target.len == 0) return error.NotFound;

                const remainder = pending.items[pos..];
                let next_pending: std.ArrayList(u8) = .empty;
                errdefer next_pending.deinit(allocator);

                if (target[0] == '/') {
                    resolved.shrinkRetainingCapacity(0);
                    resolved.append(allocator, '/')?;
                    next_pending.appendSlice(allocator, target[1..])?;
                } else {
                    resolved.shrinkRetainingCapacity(prev_len);
                    next_pending.appendSlice(allocator, target)?;
                }

                if (remainder.len > 0) {
                    next_pending.append(allocator, '/')?;
                    next_pending.appendSlice(allocator, remainder)?;
                }

                pending.deinit(allocator);
                pending = next_pending;
                pos = 0;
            }
        }

        if (resolved.items.len == 0) {
            resolved.append(allocator, '/')?;
        }

        return RealpathResult{
            path: resolved.toOwnedSlice(allocator)?,
            symlinks_followed: symlink_depth,
        };
    }
}
