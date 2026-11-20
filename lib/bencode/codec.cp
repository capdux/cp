// Source: cp/lib/bencode/codec.cp (BEP-3 Strict Bencode Decoder & Canonical Encoder)
// Implements decode, decodeWithLimits, and canonical sorted encode.

const std = @import("std");
import {
    BENCODE_MAX_DEPTH,
    BencodeError,
    BencodeDictEntry,
    BencodeValue,
} from "./types.cp";

struct BencodeReader {
    src: []const u8 = "",
    pos: usize = 0,
    alloc: std.mem.Allocator = undefined,
    max_depth: usize = BENCODE_MAX_DEPTH,
    max_items: usize = 0,
    item_count: usize = 0,

    fn peekByte(): u8? {
        if (_.pos >= _.src.len) {
            return null;
        }
        return _.src[_.pos];
    }

    fn readByte(*_): u8 ! BencodeError {
        if (_.pos >= _.src.len) {
            return BencodeError.UnexpectedEof;
        }
        const b = _.src[_.pos];
        _.pos += 1;
        return b;
    }

    fn countItem(*_) ! BencodeError {
        _.item_count += 1;
        if (_.max_items > 0 && _.item_count > _.max_items) {
            return BencodeError.MaxSizeExceeded;
        }
    }

    fn readInt(*_): i64 ! BencodeError {
        const marker = _.readByte()?;
        if (marker != 'i') {
            return BencodeError.InvalidSyntax;
        }
        const start = _.pos;
        if (start >= _.src.len) {
            return BencodeError.UnexpectedEof;
        }

        let end_pos: usize = start;
        let found_e = false;
        while (end_pos < _.src.len) : (end_pos += 1) {
            if (_.src[end_pos] == 'e') {
                found_e = true;
                break;
            }
        }
        if (!found_e) {
            return BencodeError.UnexpectedEof;
        }

        const num_slice = _.src[start..end_pos];
        _.pos = end_pos + 1;

        if (num_slice.len == 0) {
            return BencodeError.InvalidInteger;
        }

        let is_neg = false;
        let idx: usize = 0;
        if (num_slice[0] == '-') {
            is_neg = true;
            idx = 1;
            if (num_slice.len == 1) {
                return BencodeError.InvalidInteger;
            }
            if (num_slice[1] == '0') {
                if (num_slice.len == 2) {
                    return BencodeError.NegativeZero;
                }
                return BencodeError.LeadingZero;
            }
        } else if (num_slice[0] == '0' && num_slice.len > 1) {
            return BencodeError.LeadingZero;
        }

        while (idx < num_slice.len) : (idx += 1) {
            const c = num_slice[idx];
            if (c < '0' || c > '9') {
                return BencodeError.InvalidInteger;
            }
        }

        const parsed = std.fmt.parseInt(i64, num_slice, 10) catch return BencodeError.InvalidInteger;
        if (is_neg && parsed == 0) {
            return BencodeError.NegativeZero;
        }
        return parsed;
    }

    fn readBytes(*_): ([]const u8) ! BencodeError {
        const first = _.peekByte() ?? return BencodeError.UnexpectedEof;
        if (first < '0' || first > '9') {
            return BencodeError.InvalidSyntax;
        }

        const start = _.pos;
        let colon_pos: usize = start;
        let found_colon = false;
        while (colon_pos < _.src.len) : (colon_pos += 1) {
            const c = _.src[colon_pos];
            if (c == ':') {
                found_colon = true;
                break;
            }
            if (c < '0' || c > '9') {
                return BencodeError.InvalidSyntax;
            }
        }
        if (!found_colon) {
            return BencodeError.UnexpectedEof;
        }

        const len_slice = _.src[start..colon_pos];
        if (len_slice.len == 0) {
            return BencodeError.InvalidSyntax;
        }
        if (len_slice[0] == '0' && len_slice.len > 1) {
            return BencodeError.LeadingZero;
        }

        const byte_len = std.fmt.parseInt(usize, len_slice, 10) catch return BencodeError.InvalidSyntax;
        _.pos = colon_pos + 1;

        if (byte_len > (_.src.len - _.pos)) {
            return BencodeError.UnexpectedEof;
        }
        const raw = _.src[_.pos .. _.pos + byte_len];
        _.pos += byte_len;

        const owned = _.alloc.dupe(u8, raw) catch return BencodeError.OutOfMemory;
        return owned;
    }

    fn readValue(*_, depth: usize): BencodeValue ! BencodeError {
        const b = _.peekByte() ?? return BencodeError.UnexpectedEof;
        if (b == 'i') {
            _.countItem()?;
            const iv = _.readInt()?;
            return BencodeValue.Int(iv);
        }
        if (b == 'l') {
            if (depth >= _.max_depth) {
                return BencodeError.MaxDepthExceeded;
            }
            _.countItem()?;
            * = _.readByte()?;

            let list: std.ArrayList(BencodeValue) = .empty;
            errdefer {
                let i: usize = 0;
                while (i < list.items.len) : (i += 1) {
                    list.items[i].deinit(_.alloc);
                }
                list.deinit(_.alloc);
            }

            while (true) {
                const next_b = _.peekByte() ?? return BencodeError.UnexpectedEof;
                if (next_b == 'e') {
                    * = _.readByte()?;
                    break;
                }
                let item = _.readValue(depth + 1)?;
                errdefer item.deinit(_.alloc);
                list.append(_.alloc, item) catch return BencodeError.OutOfMemory;
            }

            const items = list.toOwnedSlice(_.alloc) catch return BencodeError.OutOfMemory;
            return BencodeValue.List(items);
        }
        if (b == 'd') {
            if (depth >= _.max_depth) {
                return BencodeError.MaxDepthExceeded;
            }
            _.countItem()?;
            * = _.readByte()?;

            let entries: std.ArrayList(BencodeDictEntry) = .empty;
            errdefer {
                let i: usize = 0;
                while (i < entries.items.len) : (i += 1) {
                    _.alloc.free(entries.items[i].key);
                    entries.items[i].value.deinit(_.alloc);
                }
                entries.deinit(_.alloc);
            }

            let has_prev = false;
            let prev_key: []const u8 = "";

            while (true) {
                const next_b = _.peekByte() ?? return BencodeError.UnexpectedEof;
                if (next_b == 'e') {
                    * = _.readByte()?;
                    break;
                }

                const key = _.readBytes()?;
                errdefer _.alloc.free(key);

                if (has_prev) {
                    const ord = std.mem.order(u8, prev_key, key);
                    if (ord == .eq) {
                        return BencodeError.DuplicateDictKey;
                    }
                    if (ord == .gt) {
                        return BencodeError.UnorderedDictKeys;
                    }
                }

                let val = _.readValue(depth + 1)?;
                errdefer val.deinit(_.alloc);

                entries.append(_.alloc, BencodeDictEntry{ key: key, value: val }) catch return BencodeError.OutOfMemory;
                prev_key = key;
                has_prev = true;
            }

            const owned_entries = entries.toOwnedSlice(_.alloc) catch return BencodeError.OutOfMemory;
            return BencodeValue.Dict(owned_entries);
        }
        if (b >= '0' && b <= '9') {
            _.countItem()?;
            const bs = _.readBytes()?;
            return BencodeValue.Bytes(bs);
        }
        return BencodeError.InvalidSyntax;
    }
}

pub fn decodeWithLimits(
    alloc: std.mem.Allocator,
    data: []const u8,
    max_depth: usize,
    max_items: usize,
): BencodeValue ! BencodeError {
    if (data.len == 0) {
        return BencodeError.UnexpectedEof;
    }
    let reader = BencodeReader{
        src: data,
        pos: 0,
        alloc: alloc,
        max_depth: max_depth,
        max_items: max_items,
        item_count: 0,
    };
    let root = reader.readValue(0)?;
    if (reader.pos < reader.src.len) {
        root.deinit(alloc);
        return BencodeError.TrailingBytes;
    }
    return root;
}

pub fn decode(alloc: std.mem.Allocator, data: []const u8): BencodeValue ! BencodeError {
    return decodeWithLimits(alloc, data, BENCODE_MAX_DEPTH, 0);
}

fn writeBytes(out: *std.ArrayList(u8), alloc: std.mem.Allocator, bytes: []const u8) ! BencodeError {
    let len_buf: [32]u8 = undefined;
    const len_str = std.fmt.bufPrint(&len_buf, "{d}:", .{ bytes.len }) catch return BencodeError.OutOfMemory;
    out.appendSlice(alloc, len_str) catch return BencodeError.OutOfMemory;
    out.appendSlice(alloc, bytes) catch return BencodeError.OutOfMemory;
}

fn sortEntries(entries: []BencodeDictEntry) {
    let i: usize = 1;
    while (i < entries.len) : (i += 1) {
        const cur = entries[i];
        let j: usize = i;
        while (j > 0 && std.mem.order(u8, entries[j - 1].key, cur.key) == .gt) : (j -= 1) {
            entries[j] = entries[j - 1];
        }
        entries[j] = cur;
    }
}

fn encodeValue(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: BencodeValue, depth: usize) ! BencodeError {
    switch (val) {
        .Int iv => {
            let buf: [32]u8 = undefined;
            const s = std.fmt.bufPrint(&buf, "i{d}e", .{ iv }) catch return BencodeError.OutOfMemory;
            out.appendSlice(alloc, s) catch return BencodeError.OutOfMemory;
        },
        .Bytes bs => {
            writeBytes(out, alloc, bs)?;
        },
        .List items => {
            if (depth >= BENCODE_MAX_DEPTH) {
                return BencodeError.MaxDepthExceeded;
            }
            out.append(alloc, 'l') catch return BencodeError.OutOfMemory;
            let i: usize = 0;
            while (i < items.len) : (i += 1) {
                encodeValue(out, alloc, items[i], depth + 1)?;
            }
            out.append(alloc, 'e') catch return BencodeError.OutOfMemory;
        },
        .Dict entries => {
            if (depth >= BENCODE_MAX_DEPTH) {
                return BencodeError.MaxDepthExceeded;
            }
            out.append(alloc, 'd') catch return BencodeError.OutOfMemory;
            const sorted = alloc.alloc(BencodeDictEntry, entries.len) catch return BencodeError.OutOfMemory;
            defer alloc.free(sorted);
            @memcpy(sorted, entries);
            sortEntries(sorted);

            let i: usize = 0;
            while (i < sorted.len) : (i += 1) {
                writeBytes(out, alloc, sorted[i].key)?;
                encodeValue(out, alloc, sorted[i].value, depth + 1)?;
            }
            out.append(alloc, 'e') catch return BencodeError.OutOfMemory;
        },
    }
}

pub fn encode(alloc: std.mem.Allocator, value: BencodeValue): ([]u8) ! BencodeError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    encodeValue(&out, alloc, value, 0)?;
    return out.toOwnedSlice(alloc) catch return BencodeError.OutOfMemory;
}
