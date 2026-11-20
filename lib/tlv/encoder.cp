// Source: cp/lib/tlv/encoder.cp (ASN.1 DER TLV Node Encoder & Streaming TlvWriter)
// Pure CP zero-C-dependency canonical DER encoder with nested container builder and minimal integer normalization.

const std = @import("std");
import {
    TLV_MAX_DEPTH,
    TAG_NUM_BOOLEAN,
    TAG_NUM_INTEGER,
    TAG_NUM_OCTET_STRING,
    TAG_NUM_NULL,
    TAG_NUM_OID,
    TAG_NUM_UTF8_STRING,
    TAG_NUM_SEQUENCE,
    TAG_NUM_SET,
    TlvError,
    TlvTag,
    TlvNode,
    ByteBuf,
    derLengthSize,
    encodeDerLengthTo,
} from "./types.cp";
import { encodeOidBytes } from "./oid.cp";

fn appendTagAndLength(buf: *ByteBuf, tag: TlvTag, value_len: usize): ! TlvError {
    const tag_len = tag.encodedLen();
    const len_len = derLengthSize(value_len);
    buf.ensureUnused(tag_len + len_len)?;
    const w_tag = tag.encodeTo(buf.data[buf.len .. buf.len + tag_len])?;
    buf.len += w_tag;
    const w_len = encodeDerLengthTo(buf.data[buf.len .. buf.len + len_len], value_len)?;
    buf.len += w_len;
}

fn encodeNodeInto(alloc: std.mem.Allocator, buf: *ByteBuf, node: TlvNode): ! TlvError {
    if (node.tag.constructed && node.children.len > 0) {
        let child_buf = ByteBuf.init(alloc);
        defer child_buf.deinit();

        let i: usize = 0;
        while (i < node.children.len) : (i += 1) {
            encodeNodeInto(alloc, &child_buf, node.children[i])?;
        }
        appendTagAndLength(buf, node.tag, child_buf.len)?;
        buf.appendSlice(child_buf.data[0 .. child_buf.len])?;
        return;
    }

    appendTagAndLength(buf, node.tag, node.raw_value.len)?;
    buf.appendSlice(node.raw_value)?;
}

/// Recursively DER-encodes a `TlvNode` tree into an owned byte slice.
/// Caller owns the returned slice and must free it with `alloc.free`.
pub fn encodeDerNode(alloc: std.mem.Allocator, node: TlvNode): ([]u8) ! TlvError {
    let buf = ByteBuf.init(alloc);
    errdefer buf.deinit();

    encodeNodeInto(alloc, &buf, node)?;
    return buf.toOwnedSlice();
}

/// Streaming ASN.1 DER TLV writer with nested `SEQUENCE`/`SET`/constructed container support.
pub struct TlvWriter {
    buf: ByteBuf = ByteBuf{},
    stack: [TLV_MAX_DEPTH]usize = [0; TLV_MAX_DEPTH],
    depth: usize = 0,

    pub static fn init(alloc: std.mem.Allocator): TlvWriter {
        return TlvWriter{
            buf: ByteBuf.init(alloc),
            stack: [0; TLV_MAX_DEPTH],
            depth: 0,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit();
        _.depth = 0;
    }

    pub fn reset(*_) {
        _.buf.len = 0;
        _.depth = 0;
    }

    /// Returns a read-only view of the encoded bytes so far.
    pub fn bytes(): []const u8 {
        return _.buf.data[0 .. _.buf.len];
    }

    /// Transfers ownership of the encoded DER bytes to the caller (resets writer).
    pub fn toOwnedSlice(*_): ([]u8) ! TlvError {
        if (_.depth != 0) {
            return TlvError.InvalidLength;
        }
        return _.buf.toOwnedSlice();
    }

    /// Writes a raw TLV element with explicit `tag` and `value` octets.
    pub fn writeRaw(*_, tag: TlvTag, value: []const u8): ! TlvError {
        appendTagAndLength(&_.buf, tag, value.len)?;
        _.buf.appendSlice(value)?;
    }

    /// Writes a canonical DER BOOLEAN (`0x00` for false, `0xFF` for true).
    pub fn writeBoolean(*_, val: bool): ! TlvError {
        const tag = TlvTag.universal(TAG_NUM_BOOLEAN, false);
        const b: u8 = if (val) 0xFF else 0x00;
        appendTagAndLength(&_.buf, tag, 1)?;
        _.buf.appendByte(b)?;
    }

    /// Writes a canonical DER NULL (`05 00`).
    pub fn writeNull(*_): ! TlvError {
        const tag = TlvTag.universal(TAG_NUM_NULL, false);
        appendTagAndLength(&_.buf, tag, 0)?;
    }

    /// Writes a canonical DER INTEGER from big-endian two's-complement bytes,
    /// stripping redundant leading `0x00` or `0xFF` octets. Empty input encodes as `0x00`.
    pub fn writeIntegerBytes(*_, raw: []const u8): ! TlvError {
        const tag = TlvTag.universal(TAG_NUM_INTEGER, false);
        if (raw.len == 0) {
            appendTagAndLength(&_.buf, tag, 1)?;
            _.buf.appendByte(0x00)?;
            return;
        }
        let start: usize = 0;
        while (start + 1 < raw.len) {
            const b0 = raw[start];
            const b1 = raw[start + 1];
            if ((b0 == 0x00 && (b1 & 0x80) == 0) || (b0 == 0xFF && (b1 & 0x80) != 0)) {
                start += 1;
            } else {
                break;
            }
        }
        const trimmed = raw[start ..];
        appendTagAndLength(&_.buf, tag, trimmed.len)?;
        _.buf.appendSlice(trimmed)?;
    }

    /// Writes a canonical DER non-negative INTEGER from unsigned big-endian magnitude bytes,
    /// stripping redundant leading `0x00`s and prepending `0x00` if the MSB is set.
    pub fn writeUnsignedIntegerBytes(*_, raw: []const u8): ! TlvError {
        const tag = TlvTag.universal(TAG_NUM_INTEGER, false);
        let start: usize = 0;
        while (start < raw.len && raw[start] == 0x00) {
            start += 1;
        }
        const trimmed = raw[start ..];
        if (trimmed.len == 0) {
            appendTagAndLength(&_.buf, tag, 1)?;
            _.buf.appendByte(0x00)?;
            return;
        }
        const needs_zero_prefix = (trimmed[0] & 0x80) != 0;
        const total_len = trimmed.len + (if (needs_zero_prefix) 1 else 0);
        appendTagAndLength(&_.buf, tag, total_len)?;
        if (needs_zero_prefix) {
            _.buf.appendByte(0x00)?;
        }
        _.buf.appendSlice(trimmed)?;
    }

    /// Writes a canonical DER OCTET STRING.
    pub fn writeOctetString(*_, data: []const u8): ! TlvError {
        const tag = TlvTag.universal(TAG_NUM_OCTET_STRING, false);
        _.writeRaw(tag, data)?;
    }

    /// Writes a canonical DER UTF8String.
    pub fn writeUtf8String(*_, s: []const u8): ! TlvError {
        const tag = TlvTag.universal(TAG_NUM_UTF8_STRING, false);
        _.writeRaw(tag, s)?;
    }

    /// Encodes a dotted OID string (e.g. `"1.2.840.113549.1.1.11"`) and writes a DER OID element.
    pub fn writeOid(*_, oid_str: []const u8): ! TlvError {
        const encoded = encodeOidBytes(_.buf.alloc, oid_str)?;
        defer _.buf.alloc.free(encoded);
        const tag = TlvTag.universal(TAG_NUM_OID, false);
        _.writeRaw(tag, encoded)?;
    }

    /// Begins a constructed TLV container with the given `tag` (forces `constructed = true`).
    pub fn beginConstructed(*_, tag: TlvTag): ! TlvError {
        if (_.depth >= TLV_MAX_DEPTH) {
            return TlvError.MaxDepthExceeded;
        }
        const ctag = TlvTag.init(tag.tag_class, true, tag.number);
        const tag_len = ctag.encodedLen();
        _.buf.ensureUnused(tag_len)?;
        const w_tag = ctag.encodeTo(_.buf.data[_.buf.len .. _.buf.len + tag_len])?;
        _.buf.len += w_tag;
        _.stack[_.depth] = _.buf.len;
        _.depth += 1;
    }

    /// Begins a Universal constructed `SEQUENCE` (`0x30`).
    pub fn beginSequence(*_): ! TlvError {
        _.beginConstructed(TlvTag.universal(TAG_NUM_SEQUENCE, true))?;
    }

    /// Begins a Universal constructed `SET` (`0x31`).
    pub fn beginSet(*_): ! TlvError {
        _.beginConstructed(TlvTag.universal(TAG_NUM_SET, true))?;
    }

    /// Closes the most recently opened constructed container and patches its canonical DER length.
    pub fn endConstructed(*_): ! TlvError {
        if (_.depth == 0) {
            return TlvError.InvalidLength;
        }
        _.depth -= 1;
        const content_start = _.stack[_.depth];
        const content_len = _.buf.len - content_start;
        const len_bytes = derLengthSize(content_len);

        _.buf.ensureUnused(len_bytes)?;
        if (content_len > 0) {
            let i: usize = content_len;
            while (i > 0) {
                i -= 1;
                _.buf.data[content_start + len_bytes + i] = _.buf.data[content_start + i];
            }
        }
        const w_len = encodeDerLengthTo(_.buf.data[content_start .. content_start + len_bytes], content_len)?;
        _.buf.len += w_len;
    }
}
