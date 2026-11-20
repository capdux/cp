// Source: cp/lib/bencode/bencode.cp (BEP-3 Bencode Codec & Torrent MetaInfo Parser)
// Pure CP zero-C-dependency BitTorrent BEP-3 Bencode encoder/decoder,
// strict canonical dictionary ordering validator, and MetaInfo / SHA-1 InfoHash extractor.

const std = @import("std");
import { sha1Raw } from "cap:crypto";

pub const BENCODE_MAX_DEPTH: usize = 64;

pub err BencodeError {
    UnexpectedEof,
    InvalidInteger,
    LeadingZeroNotAllowed,
    NegativeZeroNotAllowed,
    InvalidStringLength,
    UnsortedDictKeys,
    DuplicateDictKey,
    TrailingBytes,
    MaxDepthExceeded,
    InvalidTorrent,
    OutOfMemory,
}

pub enum BencodeKind {
    Int,
    Bytes,
    List,
    Dict,
}

pub struct BencodeDictEntry {
    key: []const u8 = "",
    val: BencodeValue = BencodeValue.Int(0),

    pub static fn init(key: []const u8, val: BencodeValue): BencodeDictEntry {
        return BencodeDictEntry{ key: key, val: val };
    }
}

pub enum BencodeValue {
    Int: i64,
    Bytes: []const u8,
    List: []BencodeValue,
    Dict: []BencodeDictEntry,

    pub static fn makeInt(v: i64): BencodeValue {
        return BencodeValue.Int(v);
    }

    pub static fn makeBytes(v: []const u8): BencodeValue {
        return BencodeValue.Bytes(v);
    }

    pub static fn makeList(items: []BencodeValue): BencodeValue {
        return BencodeValue.List(items);
    }

    pub static fn makeDict(entries: []BencodeDictEntry): BencodeValue {
        return BencodeValue.Dict(entries);
    }

    pub fn kind(): BencodeKind {
        return switch (_) {
            .Int => .Int,
            .Bytes => .Bytes,
            .List => .List,
            .Dict => .Dict,
        };
    }

    pub fn asInt(): i64? {
        return switch (_) {
            .Int v => v,
            else => null,
        };
    }

    pub fn asBytes(): ([]const u8)? {
        return switch (_) {
            .Bytes b => b,
            else => null,
        };
    }

    pub fn asList(): ([]BencodeValue)? {
        return switch (_) {
            .List items => items,
            else => null,
        };
    }

    pub fn asDict(): ([]BencodeDictEntry)? {
        return switch (_) {
            .Dict entries => entries,
            else => null,
        };
    }

    pub fn get(key: []const u8): BencodeValue? {
        return switch (_) {
            .Dict entries => {
                let i: usize = 0;
                while (i < entries.len) : (i += 1) {
                    if (std.mem.eql(u8, entries[i].key, key)) {
                        return entries[i].val;
                    }
                }
                return null;
            },
            else => null,
        };
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        switch (_.*) {
            .Int => {},
            .Bytes b => {
                alloc.free(b);
            },
            .List items => {
                let i: usize = 0;
                while (i < items.len) : (i += 1) {
                    items[i].deinit(alloc);
                }
                alloc.free(items);
            },
            .Dict entries => {
                let i: usize = 0;
                while (i < entries.len) : (i += 1) {
                    alloc.free(entries[i].key);
                    entries[i].val.deinit(alloc);
                }
                alloc.free(entries);
            },
        }
        _.* = BencodeValue.Int(0);
    }
}

pub struct BencodeDoc {
    root: BencodeValue = BencodeValue.Int(0),
    alloc: std.mem.Allocator? = null,

    pub static fn init(alloc: std.mem.Allocator, root: BencodeValue): BencodeDoc {
        return BencodeDoc{
            root: root,
            alloc: alloc,
        };
    }

    pub fn deinit(*_) {
        if (a := _.alloc) {
            _.root.deinit(a);
            _.alloc = null;
        }
        _.root = BencodeValue.Int(0);
    }

    pub fn value(): BencodeValue {
        return _.root;
    }

    pub fn kind(): BencodeKind {
        return _.root.kind();
    }

    pub fn get(key: []const u8): BencodeValue? {
        return _.root.get(key);
    }

    pub fn asInt(): i64? {
        return _.root.asInt();
    }

    pub fn asBytes(): ([]const u8)? {
        return _.root.asBytes();
    }

    pub fn asList(): ([]BencodeValue)? {
        return _.root.asList();
    }

    pub fn asDict(): ([]BencodeDictEntry)? {
        return _.root.asDict();
    }
}

struct BencodeReader {
    src: []const u8 = "",
    pos: usize = 0,
    alloc: std.mem.Allocator = undefined,
    capture_top_info: bool = false,
    found_info: bool = false,
    info_start: usize = 0,
    info_end: usize = 0,

    fn peekByte(): u8? {
        if (_.pos >= _.src.len) return null;
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

    fn readInt(*_): i64 ! BencodeError {
        const marker = _.readByte()?;
        if (marker != 'i') {
            return BencodeError.InvalidInteger;
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
                    return BencodeError.NegativeZeroNotAllowed;
                }
                return BencodeError.LeadingZeroNotAllowed;
            }
        } else if (num_slice[0] == '0' && num_slice.len > 1) {
            return BencodeError.LeadingZeroNotAllowed;
        }

        while (idx < num_slice.len) : (idx += 1) {
            const c = num_slice[idx];
            if (c < '0' || c > '9') {
                return BencodeError.InvalidInteger;
            }
        }

        const parsed = std.fmt.parseInt(i64, num_slice, 10) catch return BencodeError.InvalidInteger;
        if (is_neg && parsed == 0) {
            return BencodeError.NegativeZeroNotAllowed;
        }
        return parsed;
    }

    fn readBytes(*_): ([]const u8) ! BencodeError {
        const first = _.peekByte() ?? return BencodeError.UnexpectedEof;
        if (first < '0' || first > '9') {
            return BencodeError.InvalidStringLength;
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
                return BencodeError.InvalidStringLength;
            }
        }
        if (!found_colon) {
            return BencodeError.UnexpectedEof;
        }

        const len_slice = _.src[start..colon_pos];
        if (len_slice.len == 0) {
            return BencodeError.InvalidStringLength;
        }
        if (len_slice[0] == '0' && len_slice.len > 1) {
            return BencodeError.LeadingZeroNotAllowed;
        }

        const byte_len = std.fmt.parseInt(usize, len_slice, 10) catch return BencodeError.InvalidStringLength;
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
            const iv = _.readInt()?;
            return BencodeValue.Int(iv);
        }
        if (b == 'l') {
            if (depth >= BENCODE_MAX_DEPTH) {
                return BencodeError.MaxDepthExceeded;
            }
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
            if (depth >= BENCODE_MAX_DEPTH) {
                return BencodeError.MaxDepthExceeded;
            }
            * = _.readByte()?;

            let entries: std.ArrayList(BencodeDictEntry) = .empty;
            errdefer {
                let i: usize = 0;
                while (i < entries.items.len) : (i += 1) {
                    _.alloc.free(entries.items[i].key);
                    entries.items[i].val.deinit(_.alloc);
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
                        return BencodeError.UnsortedDictKeys;
                    }
                }

                const val_start = _.pos;
                let val = _.readValue(depth + 1)?;
                errdefer val.deinit(_.alloc);
                const val_end = _.pos;

                if (_.capture_top_info && depth == 0 && std.mem.eql(u8, key, "info")) {
                    _.info_start = val_start;
                    _.info_end = val_end;
                    _.found_info = true;
                }

                entries.append(_.alloc, BencodeDictEntry{ key: key, val: val }) catch return BencodeError.OutOfMemory;
                prev_key = key;
                has_prev = true;
            }

            const owned_entries = entries.toOwnedSlice(_.alloc) catch return BencodeError.OutOfMemory;
            return BencodeValue.Dict(owned_entries);
        }
        if (b >= '0' && b <= '9') {
            const bs = _.readBytes()?;
            return BencodeValue.Bytes(bs);
        }
        return BencodeError.InvalidStringLength;
    }
}

pub static class BencodeCodec {
    pub fn decode(alloc: std.mem.Allocator, data: []const u8): BencodeDoc ! BencodeError {
        if (data.len == 0) {
            return BencodeError.UnexpectedEof;
        }
        let reader = BencodeReader{
            src: data,
            pos: 0,
            alloc: alloc,
        };
        let root = reader.readValue(0)?;
        if (reader.pos < reader.src.len) {
            root.deinit(alloc);
            return BencodeError.TrailingBytes;
        }
        return BencodeDoc.init(alloc, root);
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
                BencodeCodec.writeBytes(out, alloc, bs)?;
            },
            .List items => {
                if (depth >= BENCODE_MAX_DEPTH) {
                    return BencodeError.MaxDepthExceeded;
                }
                out.append(alloc, 'l') catch return BencodeError.OutOfMemory;
                let i: usize = 0;
                while (i < items.len) : (i += 1) {
                    BencodeCodec.encodeValue(out, alloc, items[i], depth + 1)?;
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
                BencodeCodec.sortEntries(sorted);

                let i: usize = 0;
                while (i < sorted.len) : (i += 1) {
                    BencodeCodec.writeBytes(out, alloc, sorted[i].key)?;
                    BencodeCodec.encodeValue(out, alloc, sorted[i].val, depth + 1)?;
                }
                out.append(alloc, 'e') catch return BencodeError.OutOfMemory;
            },
        }
    }

    pub fn encode(alloc: std.mem.Allocator, val: BencodeValue): ([]u8) ! BencodeError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        BencodeCodec.encodeValue(&out, alloc, val, 0)?;
        return out.toOwnedSlice(alloc) catch return BencodeError.OutOfMemory;
    }

    pub fn encodeDoc(alloc: std.mem.Allocator, doc: BencodeDoc): ([]u8) ! BencodeError {
        return BencodeCodec.encode(alloc, doc.root);
    }
}

pub struct TorrentMetaInfo {
    info_hash: [20]u8 = [0; 20],
    info_hash_hex: [40]u8 = [0; 40],
    announce: []const u8 = "",
    name: []const u8 = "",
    piece_length: u64 = 0,
    pieces_count: usize = 0,
    total_size: u64 = 0,
    alloc: std.mem.Allocator? = null,
    has_announce: bool = false,

    pub fn infoHashHex(): []const u8 {
        return _.info_hash_hex[0..40];
    }

    pub fn deinit(*_) {
        if (a := _.alloc) {
            if (_.has_announce) {
                a.free(_.announce);
            }
            if (_.name.len > 0) {
                a.free(_.name);
            }
            _.alloc = null;
        }
        _.announce = "";
        _.name = "";
        _.has_announce = false;
    }
}

pub static class TorrentParser {
    pub fn parseMetaInfo(alloc: std.mem.Allocator, torrent_bytes: []const u8): TorrentMetaInfo ! BencodeError {
        if (torrent_bytes.len == 0) {
            return BencodeError.UnexpectedEof;
        }
        let reader = BencodeReader{
            src: torrent_bytes,
            pos: 0,
            alloc: alloc,
            capture_top_info: true,
        };
        let root = reader.readValue(0)?;
        let doc = BencodeDoc.init(alloc, root);
        defer doc.deinit();

        if (reader.pos < reader.src.len) {
            return BencodeError.TrailingBytes;
        }
        if (doc.kind() != .Dict || !reader.found_info) {
            return BencodeError.InvalidTorrent;
        }

        const info_val = doc.get("info") ?? return BencodeError.InvalidTorrent;
        if (info_val.kind() != .Dict) {
            return BencodeError.InvalidTorrent;
        }

        let meta = TorrentMetaInfo{
            alloc: alloc,
        };

        const info_raw = torrent_bytes[reader.info_start..reader.info_end];
        sha1Raw(&meta.info_hash, info_raw);
        const hex_arr = std.fmt.bytesToHex(meta.info_hash, .lower);
        @memcpy(&meta.info_hash_hex, hex_arr[0..40]);

        if (ann_val := doc.get("announce")) {
            const ann_bytes = ann_val.asBytes() ?? return BencodeError.InvalidTorrent;
            meta.announce = alloc.dupe(u8, ann_bytes) catch return BencodeError.OutOfMemory;
            meta.has_announce = true;
        }
        errdefer {
            if (meta.has_announce) {
                alloc.free(meta.announce);
            }
        }

        const name_val = info_val.get("name") ?? return BencodeError.InvalidTorrent;
        const name_bytes = name_val.asBytes() ?? return BencodeError.InvalidTorrent;
        if (name_bytes.len == 0) {
            return BencodeError.InvalidTorrent;
        }
        meta.name = alloc.dupe(u8, name_bytes) catch return BencodeError.OutOfMemory;
        errdefer alloc.free(meta.name);

        const pl_val = info_val.get("piece length") ?? return BencodeError.InvalidTorrent;
        const pl_i64 = pl_val.asInt() ?? return BencodeError.InvalidTorrent;
        if (pl_i64 <= 0) {
            return BencodeError.InvalidTorrent;
        }
        meta.piece_length = pl_i64 as u64;

        const pieces_val = info_val.get("pieces") ?? return BencodeError.InvalidTorrent;
        const pieces_bytes = pieces_val.asBytes() ?? return BencodeError.InvalidTorrent;
        if (pieces_bytes.len == 0 || (pieces_bytes.len % 20) != 0) {
            return BencodeError.InvalidTorrent;
        }
        meta.pieces_count = pieces_bytes.len / 20;

        if (len_val := info_val.get("length")) {
            const len_i64 = len_val.asInt() ?? return BencodeError.InvalidTorrent;
            if (len_i64 < 0) {
                return BencodeError.InvalidTorrent;
            }
            meta.total_size = len_i64 as u64;
        } else if (files_val := info_val.get("files")) {
            const file_list = files_val.asList() ?? return BencodeError.InvalidTorrent;
            if (file_list.len == 0) {
                return BencodeError.InvalidTorrent;
            }
            let sum: u64 = 0;
            let i: usize = 0;
            while (i < file_list.len) : (i += 1) {
                const f_dict = file_list[i];
                if (f_dict.kind() != .Dict) {
                    return BencodeError.InvalidTorrent;
                }
                const f_len_val = f_dict.get("length") ?? return BencodeError.InvalidTorrent;
                const f_len_i64 = f_len_val.asInt() ?? return BencodeError.InvalidTorrent;
                if (f_len_i64 < 0) {
                    return BencodeError.InvalidTorrent;
                }
                sum += f_len_i64 as u64;
            }
            meta.total_size = sum;
        } else {
            return BencodeError.InvalidTorrent;
        }

        return meta;
    }
}
