// Source: /data/dev/bun/src/string/wtf.cp
// lib/encoding/utf16be.cp
// UTF-16BE & ASCII / Latin-1 fast codecs for cap:encoding

const std = @import("std");
import { Utf8, REPLACEMENT_CHAR } from "../string/utf.cp";

pub struct Utf16BE {
    pub static fn encodeUtf8ToUtf16Be(input: []const u8, out: []u8): usize {
        let pos: usize = 0;
        let out_pos: usize = 0;
        while (pos < input.len) {
            const dec_res = Utf8.decodeNext(input, pos);
            const cp = dec_res.codepoint;
            if (cp < 0x10000) {
                if (out_pos + 2 > out.len) break;
                out[out_pos] = ((cp >> 8) & 0xFF) as u8;
                out[out_pos + 1] = (cp & 0xFF) as u8;
                out_pos += 2;
            } else {
                if (out_pos + 4 > out.len) break;
                const adj = cp - 0x10000;
                const hi: u16 = (0xD800 + (adj >> 10)) as u16;
                const lo: u16 = (0xDC00 + (adj & 0x3FF)) as u16;
                out[out_pos] = ((hi >> 8) & 0xFF) as u8;
                out[out_pos + 1] = (hi & 0xFF) as u8;
                out[out_pos + 2] = ((lo >> 8) & 0xFF) as u8;
                out[out_pos + 3] = (lo & 0xFF) as u8;
                out_pos += 4;
            }
            pos += (dec_res.bytes_consumed as usize);
        }
        return out_pos;
    }

    pub static fn decodeUtf16BeToUtf8(input: []const u8, out: []u8): usize {
        let i: usize = 0;
        let out_pos: usize = 0;
        while (i + 1 < input.len) {
            const unit0: u32 = ((input[i] as u32) << 8) | (input[i + 1] as u32);
            let cp: u32 = unit0;
            if (unit0 >= 0xD800 && unit0 <= 0xDBFF) {
                if (i + 3 < input.len) {
                    const unit1: u32 = ((input[i + 2] as u32) << 8) | (input[i + 3] as u32);
                    if (unit1 >= 0xDC00 && unit1 <= 0xDFFF) {
                        cp = (0x10000 as u32) + ((unit0 - 0xD800) << 10) + (unit1 - 0xDC00);
                        i += 2;
                    } else {
                        cp = REPLACEMENT_CHAR;
                    }
                } else {
                    cp = REPLACEMENT_CHAR;
                }
            } else if (unit0 >= 0xDC00 && unit0 <= 0xDFFF) {
                cp = REPLACEMENT_CHAR;
            }
            const written = Utf8.encodeCodepoint(cp, out[out_pos..out.len]);
            if (written == 0) break;
            out_pos += written;
            i += 2;
        }
        return out_pos;
    }
}

pub struct Ascii {
    pub static fn isAscii(input: []const u8): bool {
        let i: usize = 0;
        while (i < input.len) {
            if (input[i] >= 0x80) return false;
            i += 1;
        }
        return true;
    }

    pub static fn toLower(input: []const u8, out: []u8): usize {
        const n = if (input.len < out.len) input.len else out.len;
        let i: usize = 0;
        while (i < n) {
            const c = input[i];
            out[i] = if (c >= 'A' && c <= 'Z') c + 32 else c;
            i += 1;
        }
        return n;
    }

    pub static fn toUpper(input: []const u8, out: []u8): usize {
        const n = if (input.len < out.len) input.len else out.len;
        let i: usize = 0;
        while (i < n) {
            const c = input[i];
            out[i] = if (c >= 'a' && c <= 'z') c - 32 else c;
            i += 1;
        }
        return n;
    }

    pub static fn latin1ToUtf8(input: []const u8, out: []u8): usize {
        let i: usize = 0;
        let pos: usize = 0;
        while (i < input.len) {
            const b = input[i];
            if (b < 0x80) {
                if (pos + 1 > out.len) break;
                out[pos] = b;
                pos += 1;
            } else {
                if (pos + 2 > out.len) break;
                out[pos] = (0xC0 as u8) | (b >> 6);
                out[pos + 1] = (0x80 as u8) | (b & 0x3F);
                pos += 2;
            }
            i += 1;
        }
        return pos;
    }
}
