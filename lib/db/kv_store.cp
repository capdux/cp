// Source: cp/lib/kv/store.cp (Bitcask-Style Durable KV Store with Atomic Compaction)
// Pure CP crash-safe embedded key-value storage engine backed by CRC32 append-only WAL
// and atomic temp-write + fsync + rename compaction using native std.c / std.os.linux I/O.

const std = @import("std");
import { KvError, WalOp, WalRecord, WalCodec } from "./kv_wal.cp";

const EINTR: i32 = 4;
const SEEK_SET: i32 = 0;
const SEEK_END: i32 = 2;
const COMPACT_SUFFIX: string = ".compact.tmp";

pub struct KvStoreOptions {
    sync_on_write: bool = false,
    recover_torn_tail: bool = true,
}

pub struct KvStats {
    live_keys: usize = 0,
    total_records: u64 = 0,
    dead_records: u64 = 0,
    wal_bytes: u64 = 0,
    compactions: u64 = 0,
}

fn cErrno(): i32 {
    let val: i32 = 0;
    zig {
        val = @intFromEnum(std.c.errno(-1));
    }
    return val;
}

fn copyPathZ(path: string, buf: []u8): ([*:0]const u8)! {
    if (path.len == 0 || path.len >= buf.len) {
        return KvError.IoError;
    }
    @memcpy(buf[0..path.len], path);
    buf[path.len] = 0;
    return buf[0..path.len :0].ptr;
}

fn makeCompactPathZ(path: string, buf: []u8): ([*:0]const u8)! {
    const needed = path.len + COMPACT_SUFFIX.len;
    if (path.len == 0 || needed >= buf.len) {
        return KvError.IoError;
    }
    @memcpy(buf[0..path.len], path);
    @memcpy(buf[path.len..needed], COMPACT_SUFFIX);
    buf[needed] = 0;
    return buf[0..needed :0].ptr;
}

fn openRwCreate(path_z: [*:0]const u8): i32! {
    let z_flags = std.c.O{
        .CREAT = true,
        .CLOEXEC = true,
    };
    z_flags.ACCMODE = .RDWR;
    while (true) {
        const fd = std.c.open(path_z, z_flags, @as(std.c.mode_t, 0o644));
        if (fd >= 0) return fd;
        if (cErrno() == EINTR) continue;
        return KvError.IoError;
    }
}

fn openRwExisting(path_z: [*:0]const u8): i32! {
    let z_flags = std.c.O{
        .CLOEXEC = true,
    };
    z_flags.ACCMODE = .RDWR;
    while (true) {
        const fd = std.c.open(path_z, z_flags, @as(std.c.mode_t, 0));
        if (fd >= 0) return fd;
        if (cErrno() == EINTR) continue;
        return KvError.IoError;
    }
}

fn openWriteTrunc(path_z: [*:0]const u8): i32! {
    let z_flags = std.c.O{
        .CREAT = true,
        .TRUNC = true,
        .CLOEXEC = true,
    };
    z_flags.ACCMODE = .WRONLY;
    while (true) {
        const fd = std.c.open(path_z, z_flags, @as(std.c.mode_t, 0o644));
        if (fd >= 0) return fd;
        if (cErrno() == EINTR) continue;
        return KvError.IoError;
    }
}

fn closeFd(fd: i32) {
    if (fd >= 0) {
        * = std.c.close(fd);
    }
}

fn lseekFd(fd: i32, offset: i64, whence: i32): u64! {
    const rc = std.c.lseek(fd, @intCast(offset), @intCast(whence));
    if (rc < 0) return KvError.IoError;
    return rc as u64;
}

fn fsyncFd(fd: i32): ! {
    while (true) {
        const rc = std.c.fsync(fd);
        if (rc == 0) return;
        if (cErrno() == EINTR) continue;
        return KvError.IoError;
    }
}

fn ftruncateFd(fd: i32, length: u64): ! {
    while (true) {
        const rc = std.c.ftruncate(fd, @intCast(length));
        if (rc == 0) return;
        if (cErrno() == EINTR) continue;
        return KvError.IoError;
    }
}

fn writeAllFd(fd: i32, data: []const u8): ! {
    let written: usize = 0;
    while (written < data.len) {
        const rc = std.c.write(fd, data[written..data.len].ptr, data.len - written);
        if (rc > 0) {
            written += (rc as usize);
            continue;
        }
        if (rc < 0 && cErrno() == EINTR) {
            continue;
        }
        return KvError.IoError;
    }
}

fn readAllFd(fd: i32, buf: []u8): usize! {
    let total: usize = 0;
    while (total < buf.len) {
        const rc = std.c.read(fd, buf[total..buf.len].ptr, buf.len - total);
        if (rc > 0) {
            total += (rc as usize);
            continue;
        }
        if (rc == 0) {
            break;
        }
        if (cErrno() == EINTR) {
            continue;
        }
        return KvError.IoError;
    }
    return total;
}

fn renameZ(old_z: [*:0]const u8, new_z: [*:0]const u8): ! {
    while (true) {
        const rflags: std.os.linux.RENAME = @bitCast(@as(u32, 0));
        const rc = std.os.linux.renameat2(std.os.linux.AT.FDCWD, old_z, std.os.linux.AT.FDCWD, new_z, rflags);
        const e = std.os.linux.errno(rc);
        if (e == .SUCCESS) return;
        if (e == .INTR) continue;
        return KvError.IoError;
    }
}

fn unlinkZ(path_z: [*:0]const u8) {
    * = std.c.unlink(path_z);
}

fn stringLessThan(a: string, b: string): bool {
    return std.mem.order(u8, a, b) == .lt;
}

fn sortStringSlice(items: []string) {
    if (items.len <= 1) return;
    let i: usize = 1;
    while (i < items.len) : (i += 1) {
        const cur = items[i];
        let j: usize = i;
        while (j > 0 && stringLessThan(cur, items[j - 1])) : (j -= 1) {
            items[j] = items[j - 1];
        }
        items[j] = cur;
    }
}

pub struct KvStore {
    alloc: std.mem.Allocator,
    path: []u8 = "",
    fd: i32 = -1,
    opts: KvStoreOptions = KvStoreOptions{},
    entries: std.StringHashMapUnmanaged([]u8) = .empty,
    total_records: u64 = 0,
    dead_records: u64 = 0,
    wal_bytes: u64 = 0,
    compactions: u64 = 0,
    closed: bool = false,

    /// Opens or creates a Bitcask-style WAL store at `path` and replays all valid records.
    /// If `opts.recover_torn_tail` is true and a partial trailing frame is found, truncates
    /// the WAL back to the last valid record boundary via `ftruncate`.
    pub static fn open(alloc: std.mem.Allocator, path: string, opts: KvStoreOptions = KvStoreOptions{}): KvStore! {
        let path_buf: [4096]u8 = undefined;
        // Validate path fits together with ".compact.tmp" suffix.
        if (path.len == 0 || path.len + COMPACT_SUFFIX.len >= path_buf.len) {
            return KvError.IoError;
        }
        const path_z = copyPathZ(path, &path_buf)?;

        const owned_path = alloc.dupe(u8, path) catch return KvError.OutOfMemory;
        errdefer alloc.free(owned_path);

        const fd = openRwCreate(path_z)?;
        errdefer closeFd(fd);

        let entries: std.StringHashMapUnmanaged([]u8) = .empty;
        errdefer {
            let it = entries.iterator();
            while (entry := it.next()) {
                alloc.free(entry.key_ptr.*);
                alloc.free(entry.value_ptr.*);
            }
            entries.deinit(alloc);
        }

        const file_size_u64 = lseekFd(fd, 0, SEEK_END)?;
        * = lseekFd(fd, 0, SEEK_SET)?;

        let total_records: u64 = 0;
        let dead_records: u64 = 0;
        let valid_end: usize = 0;
        let saw_torn_tail = false;

        if (file_size_u64 > 0) {
            const file_size: usize = file_size_u64 as usize;
            let raw = alloc.alloc(u8, file_size) catch return KvError.OutOfMemory;
            defer alloc.free(raw);

            const n_read = readAllFd(fd, raw)?;
            const data = raw[0..n_read];

            let offset: usize = 0;
            while (offset < data.len) {
                const decoded = WalCodec.decodeNext(data, offset);
                switch (decoded) {
                    .Record frame => {
                        const rec = frame.record;
                        total_records += 1;
                        switch (rec.op) {
                            .Put => {
                                if (existing := entries.getPtr(rec.key)) {
                                    const new_val = alloc.dupe(u8, rec.value) catch return KvError.OutOfMemory;
                                    alloc.free(existing.*);
                                    existing.* = new_val;
                                    dead_records += 1;
                                } else {
                                    const key_dup = alloc.dupe(u8, rec.key) catch return KvError.OutOfMemory;
                                    errdefer alloc.free(key_dup);
                                    const val_dup = alloc.dupe(u8, rec.value) catch return KvError.OutOfMemory;
                                    errdefer alloc.free(val_dup);
                                    entries.put(alloc, key_dup, val_dup) catch return KvError.OutOfMemory;
                                }
                            },
                            .Delete => {
                                if (removed := entries.fetchRemove(rec.key)) {
                                    alloc.free(removed.key);
                                    alloc.free(removed.value);
                                    dead_records += 2;
                                } else {
                                    dead_records += 1;
                                }
                            },
                        }
                        offset = frame.next_offset;
                        valid_end = offset;
                    },
                    .Eof => {
                        break;
                    },
                    .TornTail => {
                        if (!opts.recover_torn_tail) {
                            return KvError.CorruptRecord;
                        }
                        saw_torn_tail = true;
                        break;
                    },
                    .CorruptRecord => {
                        return KvError.CorruptRecord;
                    },
                }
            }
        }

        if (saw_torn_tail) {
            ftruncateFd(fd, valid_end as u64)?;
            fsyncFd(fd)?;
        }
        * = lseekFd(fd, valid_end as i64, SEEK_SET)?;

        return KvStore{
            alloc: alloc,
            path: owned_path,
            fd: fd,
            opts: opts,
            entries: entries,
            total_records: total_records,
            dead_records: dead_records,
            wal_bytes: valid_end as u64,
            compactions: 0,
            closed: false,
        };
    }

    /// Inserts or updates a key-value pair and appends a CRC32-protected Put record to the WAL.
    pub fn put(*_, key: string, value: string): ! {
        if (_.closed || _.fd < 0) return KvError.IoError;
        if (key.len == 0) return KvError.InvalidKey;

        const frame = WalCodec.encode(_.alloc, WalRecord.put(key, value))?;
        defer _.alloc.free(frame);

        if (existing := _.entries.getPtr(key)) {
            const val_dup = _.alloc.dupe(u8, value) catch return KvError.OutOfMemory;
            errdefer _.alloc.free(val_dup);

            writeAllFd(_.fd, frame)?;
            if (_.opts.sync_on_write) {
                fsyncFd(_.fd)?;
            }

            _.alloc.free(existing.*);
            existing.* = val_dup;
            _.dead_records += 1;
        } else {
            _.entries.ensureUnusedCapacity(_.alloc, 1) catch return KvError.OutOfMemory;
            const key_dup = _.alloc.dupe(u8, key) catch return KvError.OutOfMemory;
            errdefer _.alloc.free(key_dup);
            const val_dup = _.alloc.dupe(u8, value) catch return KvError.OutOfMemory;
            errdefer _.alloc.free(val_dup);

            writeAllFd(_.fd, frame)?;
            if (_.opts.sync_on_write) {
                fsyncFd(_.fd)?;
            }

            _.entries.putAssumeCapacity(key_dup, val_dup);
        }

        _.total_records += 1;
        _.wal_bytes += (frame.len as u64);
    }

    /// Returns a borrowed view of the value associated with `key`, or `null` if absent.
    pub fn get(key: string): string? {
        if (key.len == 0) return null;
        if (v := _.entries.get(key)) {
            return v;
        }
        return null;
    }

    /// Deletes `key` if present by appending a WAL tombstone record and removing it from memory.
    /// Returns `true` if the key existed and was removed, or `false` if absent.
    pub fn delete(*_, key: string): bool! {
        if (_.closed || _.fd < 0) return KvError.IoError;
        if (key.len == 0) return KvError.InvalidKey;
        if (!_.entries.contains(key)) {
            return false;
        }

        const frame = WalCodec.encode(_.alloc, WalRecord.del(key))?;
        defer _.alloc.free(frame);

        writeAllFd(_.fd, frame)?;
        if (_.opts.sync_on_write) {
            fsyncFd(_.fd)?;
        }

        if (removed := _.entries.fetchRemove(key)) {
            _.alloc.free(removed.key);
            _.alloc.free(removed.value);
        }

        _.total_records += 1;
        _.dead_records += 2;
        _.wal_bytes += (frame.len as u64);
        return true;
    }

    /// Returns `true` if `key` currently exists in the store.
    pub fn has(key: string): bool {
        if (key.len == 0) return false;
        return _.entries.contains(key);
    }

    /// Returns the number of live keys currently in the store.
    pub fn len(): usize {
        return _.entries.count();
    }

    /// Returns all live keys sorted lexicographically.
    /// Caller owns each returned key slice and the outer slice (free with `KvStore.freeKeys`).
    pub fn keys(alloc: std.mem.Allocator): ([]string)! {
        return _.prefixScan(alloc, "");
    }

    /// Returns all live keys starting with `prefix`, sorted lexicographically.
    /// Caller owns each returned key slice and the outer slice (free with `KvStore.freeKeys`).
    pub fn prefixScan(alloc: std.mem.Allocator, prefix: string): ([]string)! {
        let count: usize = 0;
        let it_count = _.entries.iterator();
        while (entry := it_count.next()) {
            const k = entry.key_ptr.*;
            if (prefix.len == 0 || std.mem.startsWith(u8, k, prefix)) {
                count += 1;
            }
        }

        let out = alloc.alloc(string, count) catch return KvError.OutOfMemory;
        let filled: usize = 0;
        errdefer {
            let k_idx: usize = 0;
            while (k_idx < filled) : (k_idx += 1) {
                alloc.free(out[k_idx]);
            }
            alloc.free(out);
        }

        let it = _.entries.iterator();
        while (entry := it.next()) {
            const k = entry.key_ptr.*;
            if (prefix.len == 0 || std.mem.startsWith(u8, k, prefix)) {
                out[filled] = alloc.dupe(u8, k) catch return KvError.OutOfMemory;
                filled += 1;
            }
        }

        sortStringSlice(out);
        return out;
    }

    /// Frees a key list allocated by `keys()` or `prefixScan()`.
    pub static fn freeKeys(alloc: std.mem.Allocator, items: []string) {
        for (k of items) {
            alloc.free(k);
        }
        if (items.len > 0) {
            alloc.free(items);
        }
    }

    /// Flushes OS buffers for the active WAL file descriptor via `fsync`.
    pub fn sync(*_): ! {
        if (_.closed || _.fd < 0) return KvError.IoError;
        fsyncFd(_.fd)?;
    }

    /// Performs atomic Bitcask compaction:
    /// 1. Writes all live key-value pairs to `<path>.compact.tmp`
    /// 2. Calls `fsync` on the temporary file and closes it
    /// 3. Atomically replaces the active WAL via `rename`
    /// 4. Reopens the compacted WAL and resets dead record counters
    pub fn compact(*_): ! {
        if (_.closed || _.fd < 0) return KvError.IoError;

        let main_buf: [4096]u8 = undefined;
        let tmp_buf: [4096]u8 = undefined;
        const main_z = copyPathZ(_.path, &main_buf)?;
        const tmp_z = makeCompactPathZ(_.path, &tmp_buf)?;

        const tmp_fd = openWriteTrunc(tmp_z)?;
        let tmp_closed = false;
        errdefer {
            if (!tmp_closed) closeFd(tmp_fd);
            unlinkZ(tmp_z);
        }

        let new_bytes: u64 = 0;
        let live_count: u64 = 0;
        let it = _.entries.iterator();
        while (entry := it.next()) {
            const frame = WalCodec.encode(_.alloc, WalRecord.put(entry.key_ptr.*, entry.value_ptr.*))?;
            defer _.alloc.free(frame);
            writeAllFd(tmp_fd, frame)?;
            new_bytes += (frame.len as u64);
            live_count += 1;
        }

        fsyncFd(tmp_fd)?;
        closeFd(tmp_fd);
        tmp_closed = true;

        renameZ(tmp_z, main_z)?;

        const new_fd = openRwExisting(main_z)?;
        errdefer closeFd(new_fd);
        * = lseekFd(new_fd, new_bytes as i64, SEEK_SET)?;

        closeFd(_.fd);
        _.fd = new_fd;
        _.total_records = live_count;
        _.dead_records = 0;
        _.wal_bytes = new_bytes;
        _.compactions += 1;
    }

    /// Returns a snapshot of store metrics and WAL byte statistics.
    pub fn stats(): KvStats {
        return KvStats{
            live_keys: _.entries.count(),
            total_records: _.total_records,
            dead_records: _.dead_records,
            wal_bytes: _.wal_bytes,
            compactions: _.compactions,
        };
    }

    /// Closes the underlying WAL file descriptor. Idempotent.
    pub fn close(*_) {
        if (_.fd >= 0) {
            closeFd(_.fd);
            _.fd = -1;
        }
        _.closed = true;
    }

    /// Closes the WAL file descriptor and releases all heap memory owned by the store.
    pub fn deinit(*_) {
        _.close();
        let it = _.entries.iterator();
        while (entry := it.next()) {
            _.alloc.free(entry.key_ptr.*);
            _.alloc.free(entry.value_ptr.*);
        }
        _.entries.deinit(_.alloc);
        if (_.path.len > 0) {
            _.alloc.free(_.path);
            _.path = "";
        }
    }
}
