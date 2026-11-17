// Source: UNVERIFIED (origin to be confirmed: Bun / Zig std / custom)
// lib/http/h2/hpack.cp
// RFC 7541 HPACK Header Compression for HTTP/2
// Prefix integer codec, static table lookup, and header block encoder/decoder

const std = @import("std");

pub struct HpackIntResult {
    pub value: usize = 0;
    pub bytes_read: usize = 0;
}

pub struct HpackStaticEntry {
    pub index: usize = 0;
    pub name: []const u8 = "";
    pub value: []const u8 = "";
}

pub struct HpackDecodedHeader {
    pub name: []const u8 = "";
    pub value: []const u8 = "";
    pub never_index: bool = false;
    pub bytes_read: usize = 0;
}

pub struct HpackCodec {
    /// RFC 7541 §5.1 Integer Representation Encoder.
    /// Writes `value` with `prefix_bits` (1..8) and `first_byte_mask` into `out`.
    pub static fn encodePrefixInt(out: []u8, value: usize, prefix_bits: u8, first_byte_mask: u8): usize {
        if (out.len == 0 || prefix_bits == 0 || prefix_bits > 8) {
            return 0;
        }
        const max_prefix: usize = ((1 as usize) << prefix_bits) - 1;
        if (value < max_prefix) {
            out[0] = first_byte_mask | (value as u8);
            return 1;
        }
        out[0] = first_byte_mask | (max_prefix as u8);
        let rem = value - max_prefix;
        let pos: usize = 1;
        while (rem >= 128) {
            if (pos >= out.len) { return 0; }
            out[pos] = ((rem & 0x7F) as u8) | 0x80;
            pos += 1;
            rem = rem >> 7;
        }
        if (pos >= out.len) { return 0; }
        out[pos] = (rem & 0x7F) as u8;
        return pos + 1;
    }

    /// RFC 7541 §5.1 Integer Representation Decoder.
    pub static fn decodePrefixInt(buf: []const u8, prefix_bits: u8): HpackIntResult? {
        if (buf.len == 0 || prefix_bits == 0 || prefix_bits > 8) {
            return null;
        }
        const max_prefix: usize = ((1 as usize) << prefix_bits) - 1;
        let value: usize = (buf[0] as usize) & max_prefix;
        if (value < max_prefix) {
            return HpackIntResult {
                value: value,
                bytes_read: 1,
            };
        }
        let shift: usize = 0;
        let pos: usize = 1;
        while (pos < buf.len) {
            const b = buf[pos];
            pos += 1;
            value += ((b & 0x7F) as usize) << shift;
            if ((b & 0x80) == 0) {
                return HpackIntResult {
                    value: value,
                    bytes_read: pos,
                };
            }
            if (shift >= 56) {
                return null;
            }
            shift += 7;
        }
        return null;
    }

    /// RFC 7541 Appendix A Static Table Lookup (1-based index for core HTTP/2 pseudo & standard headers).
    pub static fn lookupStatic(index: usize): HpackStaticEntry? {
        if (index == 1) { return HpackStaticEntry { index: 1, name: ":authority", value: "" }; }
        if (index == 2) { return HpackStaticEntry { index: 2, name: ":method", value: "GET" }; }
        if (index == 3) { return HpackStaticEntry { index: 3, name: ":method", value: "POST" }; }
        if (index == 4) { return HpackStaticEntry { index: 4, name: ":path", value: "/" }; }
        if (index == 5) { return HpackStaticEntry { index: 5, name: ":path", value: "/index.html" }; }
        if (index == 6) { return HpackStaticEntry { index: 6, name: ":scheme", value: "http" }; }
        if (index == 7) { return HpackStaticEntry { index: 7, name: ":scheme", value: "https" }; }
        if (index == 8) { return HpackStaticEntry { index: 8, name: ":status", value: "200" }; }
        if (index == 9) { return HpackStaticEntry { index: 9, name: ":status", value: "204" }; }
        if (index == 10) { return HpackStaticEntry { index: 10, name: ":status", value: "206" }; }
        if (index == 11) { return HpackStaticEntry { index: 11, name: ":status", value: "304" }; }
        if (index == 12) { return HpackStaticEntry { index: 12, name: ":status", value: "400" }; }
        if (index == 13) { return HpackStaticEntry { index: 13, name: ":status", value: "404" }; }
        if (index == 14) { return HpackStaticEntry { index: 14, name: ":status", value: "500" }; }
        if (index == 16) { return HpackStaticEntry { index: 16, name: "accept-encoding", value: "gzip, deflate" }; }
        if (index == 28) { return HpackStaticEntry { index: 28, name: "content-length", value: "" }; }
        if (index == 31) { return HpackStaticEntry { index: 31, name: "content-type", value: "" }; }
        if (index == 38) { return HpackStaticEntry { index: 38, name: "host", value: "" }; }
        if (index == 54) { return HpackStaticEntry { index: 54, name: "server", value: "" }; }
        if (index == 58) { return HpackStaticEntry { index: 58, name: "user-agent", value: "" }; }
        return null;
    }

    /// Finds an exact (name + value) match in the RFC 7541 static table, returning 0 if not found.
    pub static fn findStaticExact(name: []const u8, value: []const u8): usize {
        if (std.mem.eql(u8, name, ":method")) {
            if (std.mem.eql(u8, value, "GET")) { return 2; }
            if (std.mem.eql(u8, value, "POST")) { return 3; }
        }
        if (std.mem.eql(u8, name, ":path")) {
            if (std.mem.eql(u8, value, "/")) { return 4; }
            if (std.mem.eql(u8, value, "/index.html")) { return 5; }
        }
        if (std.mem.eql(u8, name, ":scheme")) {
            if (std.mem.eql(u8, value, "http")) { return 6; }
            if (std.mem.eql(u8, value, "https")) { return 7; }
        }
        if (std.mem.eql(u8, name, ":status")) {
            if (std.mem.eql(u8, value, "200")) { return 8; }
            if (std.mem.eql(u8, value, "204")) { return 9; }
            if (std.mem.eql(u8, value, "206")) { return 10; }
            if (std.mem.eql(u8, value, "304")) { return 11; }
            if (std.mem.eql(u8, value, "400")) { return 12; }
            if (std.mem.eql(u8, value, "404")) { return 13; }
            if (std.mem.eql(u8, value, "500")) { return 14; }
        }
        if (std.mem.eql(u8, name, "accept-encoding") && std.mem.eql(u8, value, "gzip, deflate")) {
            return 16;
        }
        return 0;
    }

    /// Encodes a non-Huffman string literal (7-bit length prefix + raw octets).
    pub static fn encodeStringLiteral(out: []u8, str: []const u8): usize {
        const hdr_len = HpackCodec.encodePrefixInt(out, str.len, 7, 0x00);
        if (hdr_len == 0 || out.len < hdr_len + str.len) {
            return 0;
        }
        let i: usize = 0;
        while (i < str.len) {
            out[hdr_len + i] = str[i];
            i += 1;
        }
        return hdr_len + str.len;
    }

    /// Decodes a non-Huffman string literal.
    pub static fn decodeStringLiteral(buf: []const u8): HpackDecodedHeader? {
        if (buf.len == 0) { return null; }
        const len_res_opt = HpackCodec.decodePrefixInt(buf, 7);
        if (len_res_opt == null) { return null; }
        const len_res = len_res_opt.?;
        const total = len_res.bytes_read + len_res.value;
        if (buf.len < total) { return null; }
        return HpackDecodedHeader {
            name: "",
            value: buf[len_res.bytes_read .. total],
            never_index: false,
            bytes_read: total,
        };
    }

    /// Encodes an Indexed Header Field Representation (`0x80` bit mask, 7-bit prefix).
    pub static fn encodeIndexed(out: []u8, index: usize): usize {
        if (index == 0) { return 0; }
        return HpackCodec.encodePrefixInt(out, index, 7, 0x80);
    }

    /// Encodes a header field, automatically using an indexed representation if `(name, value)` is in the static table,
    /// or a Literal Header Field without Indexing (`0x00` / `0x10` never-indexed) otherwise.
    pub static fn encodeHeader(out: []u8, name: []const u8, value: []const u8, never_index: bool): usize {
        if (!never_index) {
            const exact_idx = HpackCodec.findStaticExact(name, value);
            if (exact_idx > 0) {
                return HpackCodec.encodeIndexed(out, exact_idx);
            }
        }
        if (out.len < 3) { return 0; }
        out[0] = if (never_index) (0x10 as u8) else (0x00 as u8);
        let pos: usize = 1;
        const n_len = HpackCodec.encodeStringLiteral(out[pos..], name);
        if (n_len == 0) { return 0; }
        pos += n_len;
        const v_len = HpackCodec.encodeStringLiteral(out[pos..], value);
        if (v_len == 0) { return 0; }
        return pos + v_len;
    }

    /// Decodes a single HPACK header field representation from `buf`.
    pub static fn decodeHeader(buf: []const u8): HpackDecodedHeader? {
        if (buf.len == 0) { return null; }
        const first = buf[0];

        // 1. Indexed Header Field (bit 7 == 1)
        if ((first & 0x80) != 0) {
            const idx_res_opt = HpackCodec.decodePrefixInt(buf, 7);
            if (idx_res_opt == null) { return null; }
            const idx_res = idx_res_opt.?;
            const entry_opt = HpackCodec.lookupStatic(idx_res.value);
            if (entry_opt == null) { return null; }
            const entry = entry_opt.?;
            return HpackDecodedHeader {
                name: entry.name,
                value: entry.value,
                never_index: false,
                bytes_read: idx_res.bytes_read,
            };
        }

        // 2. Literal Header Field with New Name (`0x00` without indexing, `0x10` never indexed, `0x40` incremental indexing)
        const never_idx = (first & 0xF0) == 0x10;
        const prefix_bits: u8 = if ((first & 0x40) != 0) (6 as u8) else (4 as u8);
        const name_idx_opt = HpackCodec.decodePrefixInt(buf, prefix_bits);
        if (name_idx_opt == null) { return null; }
        const name_idx = name_idx_opt.?;
        let pos: usize = name_idx.bytes_read;

        let header_name: []const u8 = "";
        if (name_idx.value > 0) {
            const st_opt = HpackCodec.lookupStatic(name_idx.value);
            if (st_opt == null) { return null; }
            header_name = st_opt.?.name;
        } else {
            const name_str_opt = HpackCodec.decodeStringLiteral(buf[pos..]);
            if (name_str_opt == null) { return null; }
            const name_str = name_str_opt.?;
            header_name = name_str.value;
            pos += name_str.bytes_read;
        }

        const val_str_opt = HpackCodec.decodeStringLiteral(buf[pos..]);
        if (val_str_opt == null) { return null; }
        const val_str = val_str_opt.?;
        pos += val_str.bytes_read;

        return HpackDecodedHeader {
            name: header_name,
            value: val_str.value,
            never_index: never_idx,
            bytes_read: pos,
        };
    }
}
