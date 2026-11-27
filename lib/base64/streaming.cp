// Source: cp/lib/base64/streaming.cp (Incremental streaming Base64 encoder and decoder)

const std = @import("std");
import {
    Base64Variant,
    Base64Error,
    PAD_CHAR,
    INVALID_INDEX,
    hasPadding,
    alphabetFor,
    decodeTableFor,
    isWhitespace,
} from "./types.cp";
import { encodedLen, decodeTailQuantum } from "./codec.cp";

/// Incremental chunk-by-chunk Base64 encoder supporting arbitrary 1..N byte chunk boundaries.
pub struct StreamingBase64Encoder {
    variant: Base64Variant,
    buf: [3]u8,
    buf_len: usize,
    finished: bool,

    /// Creates a new streaming encoder for `variant`.
    pub static fn init(variant: Base64Variant): StreamingBase64Encoder {
        return StreamingBase64Encoder {
            variant: variant,
            buf: [0; 3],
            buf_len: 0,
            finished: false,
        };
    }

    /// Returns the maximum number of bytes `update` may write for `chunk_len`.
    pub fn maxUpdateLen(chunk_len: usize): usize {
        return ((_.buf_len + chunk_len) / 3) * 4;
    }

    /// Returns the exact number of bytes `finish` will write.
    pub fn finishLen(): usize {
        return encodedLen(_.buf_len, _.variant);
    }

    /// Feeds `chunk` into the encoder, writing complete 4-byte quanta to `dst`.
    pub fn update(*_, dst: []u8, chunk: []const u8): Base64Error![]u8 {
        if (_.finished) {
            return err.InvalidLength;
        }
        let max_out = _.maxUpdateLen(chunk.len);
        if (dst.len < max_out) {
            return err.DestinationTooSmall;
        }
        if (chunk.len == 0) {
            return dst[0..0];
        }

        let alphabet = alphabetFor(_.variant);
        let in_idx: usize = 0;
        let out_idx: usize = 0;

        if (_.buf_len > 0) {
            while (_.buf_len < 3 && in_idx < chunk.len) {
                _.buf[_.buf_len] = chunk[in_idx];
                _.buf_len += 1;
                in_idx += 1;
            }
            if (_.buf_len == 3) {
                let b0 = _.buf[0] as u32;
                let b1 = _.buf[1] as u32;
                let b2 = _.buf[2] as u32;
                let triple = (b0 << 16) | (b1 << 8) | b2;
                dst[out_idx] = alphabet[((triple >> 18) & 0x3F) as usize];
                dst[out_idx + 1] = alphabet[((triple >> 12) & 0x3F) as usize];
                dst[out_idx + 2] = alphabet[((triple >> 6) & 0x3F) as usize];
                dst[out_idx + 3] = alphabet[(triple & 0x3F) as usize];
                out_idx += 4;
                _.buf_len = 0;
            }
        }

        while (in_idx + 3 <= chunk.len) {
            let b0 = chunk[in_idx] as u32;
            let b1 = chunk[in_idx + 1] as u32;
            let b2 = chunk[in_idx + 2] as u32;
            let triple = (b0 << 16) | (b1 << 8) | b2;
            dst[out_idx] = alphabet[((triple >> 18) & 0x3F) as usize];
            dst[out_idx + 1] = alphabet[((triple >> 12) & 0x3F) as usize];
            dst[out_idx + 2] = alphabet[((triple >> 6) & 0x3F) as usize];
            dst[out_idx + 3] = alphabet[(triple & 0x3F) as usize];
            in_idx += 3;
            out_idx += 4;
        }

        while (in_idx < chunk.len) {
            _.buf[_.buf_len] = chunk[in_idx];
            _.buf_len += 1;
            in_idx += 1;
        }

        return dst[0..out_idx];
    }

    /// Flushes any remaining buffered bytes (1 or 2 bytes) and optional padding to `dst`.
    pub fn finish(*_, dst: []u8): Base64Error![]u8 {
        if (_.finished) {
            return dst[0..0];
        }
        let needed = _.finishLen();
        if (dst.len < needed) {
            return err.DestinationTooSmall;
        }
        _.finished = true;
        if (_.buf_len == 0) {
            return dst[0..0];
        }

        let alphabet = alphabetFor(_.variant);
        let pad = hasPadding(_.variant);
        let out_idx: usize = 0;

        if (_.buf_len == 1) {
            let b0 = _.buf[0] as u32;
            dst[out_idx] = alphabet[((b0 >> 2) & 0x3F) as usize];
            dst[out_idx + 1] = alphabet[((b0 << 4) & 0x3F) as usize];
            out_idx += 2;
            if (pad) {
                dst[out_idx] = PAD_CHAR;
                dst[out_idx + 1] = PAD_CHAR;
                out_idx += 2;
            }
        } else if (_.buf_len == 2) {
            let b0 = _.buf[0] as u32;
            let b1 = _.buf[1] as u32;
            let pair = (b0 << 8) | b1;
            dst[out_idx] = alphabet[((pair >> 10) & 0x3F) as usize];
            dst[out_idx + 1] = alphabet[((pair >> 4) & 0x3F) as usize];
            dst[out_idx + 2] = alphabet[((pair << 2) & 0x3F) as usize];
            out_idx += 3;
            if (pad) {
                dst[out_idx] = PAD_CHAR;
                out_idx += 1;
            }
        }

        _.buf_len = 0;
        return dst[0..out_idx];
    }

    /// Convenience helper appending encoded bytes for `chunk` directly into `out`.
    pub fn updateToList(*_, alloc: std.mem.Allocator, out: *std.ArrayList(u8), chunk: []const u8): Base64Error!void {
        let max_out = _.maxUpdateLen(chunk.len);
        if (max_out == 0) {
            * = _.update(out.items[0..0], chunk)?;
            return;
        }
        let start = out.items.len;
        out.resize(alloc, start + max_out) catch return err.OutOfMemory;
        let written = _.update(out.items[start..], chunk)?;
        out.items.len = start + written.len;
    }

    /// Convenience helper appending final encoded bytes directly into `out`.
    pub fn finishToList(*_, alloc: std.mem.Allocator, out: *std.ArrayList(u8)): Base64Error!void {
        let needed = _.finishLen();
        if (needed == 0) {
            _.finished = true;
            return;
        }
        let start = out.items.len;
        out.resize(alloc, start + needed) catch return err.OutOfMemory;
        let written = _.finish(out.items[start..])?;
        out.items.len = start + written.len;
    }
}

/// Incremental chunk-by-chunk Base64 decoder supporting arbitrary 1..N byte chunk boundaries.
pub struct StreamingBase64Decoder {
    variant: Base64Variant,
    ignore_whitespace: bool,
    q: [4]u8,
    q_len: usize,
    pad_count: usize,
    saw_pad: bool,
    finished: bool,

    /// Creates a new strict streaming decoder for `variant`.
    pub static fn init(variant: Base64Variant): StreamingBase64Decoder {
        return StreamingBase64Decoder {
            variant: variant,
            ignore_whitespace: false,
            q: [0; 4],
            q_len: 0,
            pad_count: 0,
            saw_pad: false,
            finished: false,
        };
    }

    /// Creates a new streaming decoder that ignores ASCII whitespace (` `, `\r`, `\n`, `\t`).
    pub fn initIgnoreWhitespace(variant: Base64Variant): StreamingBase64Decoder {
        return StreamingBase64Decoder {
            variant: variant,
            ignore_whitespace: true,
            q: [0; 4],
            q_len: 0,
            pad_count: 0,
            saw_pad: false,
            finished: false,
        };
    }

    /// Returns the maximum number of decoded bytes `update` may write for `chunk_len`.
    pub fn maxUpdateLen(chunk_len: usize): usize {
        return ((_.q_len + chunk_len) / 4) * 3;
    }

    /// Returns the maximum number of decoded bytes `finish` may write (at most 2).
    pub fn maxFinishLen(): usize {
        if (_.q_len >= 2) {
            return _.q_len - 1;
        }
        return 0;
    }

    /// Feeds `chunk` into the decoder, writing complete 3-byte quanta to `dst`.
    pub fn update(*_, dst: []u8, chunk: []const u8): Base64Error![]u8 {
        if (_.finished) {
            return err.InvalidLength;
        }
        let table = decodeTableFor(_.variant);
        let pad = hasPadding(_.variant);
        let out_idx: usize = 0;

        for (c in chunk) {
            if (_.ignore_whitespace && isWhitespace(c)) {
                continue;
            }
            if (c == PAD_CHAR) {
                if (!pad) {
                    return err.InvalidPadding;
                }
                if (!_.saw_pad) {
                    if (_.q_len < 2) {
                        return err.InvalidPadding;
                    }
                    _.saw_pad = true;
                }
                _.pad_count += 1;
                if (_.q_len + _.pad_count > 4) {
                    return err.InvalidPadding;
                }
                continue;
            }
            if (_.saw_pad) {
                return err.InvalidPadding;
            }
            let d = table[c as usize];
            if (d == INVALID_INDEX) {
                return err.InvalidCharacter;
            }
            _.q[_.q_len] = d;
            _.q_len += 1;
            if (_.q_len == 4) {
                out_idx = decodeTailQuantum(dst, out_idx, _.q[0], _.q[1], _.q[2], true, _.q[3], true)?;
                _.q_len = 0;
            }
        }

        return dst[0..out_idx];
    }

    /// Finalizes decoding, validating trailing padding/length and RFC 4648 §3.5 zero bits.
    pub fn finish(*_, dst: []u8): Base64Error![]u8 {
        if (_.finished) {
            return dst[0..0];
        }
        _.finished = true;
        let pad = hasPadding(_.variant);
        let out_idx: usize = 0;

        if (_.saw_pad) {
            if (_.q_len + _.pad_count != 4) {
                return err.InvalidPadding;
            }
            if (_.q_len == 2) {
                out_idx = decodeTailQuantum(dst, out_idx, _.q[0], _.q[1], 0, false, 0, false)?;
                _.q_len = 0;
                _.pad_count = 0;
                return dst[0..out_idx];
            }
            if (_.q_len == 3) {
                out_idx = decodeTailQuantum(dst, out_idx, _.q[0], _.q[1], _.q[2], true, 0, false)?;
                _.q_len = 0;
                _.pad_count = 0;
                return dst[0..out_idx];
            }
            return err.InvalidPadding;
        }

        if (_.q_len == 0) {
            return dst[0..0];
        }
        if (_.q_len == 1) {
            return err.InvalidLength;
        }
        if (pad) {
            return err.InvalidLength;
        }
        if (_.q_len == 2) {
            out_idx = decodeTailQuantum(dst, out_idx, _.q[0], _.q[1], 0, false, 0, false)?;
        } else {
            out_idx = decodeTailQuantum(dst, out_idx, _.q[0], _.q[1], _.q[2], true, 0, false)?;
        }
        _.q_len = 0;
        return dst[0..out_idx];
    }

    /// Convenience helper appending decoded bytes for `chunk` directly into `out`.
    pub fn updateToList(*_, alloc: std.mem.Allocator, out: *std.ArrayList(u8), chunk: []const u8): Base64Error!void {
        let max_out = _.maxUpdateLen(chunk.len);
        if (max_out == 0) {
            * = _.update(out.items[0..0], chunk)?;
            return;
        }
        let start = out.items.len;
        out.resize(alloc, start + max_out) catch return err.OutOfMemory;
        let written = _.update(out.items[start..], chunk)?;
        out.items.len = start + written.len;
    }

    /// Convenience helper appending final decoded bytes directly into `out`.
    pub fn finishToList(*_, alloc: std.mem.Allocator, out: *std.ArrayList(u8)): Base64Error!void {
        let max_out = _.maxFinishLen();
        if (max_out == 0) {
            * = _.finish(out.items[0..0])?;
            return;
        }
        let start = out.items.len;
        out.resize(alloc, start + max_out) catch return err.OutOfMemory;
        let written = _.finish(out.items[start..])?;
        out.items.len = start + written.len;
    }
}
