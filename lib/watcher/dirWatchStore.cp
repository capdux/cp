// Source: Absorbed from /data/dev/bun/src/bake/DevServer/DirectoryWatchStore.cp | Synced: 2026-11-23T20:58:45Z
// Pure-CP zero-allocation directory watch & intrusive dependency-chain store for
// tracking unresolved relative imports in DevServer: deduplicates watched directories,
// stores per-directory failed-import dependencies in a stable-index intrusive linked
// list with O(1) free-list recycling, and automatically tears down empty directory watches.

pub const DIR_WATCH_MAX_DIRS: usize = 32;
pub const DIR_WATCH_MAX_DEPS: usize = 128;
pub const DIR_WATCH_NONE_INDEX: u32 = 4294967295;

pub error DirWatchStoreError {
    EmptyDirPath,
    EmptySpecifier,
    DirsCapacityExceeded,
    DepsCapacityExceeded,
}

pub enum DirWatchLoaderKind: u8 {
    Script = 0, // .js / .jsx / .ts / .tsx -> only track relative ("./", "../")
    Css = 1,    // .css -> track any non-empty specifier
    Html = 2,   // .html -> track any non-empty specifier
}

/// Returns whether a failed import resolution should be tracked by `DirWatchStore`.
/// Mirrors Bun `DirectoryWatchStore.trackResolutionFailure` guard rules.
pub fn shouldTrackResolutionFailure(
    import_source: []const u8,
    specifier: []const u8,
    loader: DirWatchLoaderKind,
): bool {
    if (specifier.len == 0 || import_source.len == 0) return false;
    if (!isAbsPath(import_source)) return false;
    return switch (loader) {
        .Script => hasPrefix2(specifier, '.', '/') || hasPrefix3(specifier, '.', '.', '/'),
        .Css, .Html => true,
    };
}

pub struct DirWatchDep {
    active: bool = false,
    next: u32 = DIR_WATCH_NONE_INDEX,
    source_file_path: []const u8 = "",
    specifier: []const u8 = "",
}

pub struct DirWatchEntry {
    dir_path: []const u8 = "",
    watch_id: u16 = 0,
    first_dep: u32 = DIR_WATCH_NONE_INDEX,
    dep_count: usize = 0,
}

pub struct DirWatchInsertResult {
    dir_created: bool = false,
    dep_inserted: bool = false,
    dep_index: u32 = DIR_WATCH_NONE_INDEX,
    watch_id: u16 = 0,
}

pub struct DirWatchRemoveOutcome {
    dep_removed: bool = false,
    dir_removed: bool = false,
    removed_watch_id: u16 = 0,
}

/// Zero-allocation directory watch store with stable-index intrusive dependency chains
/// and O(1) free-list slot recycling.
pub struct DirWatchStore {
    watches: [DIR_WATCH_MAX_DIRS]DirWatchEntry = [DirWatchEntry{}; DIR_WATCH_MAX_DIRS],
    watch_count: usize = 0,

    deps: [DIR_WATCH_MAX_DEPS]DirWatchDep = [DirWatchDep{}; DIR_WATCH_MAX_DEPS],
    dep_len: usize = 0,
    active_dep_count: usize = 0,

    free_list: [DIR_WATCH_MAX_DEPS]u32 = [0; DIR_WATCH_MAX_DEPS],
    free_count: usize = 0,

    static fn init(): DirWatchStore {
        return DirWatchStore{};
    }

    fn watchCount(): usize {
        return _.watch_count;
    }

    fn activeDepCount(): usize {
        return _.active_dep_count;
    }

    fn freeSlotCount(): usize {
        return _.free_count;
    }

    /// Registers `(source_file_path, specifier)` under `dir_path` (normalized without trailing slash).
    /// Deduplicates identical `(source_file_path, specifier)` on the same directory.
    /// If `dir_path` is newly watched, assigns `watch_id` and sets `dir_created = true`.
    fn insert(
        *_,
        dir_path: []const u8,
        source_file_path: []const u8,
        specifier: []const u8,
        watch_id: u16,
    ): DirWatchInsertResult ! DirWatchStoreError {
        if (dir_path.len == 0) return err.EmptyDirPath;
        if (specifier.len == 0) return err.EmptySpecifier;
        const norm_dir = stripTrailingSep(dir_path);

        if (widx := _.findWatchIndex(norm_dir)) {
            // Check if (source_file_path, specifier) already exists in this dir's chain
            let cur = _.watches[widx].first_dep;
            while (cur != DIR_WATCH_NONE_INDEX) {
                const d = _.deps[@as(usize, cur)];
                if (d.source_file_path == source_file_path && d.specifier == specifier) {
                    return DirWatchInsertResult{
                        dir_created: false,
                        dep_inserted: false,
                        dep_index: cur,
                        watch_id: _.watches[widx].watch_id,
                    };
                }
                cur = d.next;
            }

            const new_dep_idx = _.allocDepSlot(DirWatchDep{
                active: true,
                next: _.watches[widx].first_dep,
                source_file_path: source_file_path,
                specifier: specifier,
            })?;
            _.watches[widx].first_dep = new_dep_idx;
            _.watches[widx].dep_count += 1;
            return DirWatchInsertResult{
                dir_created: false,
                dep_inserted: true,
                dep_index: new_dep_idx,
                watch_id: _.watches[widx].watch_id,
            };
        }

        if (_.watch_count >= DIR_WATCH_MAX_DIRS) {
            return err.DirsCapacityExceeded;
        }
        const new_dep_idx = _.allocDepSlot(DirWatchDep{
            active: true,
            next: DIR_WATCH_NONE_INDEX,
            source_file_path: source_file_path,
            specifier: specifier,
        })?;
        _.watches[_.watch_count] = DirWatchEntry{
            dir_path: norm_dir,
            watch_id: watch_id,
            first_dep: new_dep_idx,
            dep_count: 1,
        };
        _.watch_count += 1;
        return DirWatchInsertResult{
            dir_created: true,
            dep_inserted: true,
            dep_index: new_dep_idx,
            watch_id: watch_id,
        };
    }

    fn findDir(dir_path: []const u8): DirWatchEntry? {
        if (dir_path.len == 0) return null;
        if (widx := _.findWatchIndex(stripTrailingSep(dir_path))) {
            return _.watches[widx];
        }
        return null;
    }

    /// Walks the intrusive linked list for `dir_path` and copies active dependencies into `out_deps`.
    /// Returns the number of dependencies written.
    fn collectDepsForDir(dir_path: []const u8, out_deps: []DirWatchDep): usize {
        if (dir_path.len == 0) return 0;
        const widx = _.findWatchIndex(stripTrailingSep(dir_path)) ?? return 0;
        let written: usize = 0;
        let cur = _.watches[widx].first_dep;
        while (cur != DIR_WATCH_NONE_INDEX && written < out_deps.len) {
            out_deps[written] = _.deps[@as(usize, cur)];
            written += 1;
            cur = _.deps[@as(usize, cur)].next;
        }
        return written;
    }

    /// Removes a specific `(source_file_path, specifier)` dependency from `dir_path`,
    /// recycles its slot to `free_list`, and removes the directory watch if no dependencies remain.
    fn removeDep(
        *_,
        dir_path: []const u8,
        source_file_path: []const u8,
        specifier: []const u8,
    ): DirWatchRemoveOutcome {
        if (dir_path.len == 0) return DirWatchRemoveOutcome{};
        const widx = _.findWatchIndex(stripTrailingSep(dir_path)) ?? return DirWatchRemoveOutcome{};

        let prev: u32 = DIR_WATCH_NONE_INDEX;
        let cur = _.watches[widx].first_dep;
        while (cur != DIR_WATCH_NONE_INDEX) {
            const cur_usize = @as(usize, cur);
            const next_idx = _.deps[cur_usize].next;
            if (_.deps[cur_usize].source_file_path == source_file_path &&
                _.deps[cur_usize].specifier == specifier)
            {
                if (prev == DIR_WATCH_NONE_INDEX) {
                    _.watches[widx].first_dep = next_idx;
                } else {
                    _.deps[@as(usize, prev)].next = next_idx;
                }
                _.freeDepSlot(cur);
                if (_.watches[widx].dep_count > 0) _.watches[widx].dep_count -= 1;

                if (_.watches[widx].first_dep == DIR_WATCH_NONE_INDEX) {
                    const wid = _.watches[widx].watch_id;
                    _.swapRemoveWatch(widx);
                    return DirWatchRemoveOutcome{
                        dep_removed: true,
                        dir_removed: true,
                        removed_watch_id: wid,
                    };
                }
                return DirWatchRemoveOutcome{
                    dep_removed: true,
                    dir_removed: false,
                    removed_watch_id: 0,
                };
            }
            prev = cur;
            cur = next_idx;
        }
        return DirWatchRemoveOutcome{};
    }

    /// Removes all failed-import dependencies belonging to `source_file_path` across all watched
    /// directories, writing any newly-emptied directory `watch_id`s into `out_removed_watch_ids`.
    /// Returns the number of watch IDs written.
    fn removeSourceFile(*_, source_file_path: []const u8, out_removed_watch_ids: []u16): usize {
        let removed_dirs: usize = 0;
        let widx: usize = 0;
        while (widx < _.watch_count) {
            let prev: u32 = DIR_WATCH_NONE_INDEX;
            let cur = _.watches[widx].first_dep;
            while (cur != DIR_WATCH_NONE_INDEX) {
                const cur_usize = @as(usize, cur);
                const next_idx = _.deps[cur_usize].next;
                if (_.deps[cur_usize].source_file_path == source_file_path) {
                    if (prev == DIR_WATCH_NONE_INDEX) {
                        _.watches[widx].first_dep = next_idx;
                    } else {
                        _.deps[@as(usize, prev)].next = next_idx;
                    }
                    _.freeDepSlot(cur);
                    if (_.watches[widx].dep_count > 0) _.watches[widx].dep_count -= 1;
                } else {
                    prev = cur;
                }
                cur = next_idx;
            }
            if (_.watches[widx].first_dep == DIR_WATCH_NONE_INDEX) {
                const wid = _.watches[widx].watch_id;
                _.swapRemoveWatch(widx);
                if (removed_dirs < out_removed_watch_ids.len) {
                    out_removed_watch_ids[removed_dirs] = wid;
                    removed_dirs += 1;
                }
            } else {
                widx += 1;
            }
        }
        return removed_dirs;
    }

    // --- Internal helpers ---

    fn findWatchIndex(norm_dir: []const u8): usize? {
        let i: usize = 0;
        while (i < _.watch_count) {
            if (_.watches[i].dir_path == norm_dir) return i;
            i += 1;
        }
        return null;
    }

    fn allocDepSlot(*_, dep: DirWatchDep): u32 ! DirWatchStoreError {
        if (_.free_count > 0) {
            _.free_count -= 1;
            const slot = _.free_list[_.free_count];
            _.deps[@as(usize, slot)] = dep;
            _.active_dep_count += 1;
            return slot;
        }
        if (_.dep_len >= DIR_WATCH_MAX_DEPS) {
            return err.DepsCapacityExceeded;
        }
        const slot = @as(u32, _.dep_len);
        _.deps[_.dep_len] = dep;
        _.dep_len += 1;
        _.active_dep_count += 1;
        return slot;
    }

    fn freeDepSlot(*_, idx: u32) {
        const u = @as(usize, idx);
        _.deps[u].active = false;
        _.deps[u].next = DIR_WATCH_NONE_INDEX;
        if (_.active_dep_count > 0) _.active_dep_count -= 1;
        if (_.free_count < DIR_WATCH_MAX_DEPS) {
            _.free_list[_.free_count] = idx;
            _.free_count += 1;
        }
    }

    fn swapRemoveWatch(*_, widx: usize) {
        const last = _.watch_count - 1;
        if (widx != last) {
            _.watches[widx] = _.watches[last];
        }
        _.watch_count -= 1;
    }
}

fn isAbsPath(p: []const u8): bool {
    if (p.len == 0) return false;
    if (p[0] == '/') return true;
    // Windows drive path e.g. C:\ or C:/
    if (p.len >= 3 && p[1] == ':' && (p[2] == '\\' || p[2] == '/')) return true;
    return false;
}

fn hasPrefix2(s: []const u8, a: u8, b: u8): bool {
    return s.len >= 2 && s[0] == a && s[1] == b;
}

fn hasPrefix3(s: []const u8, a: u8, b: u8, c: u8): bool {
    return s.len >= 3 && s[0] == a && s[1] == b && s[2] == c;
}

fn stripTrailingSep(p: []const u8): []const u8 {
    if (p.len <= 1) return p;
    let end = p.len;
    while (end > 1 && (p[end - 1] == '/' || p[end - 1] == '\\')) {
        end -= 1;
    }
    return p[0..end];
}
