// Source: Absorbed from /data/dev/bun/src/bake/DevServer/HotReloadEvent.cp | Synced: 2026-11-22T17:07:40Z
// Pure-CP zero-allocation HMR event batch accumulator designed to pair with
// `WatcherAtomicRelay<HotReloadBatch>`: deduplicates changed files & watched
// directories, joins directory+subpath notifications into a compact NUL-delimited
// stack arena without heap allocation, and supports O(1) slot reset/reuse.

pub const HOT_RELOAD_MAX_FILES: usize = 64;
pub const HOT_RELOAD_MAX_DIRS: usize = 32;
pub const HOT_RELOAD_EXTRA_ARENA_CAP: usize = 4096;

pub error HotReloadBatchError {
    FilesCapacityExceeded,
    DirsCapacityExceeded,
    ExtraArenaFull,
    IndexOutOfBounds,
}

pub fn isHotReloadPathSep(c: u8): bool {
    return c == '/' || c == '\\';
}

/// Strips redundant trailing path separators while preserving root `"/"`.
/// Mirrors Bun's `withoutTrailingSlashWindowsPath` normalization in `HotReloadEvent`.
pub fn stripTrailingPathSep(path: []const u8): []const u8 {
    if (path.len <= 1) return path;
    let end = path.len;
    while (end > 1 && isHotReloadPathSep(path[end - 1])) {
        end -= 1;
    }
    return path[0..end];
}

pub struct HotReloadSpan {
    offset: u16 = 0,
    len: u16 = 0,
}

/// Zero-allocation deduplicated file & directory change batch for HMR / DevServer.
pub struct HotReloadBatch {
    files: [HOT_RELOAD_MAX_FILES][]const u8 = [""; HOT_RELOAD_MAX_FILES],
    file_count: usize = 0,

    dirs: [HOT_RELOAD_MAX_DIRS][]const u8 = [""; HOT_RELOAD_MAX_DIRS],
    dir_count: usize = 0,

    extra_arena: [HOT_RELOAD_EXTRA_ARENA_CAP]u8 = [0; HOT_RELOAD_EXTRA_ARENA_CAP],
    extra_used: usize = 0,
    extra_spans: [HOT_RELOAD_MAX_FILES]HotReloadSpan = [HotReloadSpan{}; HOT_RELOAD_MAX_FILES],
    extra_count: usize = 0,

    timestamp_ms: u64 = 0,
    generation: u64 = 0,

    static fn init(): HotReloadBatch {
        return HotReloadBatch{};
    }

    /// Resets all deduplicated collections for zero-allocation reuse in a relay slot.
    fn reset(*_) {
        _.file_count = 0;
        _.dir_count = 0;
        _.extra_used = 0;
        _.extra_count = 0;
        _.timestamp_ms = 0;
        _.generation += 1;
    }

    /// Mirrors Bun `HotReloadEvent.isEmpty()`: true iff no files and no dirs are recorded.
    fn isEmpty(): bool {
        return (_.file_count + _.dir_count) == 0;
    }

    fn fileCount(): usize {
        return _.file_count;
    }

    fn dirCount(): usize {
        return _.dir_count;
    }

    fn extraFileCount(): usize {
        return _.extra_count;
    }

    fn extraArenaUsed(): usize {
        return _.extra_used;
    }

    /// Records a changed file path (deduplicated). Returns `true` if newly inserted.
    fn appendFile(*_, file_path: []const u8): bool ! HotReloadBatchError {
        if (file_path.len == 0) return false;
        let i: usize = 0;
        while (i < _.file_count) {
            if (_.files[i] == file_path) return false;
            i += 1;
        }
        if (_.file_count >= HOT_RELOAD_MAX_FILES) {
            return err.FilesCapacityExceeded;
        }
        _.files[_.file_count] = file_path;
        _.file_count += 1;
        return true;
    }

    /// Records a changed directory (trailing separators stripped & deduplicated) and,
    /// when `maybe_sub_path` is provided and non-empty, joins `dir + "/" + sub` into the
    /// compact NUL-delimited `extra_arena` (mirroring Bun `HotReloadEvent.appendDir`).
    /// Returns `true` if either the directory or the joined extra file was newly inserted.
    fn appendDir(*_, dir_path: []const u8, maybe_sub_path: ([]const u8)?): bool ! HotReloadBatchError {
        if (dir_path.len == 0) return false;
        const norm_dir = stripTrailingPathSep(dir_path);

        let inserted_any = false;
        let dir_exists = false;
        let i: usize = 0;
        while (i < _.dir_count) {
            if (_.dirs[i] == norm_dir) {
                dir_exists = true;
                break;
            }
            i += 1;
        }
        if (!dir_exists) {
            if (_.dir_count >= HOT_RELOAD_MAX_DIRS) {
                return err.DirsCapacityExceeded;
            }
            _.dirs[_.dir_count] = norm_dir;
            _.dir_count += 1;
            inserted_any = true;
        }

        if (raw_sub := maybe_sub_path) {
            if (raw_sub.len == 0) return inserted_any;
            // Trim leading separators from sub_path to prevent double slashes
            let sub_start: usize = 0;
            while (sub_start < raw_sub.len && isHotReloadPathSep(raw_sub[sub_start])) {
                sub_start += 1;
            }
            const sub = raw_sub[sub_start..];
            if (sub.len == 0) return inserted_any;

            const need_sep = !(norm_dir.len == 1 && isHotReloadPathSep(norm_dir[0]));
            const joined_len = norm_dir.len + (if (need_sep) 1 else 0) + sub.len;

            // Check if already recorded in extra_spans before writing to arena
            if (_.hasExtraJoined(norm_dir, need_sep, sub, joined_len)) {
                return inserted_any;
            }
            if (_.extra_count >= HOT_RELOAD_MAX_FILES) {
                return err.FilesCapacityExceeded;
            }
            if (_.extra_used + joined_len + 1 > HOT_RELOAD_EXTRA_ARENA_CAP) {
                return err.ExtraArenaFull;
            }

            const start_off = _.extra_used;
            let pos = start_off;
            let k: usize = 0;
            while (k < norm_dir.len) {
                _.extra_arena[pos] = norm_dir[k];
                pos += 1;
                k += 1;
            }
            if (need_sep) {
                _.extra_arena[pos] = '/';
                pos += 1;
            }
            k = 0;
            while (k < sub.len) {
                _.extra_arena[pos] = sub[k];
                pos += 1;
                k += 1;
            }
            // NUL-terminate entry (matches Bun extra_files NUL-delimited packing)
            _.extra_arena[pos] = 0;
            pos += 1;

            _.extra_spans[_.extra_count] = HotReloadSpan{
                offset: @as(u16, start_off),
                len: @as(u16, joined_len),
            };
            _.extra_count += 1;
            _.extra_used = pos;
            inserted_any = true;
        }

        return inserted_any;
    }

    fn getFile(idx: usize): []const u8 ! HotReloadBatchError {
        if (idx >= _.file_count) return err.IndexOutOfBounds;
        return _.files[idx];
    }

    fn getDir(idx: usize): []const u8 ! HotReloadBatchError {
        if (idx >= _.dir_count) return err.IndexOutOfBounds;
        return _.dirs[idx];
    }

    fn getExtraFile(idx: usize): []const u8 ! HotReloadBatchError {
        if (idx >= _.extra_count) return err.IndexOutOfBounds;
        const sp = _.extra_spans[idx];
        const off = @as(usize, sp.offset);
        const ln = @as(usize, sp.len);
        return _.extra_arena[off .. off + ln];
    }

    fn containsFile(path: []const u8): bool {
        let i: usize = 0;
        while (i < _.file_count) {
            if (_.files[i] == path) return true;
            i += 1;
        }
        i = 0;
        while (i < _.extra_count) {
            const sp = _.extra_spans[i];
            const off = @as(usize, sp.offset);
            const ln = @as(usize, sp.len);
            if (_.extra_arena[off .. off + ln] == path) return true;
            i += 1;
        }
        return false;
    }

    fn containsDir(dir_path: []const u8): bool {
        const norm = stripTrailingPathSep(dir_path);
        let i: usize = 0;
        while (i < _.dir_count) {
            if (_.dirs[i] == norm) return true;
            i += 1;
        }
        return false;
    }

    // --- Internal helper ---
    fn hasExtraJoined(norm_dir: []const u8, need_sep: bool, sub: []const u8, joined_len: usize): bool {
        let i: usize = 0;
        while (i < _.extra_count) {
            const sp = _.extra_spans[i];
            if (@as(usize, sp.len) == joined_len) {
                const off = @as(usize, sp.offset);
                const existing = _.extra_arena[off .. off + joined_len];
                if (matchesJoined(existing, norm_dir, need_sep, sub)) {
                    return true;
                }
            }
            i += 1;
        }
        return false;
    }
}

fn matchesJoined(existing: []const u8, norm_dir: []const u8, need_sep: bool, sub: []const u8): bool {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < norm_dir.len) {
        if (existing[pos] != norm_dir[i]) return false;
        pos += 1;
        i += 1;
    }
    if (need_sep) {
        if (existing[pos] != '/') return false;
        pos += 1;
    }
    i = 0;
    while (i < sub.len) {
        if (existing[pos] != sub[i]) return false;
        pos += 1;
        i += 1;
    }
    return pos == existing.len;
}
