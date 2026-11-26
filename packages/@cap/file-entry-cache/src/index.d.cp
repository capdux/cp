// Type definitions for CP

export class FileCacheEntry {
    path: string,
    mtime_ms: i64,
    size: u64,
    content_hash: u64,
    payload: string,
    valid: bool,
}

export fn fecStrEq(a: string, b: string): bool;
export fn fecEmptyEntry(): FileCacheEntry;
export fn fecPathExists(path: string, existing_paths: &[]string): bool;
export class FileEntryCache {
    entries: [64]FileCacheEntry,
    count: usize,
    fn new(): FileEntryCache;
    fn set(_: auto, path: string, mtime_ms: i64, size: u64, content_hash: u64, payload: string): bool;
    fn get(path: string): FileCacheEntry?;
    fn hasChanged(path: string, mtime_ms: i64, size: u64, content_hash: u64): bool;
    fn remove(_: auto, path: string): bool;
    fn reconcile(_: auto, existing_paths: &[]string): usize;
    fn len(): usize;
}

