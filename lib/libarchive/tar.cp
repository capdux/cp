// Source: cp/lib/libarchive/tar.cp (pure CP USTAR Tar codec)

const std = @import("std");
import { ArchiveError, ArchiveEntry, ArchiveEntryKind, ArchiveLimits, validateArchivePath } from "./format.cp";

const TAR_BLOCK: usize = 512;

fn writeOctalField(buf: []u8, val: u64) {
    @memset(buf, '0');
    buf[buf.len - 1] = 0;
    let v = val;
    let idx: usize = buf.len - 1;
    while (idx > 0) {
        idx -= 1;
        buf[idx] = @as(u8, @intCast('0' + (v & 7)));
        v >>= 3;
    }
}

fn parseOctalField(buf: []const u8): ArchiveError!u64 {
    const trimmed = std.mem.trim(u8, buf, " \x00");
    if (trimmed.len == 0) {
        return 0;
    }
    return std.fmt.parseInt(u64, trimmed, 8) catch return error.InvalidHeader;
}

fn computeTarChecksum(hdr: []const u8): u32 {
    let sum: u32 = 0;
    let i: usize = 0;
    for (b of hdr) {
        if (i >= 148 && i < 156) {
            sum += ' ';
        } else {
            sum += b;
        }
        i += 1;
    }
    return sum;
}

pub fn encodeUstarTar(alloc: std.mem.Allocator, entries: []const ArchiveEntry): ArchiveError![]u8 {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    for (entry of entries) {
        validateArchivePath(entry.name, false) catch |e| return e;
        if (entry.name.len > 100) {
            return error.InvalidHeader;
        }
        let hdr: [TAR_BLOCK]u8 = undefined;
        @memset(&hdr, 0);

        @memcpy(hdr[0..entry.name.len], entry.name);
        writeOctalField(hdr[100..108], entry.mode & 0o7777);
        writeOctalField(hdr[108..116], entry.uid);
        writeOctalField(hdr[116..124], entry.gid);
        const payload = if (entry.kind == .Directory) "" else entry.data;
        writeOctalField(hdr[124..136], payload.len);
        writeOctalField(hdr[136..148], entry.mtime);
        @memset(hdr[148..156], ' ');

        if (entry.kind == .Directory) {
            hdr[156] = '5';
        } else if (entry.kind == .Symlink) {
            hdr[156] = '2';
            const l_len = @min(entry.link_name.len, 100);
            @memcpy(hdr[157 .. 157 + l_len], entry.link_name[0..l_len]);
        } else {
            hdr[156] = '0';
        }

        @memcpy(hdr[257..263], "ustar\x00");
        @memcpy(hdr[263..265], "00");

        const cksum = computeTarChecksum(&hdr);
        writeOctalField(hdr[148..155], cksum);
        hdr[155] = ' ';

        out.appendSlice(alloc, &hdr) catch return error.OutOfMemory;
        if (payload.len > 0) {
            out.appendSlice(alloc, payload) catch return error.OutOfMemory;
            const rem = payload.len % TAR_BLOCK;
            if (rem != 0) {
                let pad: usize = TAR_BLOCK - rem;
                while (pad > 0) {
                    out.append(alloc, 0) catch return error.OutOfMemory;
                    pad -= 1;
                }
            }
        }
    }

    // Two 512-byte zero blocks as EOF marker
    let z: usize = 0;
    while (z < TAR_BLOCK * 2) {
        out.append(alloc, 0) catch return error.OutOfMemory;
        z += 1;
    }

    return out.toOwnedSlice(alloc) catch return error.OutOfMemory;
}

pub fn decodeUstarTar(alloc: std.mem.Allocator, data: []const u8, limits: ArchiveLimits): ArchiveError![]ArchiveEntry {
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

    while (pos + TAR_BLOCK <= data.len) {
        const hdr = data[pos .. pos + TAR_BLOCK];
        pos += TAR_BLOCK;

        let all_zero = true;
        for (b of hdr) {
            if (b != 0) {
                all_zero = false;
                break;
            }
        }
        if (all_zero) {
            break;
        }

        const expected_cksum = parseOctalField(hdr[148..156]) catch |e| return e;
        const actual_cksum = computeTarChecksum(hdr);
        if (expected_cksum != actual_cksum) {
            return error.InvalidChecksum;
        }

        const name_part = std.mem.trimRight(u8, hdr[0..100], "\x00");
        const mode = @as(u32, @truncate(parseOctalField(hdr[100..108]) catch |e| return e));
        const uid = @as(u32, @truncate(parseOctalField(hdr[108..116]) catch |e| return e));
        const gid = @as(u32, @truncate(parseOctalField(hdr[116..124]) catch |e| return e));
        const size_u64 = parseOctalField(hdr[124..136]) catch |e| return e;
        const mtime = parseOctalField(hdr[136..148]) catch |e| return e;

        if (size_u64 > limits.max_entry_bytes) {
            return error.SizeLimitExceeded;
        }
        const size: usize = @intCast(size_u64);
        if (pos + size > data.len) {
            return error.TruncatedHeader;
        }

        const payload = data[pos .. pos + size];
        const blocks = (size + TAR_BLOCK - 1) / TAR_BLOCK;
        pos += blocks * TAR_BLOCK;
        if (pos > data.len) {
            pos = data.len;
        }

        validateArchivePath(name_part, limits.allow_absolute_paths) catch |e| return e;

        if (entries.items.len >= limits.max_entries) {
            return error.TooManyEntries;
        }
        total_bytes += size;
        if (total_bytes > limits.max_total_bytes) {
            return error.SizeLimitExceeded;
        }

        let kind: ArchiveEntryKind = .File;
        if (hdr[156] == '5') {
            kind = .Directory;
        } else if (hdr[156] == '2') {
            kind = .Symlink;
        }

        const link_part = std.mem.trimRight(u8, hdr[157..257], "\x00");
        const owned_name = alloc.dupe(u8, name_part) catch return error.OutOfMemory;
        errdefer alloc.free(owned_name);
        const owned_data = alloc.dupe(u8, payload) catch return error.OutOfMemory;
        errdefer alloc.free(owned_data);
        const owned_link = alloc.dupe(u8, link_part) catch return error.OutOfMemory;

        entries.append(alloc, ArchiveEntry{
            .name = owned_name,
            .data = owned_data,
            .mode = mode,
            .uid = uid,
            .gid = gid,
            .mtime = mtime,
            .kind = kind,
            .link_name = owned_link,
        }) catch {
            alloc.free(owned_link);
            return error.OutOfMemory;
        };
    }

    return entries.toOwnedSlice(alloc) catch return error.OutOfMemory;
}
