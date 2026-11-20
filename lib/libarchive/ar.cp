// Source: cp/lib/libarchive/ar.cp (pure CP Unix ar .a/.deb codec)

const std = @import("std");
import { ArchiveError, ArchiveEntry, ArchiveEntryKind, ArchiveLimits, validateArchivePath, freeEntries } from "./format.cp";

const AR_MAGIC = "!<arch>\n";
const AR_HEADER_SIZE: usize = 60;
const AR_FMAG = "`\n";

fn writePaddedDec(buf: []u8, val: u64) {
    @memset(buf, ' ');
    let tmp: [32]u8 = undefined;
    const s = std.fmt.bufPrint(&tmp, "{d}", .{val}) catch return;
    const copy_len = @min(s.len, buf.len);
    @memcpy(buf[0..copy_len], s[0..copy_len]);
}

fn writePaddedOct(buf: []u8, val: u32) {
    @memset(buf, ' ');
    let tmp: [32]u8 = undefined;
    const s = std.fmt.bufPrint(&tmp, "{o}", .{val}) catch return;
    const copy_len = @min(s.len, buf.len);
    @memcpy(buf[0..copy_len], s[0..copy_len]);
}

fn parseTrimmedUint(field: []const u8, radix: u8): ArchiveError!u64 {
    const trimmed = std.mem.trim(u8, field, " \x00");
    if (trimmed.len == 0) {
        return 0;
    }
    return std.fmt.parseInt(u64, trimmed, radix) catch return error.InvalidHeader;
}

fn writeArHeader(out: *std.ArrayList(u8), alloc: std.mem.Allocator, name_field: []const u8, mtime: u64, uid: u32, gid: u32, mode: u32, size: usize): ArchiveError!void {
    let hdr: [AR_HEADER_SIZE]u8 = undefined;
    @memset(&hdr, ' ');
    const n_len = @min(name_field.len, 16);
    @memcpy(hdr[0..n_len], name_field[0..n_len]);
    writePaddedDec(hdr[16..28], mtime);
    writePaddedDec(hdr[28..34], uid);
    writePaddedDec(hdr[34..40], gid);
    writePaddedOct(hdr[40..48], mode);
    writePaddedDec(hdr[48..58], size);
    hdr[58] = '`';
    hdr[59] = '\n';
    out.appendSlice(alloc, &hdr) catch return error.OutOfMemory;
}

pub fn encodeAr(alloc: std.mem.Allocator, entries: []const ArchiveEntry): ArchiveError![]u8 {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    out.appendSlice(alloc, AR_MAGIC) catch return error.OutOfMemory;

    // Build GNU "//" longname table if any entry name > 15 bytes
    let longnames = std.ArrayList(u8).empty;
    defer longnames.deinit(alloc);

    let offsets = std.ArrayList(usize).empty;
    defer offsets.deinit(alloc);

    for (entry of entries) {
        validateArchivePath(entry.name, false) catch |e| return e;
        if (entry.name.len > 15) {
            const off = longnames.items.len;
            offsets.append(alloc, off) catch return error.OutOfMemory;
            longnames.appendSlice(alloc, entry.name) catch return error.OutOfMemory;
            longnames.appendSlice(alloc, "/\n") catch return error.OutOfMemory;
        } else {
            offsets.append(alloc, std.math.maxInt(usize)) catch return error.OutOfMemory;
        }
    }

    if (longnames.items.len > 0) {
        writeArHeader(&out, alloc, "//", 0, 0, 0, 0, longnames.items.len) catch |e| return e;
        out.appendSlice(alloc, longnames.items) catch return error.OutOfMemory;
        if ((longnames.items.len & 1) == 1) {
            out.append(alloc, '\n') catch return error.OutOfMemory;
        }
    }

    let idx: usize = 0;
    for (entry of entries) {
        let name_buf: [16]u8 = undefined;
        let name_slice: []const u8 = "";
        const off = offsets.items[idx];
        idx += 1;
        if (off == std.math.maxInt(usize)) {
            const s = std.fmt.bufPrint(&name_buf, "{s}/", .{entry.name}) catch return error.InvalidHeader;
            name_slice = s;
        } else {
            const s = std.fmt.bufPrint(&name_buf, "/{d}", .{off}) catch return error.InvalidHeader;
            name_slice = s;
        }

        writeArHeader(&out, alloc, name_slice, entry.mtime, entry.uid, entry.gid, entry.mode, entry.data.len) catch |e| return e;
        out.appendSlice(alloc, entry.data) catch return error.OutOfMemory;
        if ((entry.data.len & 1) == 1) {
            out.append(alloc, '\n') catch return error.OutOfMemory;
        }
    }

    return out.toOwnedSlice(alloc) catch return error.OutOfMemory;
}

pub fn decodeAr(alloc: std.mem.Allocator, data: []const u8, limits: ArchiveLimits): ArchiveError![]ArchiveEntry {
    if (data.len < AR_MAGIC.len) {
        return error.TruncatedHeader;
    }
    if (!std.mem.eql(u8, data[0..AR_MAGIC.len], AR_MAGIC)) {
        return error.InvalidMagic;
    }

    let entries = std.ArrayList(ArchiveEntry).empty;
    errdefer {
        for (entry of entries.items) {
            if (entry.name.len > 0) alloc.free(entry.name);
            if (entry.data.len > 0) alloc.free(entry.data);
            if (entry.link_name.len > 0) alloc.free(entry.link_name);
        }
        entries.deinit(alloc);
    }

    let longname_table: []const u8 = "";
    let pos: usize = AR_MAGIC.len;
    let total_bytes: usize = 0;

    while (pos < data.len) {
        if (pos + AR_HEADER_SIZE > data.len) {
            return error.TruncatedHeader;
        }
        const hdr = data[pos .. pos + AR_HEADER_SIZE];
        pos += AR_HEADER_SIZE;

        if (!std.mem.eql(u8, hdr[58..60], AR_FMAG)) {
            return error.InvalidHeader;
        }

        const raw_name = std.mem.trimRight(u8, hdr[0..16], " ");
        const mtime = parseTrimmedUint(hdr[16..28], 10) catch |e| return e;
        const uid = @as(u32, @truncate(parseTrimmedUint(hdr[28..34], 10) catch |e| return e));
        const gid = @as(u32, @truncate(parseTrimmedUint(hdr[34..40], 10) catch |e| return e));
        const mode = @as(u32, @truncate(parseTrimmedUint(hdr[40..48], 8) catch |e| return e));
        const size_u64 = parseTrimmedUint(hdr[48..58], 10) catch |e| return e;
        if (size_u64 > limits.max_entry_bytes) {
            return error.SizeLimitExceeded;
        }
        const size: usize = @intCast(size_u64);

        if (pos + size > data.len) {
            return error.TruncatedHeader;
        }
        const payload = data[pos .. pos + size];
        pos += size;
        if ((size & 1) == 1 && pos < data.len) {
            pos += 1;
        }

        // GNU longname table "//"
        if (std.mem.eql(u8, raw_name, "//")) {
            longname_table = payload;
            continue;
        }
        // GNU symbol table "/" or "/SYM64/"
        if (std.mem.eql(u8, raw_name, "/") || std.mem.eql(u8, raw_name, "/SYM64/")) {
            continue;
        }

        let resolved_name: []const u8 = raw_name;
        let actual_payload: []const u8 = payload;

        if (raw_name.len >= 2 && raw_name[0] == '/' && raw_name[1] >= '0' && raw_name[1] <= '9') {
            const off_u64 = parseTrimmedUint(raw_name[1..], 10) catch |e| return e;
            const off: usize = @intCast(off_u64);
            if (off >= longname_table.len) {
                return error.InvalidHeader;
            }
            const rest = longname_table[off..];
            let end_idx: usize = 0;
            while (end_idx < rest.len && rest[end_idx] != '\n' && rest[end_idx] != '/') {
                end_idx += 1;
            }
            resolved_name = rest[0..end_idx];
        } else if (std.mem.startsWith(u8, raw_name, "#1/")) {
            const bsd_len_u64 = parseTrimmedUint(raw_name[3..], 10) catch |e| return e;
            const bsd_len: usize = @intCast(bsd_len_u64);
            if (bsd_len > payload.len) {
                return error.TruncatedHeader;
            }
            resolved_name = std.mem.trimRight(u8, payload[0..bsd_len], "\x00");
            actual_payload = payload[bsd_len..];
        } else if (raw_name.len > 0 && raw_name[raw_name.len - 1] == '/') {
            resolved_name = raw_name[0 .. raw_name.len - 1];
        }

        validateArchivePath(resolved_name, limits.allow_absolute_paths) catch |e| return e;

        if (entries.items.len >= limits.max_entries) {
            return error.TooManyEntries;
        }
        total_bytes += actual_payload.len;
        if (total_bytes > limits.max_total_bytes) {
            return error.SizeLimitExceeded;
        }

        const owned_name = alloc.dupe(u8, resolved_name) catch return error.OutOfMemory;
        errdefer alloc.free(owned_name);
        const owned_data = alloc.dupe(u8, actual_payload) catch return error.OutOfMemory;

        entries.append(alloc, ArchiveEntry{
            .name = owned_name,
            .data = owned_data,
            .mode = mode,
            .uid = uid,
            .gid = gid,
            .mtime = mtime,
            .kind = .File,
            .link_name = "",
        }) catch {
            alloc.free(owned_data);
            return error.OutOfMemory;
        };
    }

    return entries.toOwnedSlice(alloc) catch return error.OutOfMemory;
}
