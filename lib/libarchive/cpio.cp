// Source: cp/lib/libarchive/cpio.cp (pure CP SVR4 cpio newc 070701 / crc 070702 codec)

const std = @import("std");
import { ArchiveError, ArchiveEntry, ArchiveEntryKind, ArchiveLimits, validateArchivePath } from "./format.cp";

const CPIO_MAGIC_NEWC = "070701";
const CPIO_MAGIC_CRC = "070702";
const CPIO_HEADER_SIZE: usize = 110;
const CPIO_TRAILER = "TRAILER!!!";

fn align4(n: usize): usize {
    return (n + 3) & ~@as(usize, 3);
}

fn writeHex8(buf: []u8, val: u32) {
    * = std.fmt.bufPrint(buf[0..8], "{x:0>8}", .{val}) catch return;
}

fn parseHex8(buf: []const u8): ArchiveError!u32 {
    return std.fmt.parseInt(u32, buf, 16) catch return error.InvalidHeader;
}

fn appendZeros(out: *std.ArrayList(u8), alloc: std.mem.Allocator, count: usize): ArchiveError!void {
    let i: usize = 0;
    while (i < count) {
        out.append(alloc, 0) catch return error.OutOfMemory;
        i += 1;
    }
}

fn writeCpioRecord(
    out: *std.ArrayList(u8),
    alloc: std.mem.Allocator,
    ino: u32,
    mode: u32,
    uid: u32,
    gid: u32,
    nlink: u32,
    mtime: u64,
    name: []const u8,
    payload: []const u8,
): ArchiveError!void {
    let hdr: [CPIO_HEADER_SIZE]u8 = undefined;
    @memcpy(hdr[0..6], CPIO_MAGIC_NEWC);
    writeHex8(hdr[6..14], ino);
    writeHex8(hdr[14..22], mode);
    writeHex8(hdr[22..30], uid);
    writeHex8(hdr[30..38], gid);
    writeHex8(hdr[38..46], nlink);
    writeHex8(hdr[46..54], @as(u32, @truncate(mtime)));
    writeHex8(hdr[54..62], @as(u32, @truncate(payload.len)));
    writeHex8(hdr[62..70], 0);
    writeHex8(hdr[70..78], 0);
    writeHex8(hdr[78..86], 0);
    writeHex8(hdr[86..94], 0);
    writeHex8(hdr[94..102], @as(u32, @truncate(name.len + 1)));
    writeHex8(hdr[102..110], 0);

    out.appendSlice(alloc, &hdr) catch return error.OutOfMemory;
    out.appendSlice(alloc, name) catch return error.OutOfMemory;
    out.append(alloc, 0) catch return error.OutOfMemory;

    const hdr_plus_name = CPIO_HEADER_SIZE + name.len + 1;
    const pad_name = align4(hdr_plus_name) - hdr_plus_name;
    appendZeros(out, alloc, pad_name) catch |e| return e;

    out.appendSlice(alloc, payload) catch return error.OutOfMemory;
    const pad_data = align4(payload.len) - payload.len;
    appendZeros(out, alloc, pad_data) catch |e| return e;
}

pub fn encodeCpioNewc(alloc: std.mem.Allocator, entries: []const ArchiveEntry): ArchiveError![]u8 {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    let idx: usize = 0;
    for (entry of entries) {
        idx += 1;
        validateArchivePath(entry.name, false) catch |e| return e;
        let mode = entry.mode;
        if ((mode & 0o170000) == 0) {
            if (entry.kind == .Directory) {
                mode |= 0o040000;
            } else if (entry.kind == .Symlink) {
                mode |= 0o120000;
            } else {
                mode |= 0o100000;
            }
        }
        const nlink: u32 = if (entry.kind == .Directory) 2 else 1;
        const payload = if (entry.kind == .Directory) "" else entry.data;
        writeCpioRecord(&out, alloc, @as(u32, @truncate(idx)), mode, entry.uid, entry.gid, nlink, entry.mtime, entry.name, payload) catch |e| return e;
    }

    writeCpioRecord(&out, alloc, 0, 0, 0, 0, 1, 0, CPIO_TRAILER, "") catch |e| return e;
    return out.toOwnedSlice(alloc) catch return error.OutOfMemory;
}

pub fn decodeCpioNewc(alloc: std.mem.Allocator, data: []const u8, limits: ArchiveLimits): ArchiveError![]ArchiveEntry {
    let entries = std.ArrayList(ArchiveEntry).empty;
    errdefer {
        for (entry of entries.items) {
            if (entry.name.len > 0) alloc.free(entry.name);
            if (entry.data.len > 0) alloc.free(entry.data);
            if (entry.link_name.len > 0) alloc.free(entry.link_name);
        }
        entries.deinit(alloc);
    }

    let pos: usize = 0;
    let total_bytes: usize = 0;

    while (pos < data.len) {
        if (pos + CPIO_HEADER_SIZE > data.len) {
            return error.TruncatedHeader;
        }
        const hdr = data[pos .. pos + CPIO_HEADER_SIZE];
        const magic = hdr[0..6];
        const is_newc = std.mem.eql(u8, magic, CPIO_MAGIC_NEWC);
        const is_crc = std.mem.eql(u8, magic, CPIO_MAGIC_CRC);
        if (!is_newc && !is_crc) {
            return error.InvalidMagic;
        }

        const mode = parseHex8(hdr[14..22]) catch |e| return e;
        const uid = parseHex8(hdr[22..30]) catch |e| return e;
        const gid = parseHex8(hdr[30..38]) catch |e| return e;
        const mtime = parseHex8(hdr[46..54]) catch |e| return e;
        const filesize = @as(usize, parseHex8(hdr[54..62]) catch |e| return e);
        const namesize = @as(usize, parseHex8(hdr[94..102]) catch |e| return e);
        const check = parseHex8(hdr[102..110]) catch |e| return e;

        if (namesize == 0) {
            return error.InvalidHeader;
        }
        if (filesize > limits.max_entry_bytes) {
            return error.SizeLimitExceeded;
        }

        const name_start = pos + CPIO_HEADER_SIZE;
        const name_end = name_start + namesize;
        const data_start = align4(name_end);
        if (data_start > data.len || data_start + filesize > data.len) {
            return error.TruncatedHeader;
        }

        const raw_name = data[name_start..name_end];
        const name = std.mem.trimRight(u8, raw_name, "\x00");
        const payload = data[data_start .. data_start + filesize];
        pos = align4(data_start + filesize);

        if (pos > data.len) {
            pos = data.len;
        }

        if (is_crc) {
            let sum: u32 = 0;
            for (b of payload) {
                sum = sum +% b;
            }
            if (sum != check) {
                return error.InvalidChecksum;
            }
        }

        if (std.mem.eql(u8, name, CPIO_TRAILER)) {
            break;
        }

        validateArchivePath(name, limits.allow_absolute_paths) catch |e| return e;

        if (entries.items.len >= limits.max_entries) {
            return error.TooManyEntries;
        }
        total_bytes += payload.len;
        if (total_bytes > limits.max_total_bytes) {
            return error.SizeLimitExceeded;
        }

        const ftype = mode & 0o170000;
        let kind: ArchiveEntryKind = .File;
        if (ftype == 0o040000) {
            kind = .Directory;
        } else if (ftype == 0o120000) {
            kind = .Symlink;
        } else if (ftype != 0 && ftype != 0o100000) {
            kind = .Other;
        }

        const owned_name = alloc.dupe(u8, name) catch return error.OutOfMemory;
        errdefer alloc.free(owned_name);
        const owned_data = alloc.dupe(u8, payload) catch return error.OutOfMemory;

        entries.append(alloc, ArchiveEntry{
            .name = owned_name,
            .data = owned_data,
            .mode = mode,
            .uid = uid,
            .gid = gid,
            .mtime = mtime,
            .kind = kind,
            .link_name = "",
        }) catch {
            alloc.free(owned_data);
            return error.OutOfMemory;
        };
    }

    return entries.toOwnedSlice(alloc) catch return error.OutOfMemory;
}
