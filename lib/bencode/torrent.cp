// Source: cp/lib/bencode/torrent.cp (Zero-Copy Torrent Section Extractor & SHA-1 InfoHash)
// Extracts raw bencoded sections without AST allocation and computes BEP-3 SHA-1 info_hash.

const std = @import("std");
import { BENCODE_MAX_DEPTH, BencodeError } from "./types.cp";

struct BencodeScanner {
    src: []const u8 = "",
    pos: usize = 0,

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

    fn skipInt(*_) ! BencodeError {
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
        let idx: usize = 0;
        if (num_slice[0] == '-') {
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
        * = std.fmt.parseInt(i64, num_slice, 10) catch return BencodeError.InvalidInteger;
    }

    fn scanBytesSlice(*_): ([]const u8) ! BencodeError {
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
        const slice = _.src[_.pos .. _.pos + byte_len];
        _.pos += byte_len;
        return slice;
    }

    fn skipValue(*_, depth: usize) ! BencodeError {
        const b = _.peekByte() ?? return BencodeError.UnexpectedEof;
        if (b == 'i') {
            _.skipInt()?;
            return;
        }
        if (b == 'l') {
            if (depth >= BENCODE_MAX_DEPTH) {
                return BencodeError.MaxDepthExceeded;
            }
            * = _.readByte()?;
            while (true) {
                const next_b = _.peekByte() ?? return BencodeError.UnexpectedEof;
                if (next_b == 'e') {
                    * = _.readByte()?;
                    break;
                }
                _.skipValue(depth + 1)?;
            }
            return;
        }
        if (b == 'd') {
            if (depth >= BENCODE_MAX_DEPTH) {
                return BencodeError.MaxDepthExceeded;
            }
            * = _.readByte()?;
            let has_prev = false;
            let prev_key: []const u8 = "";
            while (true) {
                const next_b = _.peekByte() ?? return BencodeError.UnexpectedEof;
                if (next_b == 'e') {
                    * = _.readByte()?;
                    break;
                }
                const key = _.scanBytesSlice()?;
                if (has_prev) {
                    const ord = std.mem.order(u8, prev_key, key);
                    if (ord == .eq) {
                        return BencodeError.DuplicateDictKey;
                    }
                    if (ord == .gt) {
                        return BencodeError.UnorderedDictKeys;
                    }
                }
                _.skipValue(depth + 1)?;
                prev_key = key;
                has_prev = true;
            }
            return;
        }
        if (b >= '0' && b <= '9') {
            * = _.scanBytesSlice()?;
            return;
        }
        return BencodeError.InvalidSyntax;
    }
}

pub fn extractRawSection(data: []const u8, dict_key: []const u8): ([]const u8) ! BencodeError {
    if (data.len == 0) {
        return BencodeError.UnexpectedEof;
    }
    let scanner = BencodeScanner{ src: data, pos: 0 };
    const first = scanner.readByte()?;
    if (first != 'd') {
        return BencodeError.InvalidSyntax;
    }

    let has_prev = false;
    let prev_key: []const u8 = "";

    while (true) {
        const next_b = scanner.peekByte() ?? return BencodeError.UnexpectedEof;
        if (next_b == 'e') {
            * = scanner.readByte()?;
            break;
        }
        const key = scanner.scanBytesSlice()?;
        if (has_prev) {
            const ord = std.mem.order(u8, prev_key, key);
            if (ord == .eq) {
                return BencodeError.DuplicateDictKey;
            }
            if (ord == .gt) {
                return BencodeError.UnorderedDictKeys;
            }
        }
        const val_start = scanner.pos;
        scanner.skipValue(1)?;
        const val_end = scanner.pos;

        if (std.mem.eql(u8, key, dict_key)) {
            return data[val_start..val_end];
        }
        prev_key = key;
        has_prev = true;
    }

    return BencodeError.KeyNotFound;
}

pub fn computeInfoHashSha1(torrent_bytes: []const u8): ([20]u8) ! BencodeError {
    const info_raw = extractRawSection(torrent_bytes, "info")?;
    let digest: [20]u8 = [0; 20];
    std.crypto.hash.Sha1.hash(info_raw, &digest, .{});
    return digest;
}

pub fn formatInfoHashHex(digest: [20]u8): [40]u8 {
    return std.fmt.bytesToHex(digest, .lower);
}
