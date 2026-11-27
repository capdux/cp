// @cap/file-entry-cache — Zero-heap incremental file metadata cache (clean-room CP)

pub const FEC_CAPACITY: usize = 64;

pub struct FileCacheEntry {
    path: string,
    mtime_ms: i64,
    size: u64,
    content_hash: u64,
    payload: string,
    valid: bool,
}

fn fecStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn fecEmptyEntry(): FileCacheEntry {
    return FileCacheEntry{
        path: "",
        mtime_ms: 0,
        size: 0,
        content_hash: 0,
        payload: "",
        valid: false,
    };
}

fn fecPathExists(path: string, existing_paths: []const string): bool {
    let i: usize = 0;
    while (i < existing_paths.len) : (i += 1) {
        if (fecStrEq(path, existing_paths[i])) return true;
    }
    return false;
}

pub struct FileEntryCache {
    entries: [64]FileCacheEntry,
    count: usize,

    static fn new(): FileEntryCache {
        let arr: [64]FileCacheEntry = [fecEmptyEntry(); 64];
        return FileEntryCache{
            entries: arr,
            count: 0,
        };
    }

    fn set(
        *_,
        path: string,
        mtime_ms: i64,
        size: u64,
        content_hash: u64,
        payload: string,
    ): bool {
        let free_idx: usize = FEC_CAPACITY;
        let i: usize = 0;
        while (i < FEC_CAPACITY) : (i += 1) {
            if (_.entries[i].valid) {
                if (fecStrEq(_.entries[i].path, path)) {
                    _.entries[i] = FileCacheEntry{
                        path: path,
                        mtime_ms: mtime_ms,
                        size: size,
                        content_hash: content_hash,
                        payload: payload,
                        valid: true,
                    };
                    return true;
                }
            } else if (free_idx == FEC_CAPACITY) {
                free_idx = i;
            }
        }
        if (free_idx == FEC_CAPACITY) return false;
        _.entries[free_idx] = FileCacheEntry{
            path: path,
            mtime_ms: mtime_ms,
            size: size,
            content_hash: content_hash,
            payload: payload,
            valid: true,
        };
        _.count += 1;
        return true;
    }

    fn get(path: string): FileCacheEntry? {
        let i: usize = 0;
        while (i < FEC_CAPACITY) : (i += 1) {
            if (_.entries[i].valid && fecStrEq(_.entries[i].path, path)) {
                return _.entries[i];
            }
        }
        return null;
    }

    fn hasChanged(path: string, mtime_ms: i64, size: u64, content_hash: u64): bool {
        let i: usize = 0;
        while (i < FEC_CAPACITY) : (i += 1) {
            if (_.entries[i].valid && fecStrEq(_.entries[i].path, path)) {
                if (_.entries[i].mtime_ms == mtime_ms &&
                    _.entries[i].size == size &&
                    _.entries[i].content_hash == content_hash) {
                    return false;
                }
                return true;
            }
        }
        return true;
    }

    fn remove(*_, path: string): bool {
        let i: usize = 0;
        while (i < FEC_CAPACITY) : (i += 1) {
            if (_.entries[i].valid && fecStrEq(_.entries[i].path, path)) {
                _.entries[i] = fecEmptyEntry();
                _.count -= 1;
                return true;
            }
        }
        return false;
    }

    fn reconcile(*_, existing_paths: []const string): usize {
        let removed: usize = 0;
        let i: usize = 0;
        while (i < FEC_CAPACITY) : (i += 1) {
            if (_.entries[i].valid) {
                if (!fecPathExists(_.entries[i].path, existing_paths)) {
                    _.entries[i] = fecEmptyEntry();
                    _.count -= 1;
                    removed += 1;
                }
            }
        }
        return removed;
    }

    fn len(): usize {
        return _.count;
    }
}
