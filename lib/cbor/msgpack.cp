// Source: cp/lib/cbor/msgpack.cp (MessagePack Binary Encoder & Decoder)
// Pure CP MessagePack binary serialization & deserialization engine supporting
// positive/negative fixint, uint8/16/32/64, int8/16/32/64, nil, bool, float32/64,
// fixstr/str8/16/32, bin8/16/32, fixarray/array16/32, fixmap/map16/32,
// max nesting depth 64 guard, and strict EOF / trailing-bytes validation.

const std = @import("std");
import { BinError, BinMapEntry, BinValue, BinDocument } from "./value.cp";

pub const MSGPACK_MAX_DEPTH: usize = 64;

struct MsgPackReader {
    src: []const u8 = "",
    pos: usize = 0,
    alloc: std.mem.Allocator = undefined,

    fn remaining(): usize {
        if (_.pos >= _.src.len) return 0;
        return _.src.len - _.pos;
    }

    fn readByte(*_): u8! {
        if (_.pos >= _.src.len) {
            return err.UnexpectedEof;
        }
        const b = _.src[_.pos];
        _.pos += 1;
        return b;
    }

    fn readSlice(*_, n: usize): ([]const u8)! {
        if (n > _.remaining()) {
            return err.UnexpectedEof;
        }
        const s = _.src[_.pos .. _.pos + n];
        _.pos += n;
        return s;
    }

    fn readUint16(*_): u16! {
        const s = _.readSlice(2)?;
        return ((s[0] as u16) << 8) | (s[1] as u16);
    }

    fn readUint32(*_): u32! {
        const s = _.readSlice(4)?;
        return ((s[0] as u32) << 24) |
            ((s[1] as u32) << 16) |
            ((s[2] as u32) << 8) |
            (s[3] as u32);
    }

    fn readUint64(*_): u64! {
        const s = _.readSlice(8)?;
        return ((s[0] as u64) << 56) |
            ((s[1] as u64) << 48) |
            ((s[2] as u64) << 40) |
            ((s[3] as u64) << 32) |
            ((s[4] as u64) << 24) |
            ((s[5] as u64) << 16) |
            ((s[6] as u64) << 8) |
            (s[7] as u64);
    }

    fn readBytesWithLen(*_, len: usize): ([]const u8)! {
        if (len > _.remaining()) {
            return err.UnexpectedEof;
        }
        const raw = _.readSlice(len)?;
        return _.alloc.dupe(u8, raw) catch return err.OutOfMemory;
    }

    fn readTextWithLen(*_, len: usize): ([]const u8)! {
        if (len > _.remaining()) {
            return err.UnexpectedEof;
        }
        const raw = _.readSlice(len)?;
        if (!std.unicode.utf8ValidateSlice(raw)) {
            return err.InvalidFormat;
        }
        return _.alloc.dupe(u8, raw) catch return err.OutOfMemory;
    }

    fn readArrayWithLen(*_, count: usize, depth: usize): BinValue! {
        if (depth >= MSGPACK_MAX_DEPTH) {
            return err.MaxDepthExceeded;
        }
        if (count > _.remaining()) {
            return err.UnexpectedEof;
        }
        let items = _.alloc.alloc(BinValue, count) catch return err.OutOfMemory;
        let i: usize = 0;
        while (i < count) : (i += 1) {
            items[i] = _.readValue(depth + 1)?;
        }
        return BinValue.Array(items);
    }

    fn readMapWithLen(*_, count: usize, depth: usize): BinValue! {
        if (depth >= MSGPACK_MAX_DEPTH) {
            return err.MaxDepthExceeded;
        }
        if (count > _.remaining() / 2) {
            return err.UnexpectedEof;
        }
        let entries = _.alloc.alloc(BinMapEntry, count) catch return err.OutOfMemory;
        let i: usize = 0;
        while (i < count) : (i += 1) {
            const k = _.readValue(depth + 1)?;
            const v = _.readValue(depth + 1)?;
            entries[i] = BinMapEntry{ key: k, val: v };
        }
        return BinValue.Map(entries);
    }

    fn readValue(*_, depth: usize): BinValue! {
        const tag = _.readByte()?;

        // Positive fixint: 0x00 ..= 0x7F
        if (tag <= 0x7F) {
            return BinValue.Uint(tag as u64);
        }
        // Fixmap: 0x80 ..= 0x8F
        if (tag <= 0x8F) {
            return _.readMapWithLen((tag & 0x0F) as usize, depth);
        }
        // Fixarray: 0x90 ..= 0x9F
        if (tag <= 0x9F) {
            return _.readArrayWithLen((tag & 0x0F) as usize, depth);
        }
        // Fixstr: 0xA0 ..= 0xBF
        if (tag <= 0xBF) {
            const s = _.readTextWithLen((tag & 0x1F) as usize)?;
            return BinValue.Text(s);
        }
        // Negative fixint: 0xE0 ..= 0xFF
        if (tag >= 0xE0) {
            let signed_v: i64 = 0;
            zig {
                const sv: i8 = @bitCast(tag);
                signed_v = @intCast(sv);
            }
            return BinValue.Int(signed_v);
        }

        switch (tag) {
            0xC0 => return BinValue.Null,
            0xC1 => return err.InvalidFormat,
            0xC2 => return BinValue.Bool(false),
            0xC3 => return BinValue.Bool(true),
            0xC4 => {
                const len = (_.readByte()?) as usize;
                return BinValue.Bytes(_.readBytesWithLen(len)?);
            },
            0xC5 => {
                const len = (_.readUint16()?) as usize;
                return BinValue.Bytes(_.readBytesWithLen(len)?);
            },
            0xC6 => {
                const len = (_.readUint32()?) as usize;
                return BinValue.Bytes(_.readBytesWithLen(len)?);
            },
            0xC7 | 0xC8 | 0xC9 => return err.UnsupportedFeature,
            0xCA => {
                const bits = _.readUint32()?;
                let f: f64 = 0.0;
                zig {
                    const s: f32 = @bitCast(bits);
                    f = @floatCast(s);
                }
                return BinValue.Float(f);
            },
            0xCB => {
                const bits = _.readUint64()?;
                let f: f64 = 0.0;
                zig {
                    f = @bitCast(bits);
                }
                return BinValue.Float(f);
            },
            0xCC => {
                const v = _.readByte()?;
                return BinValue.Uint(v as u64);
            },
            0xCD => {
                const v = _.readUint16()?;
                return BinValue.Uint(v as u64);
            },
            0xCE => {
                const v = _.readUint32()?;
                return BinValue.Uint(v as u64);
            },
            0xCF => {
                const v = _.readUint64()?;
                return BinValue.Uint(v);
            },
            0xD0 => {
                const raw = _.readByte()?;
                let sv: i64 = 0;
                zig {
                    const b: i8 = @bitCast(raw);
                    sv = @intCast(b);
                }
                return BinValue.Int(sv);
            },
            0xD1 => {
                const raw = _.readUint16()?;
                let sv: i64 = 0;
                zig {
                    const w: i16 = @bitCast(raw);
                    sv = @intCast(w);
                }
                return BinValue.Int(sv);
            },
            0xD2 => {
                const raw = _.readUint32()?;
                let sv: i64 = 0;
                zig {
                    const d: i32 = @bitCast(raw);
                    sv = @intCast(d);
                }
                return BinValue.Int(sv);
            },
            0xD3 => {
                const raw = _.readUint64()?;
                let sv: i64 = 0;
                zig {
                    sv = @bitCast(raw);
                }
                return BinValue.Int(sv);
            },
            0xD4 | 0xD5 | 0xD6 | 0xD7 | 0xD8 => return err.UnsupportedFeature,
            0xD9 => {
                const len = (_.readByte()?) as usize;
                return BinValue.Text(_.readTextWithLen(len)?);
            },
            0xDA => {
                const len = (_.readUint16()?) as usize;
                return BinValue.Text(_.readTextWithLen(len)?);
            },
            0xDB => {
                const len = (_.readUint32()?) as usize;
                return BinValue.Text(_.readTextWithLen(len)?);
            },
            0xDC => {
                const count = (_.readUint16()?) as usize;
                return _.readArrayWithLen(count, depth);
            },
            0xDD => {
                const count = (_.readUint32()?) as usize;
                return _.readArrayWithLen(count, depth);
            },
            0xDE => {
                const count = (_.readUint16()?) as usize;
                return _.readMapWithLen(count, depth);
            },
            0xDF => {
                const count = (_.readUint32()?) as usize;
                return _.readMapWithLen(count, depth);
            },
            else => return err.InvalidFormat,
        }
    }
}

pub struct MsgPack {
    static fn writeUint(
        out: *std.ArrayList(u8),
        alloc: std.mem.Allocator,
        u: u64,
    ): ! {
        if (u <= 0x7F) {
            out.append(alloc, u as u8) catch return err.OutOfMemory;
            return;
        }
        if (u <= 0xFF) {
            let buf: [2]u8 = [0xCC, u as u8];
            out.appendSlice(alloc, buf[0..2]) catch return err.OutOfMemory;
            return;
        }
        if (u <= 0xFFFF) {
            let buf: [3]u8 = [
                0xCD,
                ((u >> 8) & 0xFF) as u8,
                (u & 0xFF) as u8,
            ];
            out.appendSlice(alloc, buf[0..3]) catch return err.OutOfMemory;
            return;
        }
        if (u <= 0xFFFF_FFFF) {
            let buf: [5]u8 = [
                0xCE,
                ((u >> 24) & 0xFF) as u8,
                ((u >> 16) & 0xFF) as u8,
                ((u >> 8) & 0xFF) as u8,
                (u & 0xFF) as u8,
            ];
            out.appendSlice(alloc, buf[0..5]) catch return err.OutOfMemory;
            return;
        }
        let buf: [9]u8 = [
            0xCF,
            ((u >> 56) & 0xFF) as u8,
            ((u >> 48) & 0xFF) as u8,
            ((u >> 40) & 0xFF) as u8,
            ((u >> 32) & 0xFF) as u8,
            ((u >> 24) & 0xFF) as u8,
            ((u >> 16) & 0xFF) as u8,
            ((u >> 8) & 0xFF) as u8,
            (u & 0xFF) as u8,
        ];
        out.appendSlice(alloc, buf[0..9]) catch return err.OutOfMemory;
    }

    static fn writeInt(
        out: *std.ArrayList(u8),
        alloc: std.mem.Allocator,
        i: i64,
    ): ! {
        if (i >= 0) {
            MsgPack.writeUint(out, alloc, i as u64)?;
            return;
        }
        if (i >= -32) {
            let b: u8 = 0;
            zig {
                const sv: i8 = @intCast(i);
                b = @bitCast(sv);
            }
            out.append(alloc, b) catch return err.OutOfMemory;
            return;
        }
        if (i >= -128) {
            let b: u8 = 0;
            zig {
                const sv: i8 = @intCast(i);
                b = @bitCast(sv);
            }
            let buf: [2]u8 = [0xD0, b];
            out.appendSlice(alloc, buf[0..2]) catch return err.OutOfMemory;
            return;
        }
        if (i >= -32768) {
            let w: u16 = 0;
            zig {
                const sv: i16 = @intCast(i);
                w = @bitCast(sv);
            }
            let buf: [3]u8 = [
                0xD1,
                ((w >> 8) & 0xFF) as u8,
                (w & 0xFF) as u8,
            ];
            out.appendSlice(alloc, buf[0..3]) catch return err.OutOfMemory;
            return;
        }
        if (i >= -2147483648) {
            let d: u32 = 0;
            zig {
                const sv: i32 = @intCast(i);
                d = @bitCast(sv);
            }
            let buf: [5]u8 = [
                0xD2,
                ((d >> 24) & 0xFF) as u8,
                ((d >> 16) & 0xFF) as u8,
                ((d >> 8) & 0xFF) as u8,
                (d & 0xFF) as u8,
            ];
            out.appendSlice(alloc, buf[0..5]) catch return err.OutOfMemory;
            return;
        }
        let q: u64 = 0;
        zig {
            q = @bitCast(i);
        }
        let buf: [9]u8 = [
            0xD3,
            ((q >> 56) & 0xFF) as u8,
            ((q >> 48) & 0xFF) as u8,
            ((q >> 40) & 0xFF) as u8,
            ((q >> 32) & 0xFF) as u8,
            ((q >> 24) & 0xFF) as u8,
            ((q >> 16) & 0xFF) as u8,
            ((q >> 8) & 0xFF) as u8,
            (q & 0xFF) as u8,
        ];
        out.appendSlice(alloc, buf[0..9]) catch return err.OutOfMemory;
    }

    static fn writeFloat(
        out: *std.ArrayList(u8),
        alloc: std.mem.Allocator,
        v: f64,
    ): ! {
        zig {
            if (std.math.isNan(v)) {
                out.appendSlice(alloc, &[_]u8{ 0xCA, 0x7F, 0xC0, 0x00, 0x00 }) catch return error.OutOfMemory;
                return;
            }
            const bits64: u64 = @bitCast(v);
            const f32_v: f32 = @floatCast(v);
            if (@as(u64, @bitCast(@as(f64, @floatCast(f32_v)))) == bits64) {
                const b32: u32 = @bitCast(f32_v);
                const buf = [5]u8{
                    0xCA,
                    @intCast((b32 >> 24) & 0xFF),
                    @intCast((b32 >> 16) & 0xFF),
                    @intCast((b32 >> 8) & 0xFF),
                    @intCast(b32 & 0xFF),
                };
                out.appendSlice(alloc, &buf) catch return error.OutOfMemory;
                return;
            }
            const buf = [9]u8{
                0xCB,
                @intCast((bits64 >> 56) & 0xFF),
                @intCast((bits64 >> 48) & 0xFF),
                @intCast((bits64 >> 40) & 0xFF),
                @intCast((bits64 >> 32) & 0xFF),
                @intCast((bits64 >> 24) & 0xFF),
                @intCast((bits64 >> 16) & 0xFF),
                @intCast((bits64 >> 8) & 0xFF),
                @intCast(bits64 & 0xFF),
            };
            out.appendSlice(alloc, &buf) catch return error.OutOfMemory;
        }
    }

    static fn writeBytes(
        out: *std.ArrayList(u8),
        alloc: std.mem.Allocator,
        bs: []const u8,
    ): ! {
        const len = bs.len;
        if (len <= 0xFF) {
            let hdr: [2]u8 = [0xC4, len as u8];
            out.appendSlice(alloc, hdr[0..2]) catch return err.OutOfMemory;
        } else if (len <= 0xFFFF) {
            let hdr: [3]u8 = [0xC5, ((len >> 8) & 0xFF) as u8, (len & 0xFF) as u8];
            out.appendSlice(alloc, hdr[0..3]) catch return err.OutOfMemory;
        } else if ((len as u64) <= 0xFFFF_FFFF) {
            let hdr: [5]u8 = [
                0xC6,
                ((len >> 24) & 0xFF) as u8,
                ((len >> 16) & 0xFF) as u8,
                ((len >> 8) & 0xFF) as u8,
                (len & 0xFF) as u8,
            ];
            out.appendSlice(alloc, hdr[0..5]) catch return err.OutOfMemory;
        } else {
            return err.UnsupportedFeature;
        }
        out.appendSlice(alloc, bs) catch return err.OutOfMemory;
    }

    static fn writeText(
        out: *std.ArrayList(u8),
        alloc: std.mem.Allocator,
        s: []const u8,
    ): ! {
        const len = s.len;
        if (len <= 31) {
            out.append(alloc, 0xA0 | (len as u8)) catch return err.OutOfMemory;
        } else if (len <= 0xFF) {
            let hdr: [2]u8 = [0xD9, len as u8];
            out.appendSlice(alloc, hdr[0..2]) catch return err.OutOfMemory;
        } else if (len <= 0xFFFF) {
            let hdr: [3]u8 = [0xDA, ((len >> 8) & 0xFF) as u8, (len & 0xFF) as u8];
            out.appendSlice(alloc, hdr[0..3]) catch return err.OutOfMemory;
        } else if ((len as u64) <= 0xFFFF_FFFF) {
            let hdr: [5]u8 = [
                0xDB,
                ((len >> 24) & 0xFF) as u8,
                ((len >> 16) & 0xFF) as u8,
                ((len >> 8) & 0xFF) as u8,
                (len & 0xFF) as u8,
            ];
            out.appendSlice(alloc, hdr[0..5]) catch return err.OutOfMemory;
        } else {
            return err.UnsupportedFeature;
        }
        out.appendSlice(alloc, s) catch return err.OutOfMemory;
    }

    static fn writeArrayHeader(
        out: *std.ArrayList(u8),
        alloc: std.mem.Allocator,
        len: usize,
    ): ! {
        if (len <= 15) {
            out.append(alloc, 0x90 | (len as u8)) catch return err.OutOfMemory;
        } else if (len <= 0xFFFF) {
            let hdr: [3]u8 = [0xDC, ((len >> 8) & 0xFF) as u8, (len & 0xFF) as u8];
            out.appendSlice(alloc, hdr[0..3]) catch return err.OutOfMemory;
        } else if ((len as u64) <= 0xFFFF_FFFF) {
            let hdr: [5]u8 = [
                0xDD,
                ((len >> 24) & 0xFF) as u8,
                ((len >> 16) & 0xFF) as u8,
                ((len >> 8) & 0xFF) as u8,
                (len & 0xFF) as u8,
            ];
            out.appendSlice(alloc, hdr[0..5]) catch return err.OutOfMemory;
        } else {
            return err.UnsupportedFeature;
        }
    }

    static fn writeMapHeader(
        out: *std.ArrayList(u8),
        alloc: std.mem.Allocator,
        len: usize,
    ): ! {
        if (len <= 15) {
            out.append(alloc, 0x80 | (len as u8)) catch return err.OutOfMemory;
        } else if (len <= 0xFFFF) {
            let hdr: [3]u8 = [0xDE, ((len >> 8) & 0xFF) as u8, (len & 0xFF) as u8];
            out.appendSlice(alloc, hdr[0..3]) catch return err.OutOfMemory;
        } else if ((len as u64) <= 0xFFFF_FFFF) {
            let hdr: [5]u8 = [
                0xDF,
                ((len >> 24) & 0xFF) as u8,
                ((len >> 16) & 0xFF) as u8,
                ((len >> 8) & 0xFF) as u8,
                (len & 0xFF) as u8,
            ];
            out.appendSlice(alloc, hdr[0..5]) catch return err.OutOfMemory;
        } else {
            return err.UnsupportedFeature;
        }
    }

    static fn encodeValue(
        out: *std.ArrayList(u8),
        alloc: std.mem.Allocator,
        val: BinValue,
        depth: usize,
    ): ! {
        switch (val) {
            .Null => {
                out.append(alloc, 0xC0) catch return err.OutOfMemory;
            },
            .Bool b => {
                out.append(alloc, if (b) 0xC3 else 0xC2) catch return err.OutOfMemory;
            },
            .Uint u => {
                MsgPack.writeUint(out, alloc, u)?;
            },
            .Int i => {
                MsgPack.writeInt(out, alloc, i)?;
            },
            .Float f => {
                MsgPack.writeFloat(out, alloc, f)?;
            },
            .Bytes bs => {
                MsgPack.writeBytes(out, alloc, bs)?;
            },
            .Text s => {
                MsgPack.writeText(out, alloc, s)?;
            },
            .Array items => {
                if (depth >= MSGPACK_MAX_DEPTH) {
                    return err.MaxDepthExceeded;
                }
                MsgPack.writeArrayHeader(out, alloc, items.len)?;
                for (item of items) {
                    MsgPack.encodeValue(out, alloc, item, depth + 1)?;
                }
            },
            .Map entries => {
                if (depth >= MSGPACK_MAX_DEPTH) {
                    return err.MaxDepthExceeded;
                }
                MsgPack.writeMapHeader(out, alloc, entries.len)?;
                for (entry of entries) {
                    MsgPack.encodeValue(out, alloc, entry.key, depth + 1)?;
                    MsgPack.encodeValue(out, alloc, entry.val, depth + 1)?;
                }
            },
            else => {
                return err.UnsupportedFeature;
            },
        }
    }

    pub static fn encode(alloc: std.mem.Allocator, val: BinValue): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        MsgPack.encodeValue(&out, alloc, val, 0)?;
        return out.toOwnedSlice(alloc) catch return err.OutOfMemory;
    }

    pub static fn encodeDocument(alloc: std.mem.Allocator, doc: BinDocument): ([]u8)! {
        return MsgPack.encode(alloc, doc.root);
    }

    /// Decodes MessagePack bytes into a `BinValue` allocated directly from `alloc` (intended for Arena allocators).
    pub static fn decodeValue(alloc: std.mem.Allocator, bytes: []const u8): BinValue! {
        if (bytes.len == 0) {
            return err.UnexpectedEof;
        }
        let reader = MsgPackReader{
            src: bytes,
            pos: 0,
            alloc: alloc,
        };
        const root = reader.readValue(0)?;
        if (reader.pos < reader.src.len) {
            return err.TrailingBytes;
        }
        return root;
    }

    /// Decodes MessagePack bytes into an Arena-owned `BinDocument`. Call `doc.deinit()` when finished.
    pub static fn decode(alloc: std.mem.Allocator, bytes: []const u8): BinDocument! {
        let doc = BinDocument.create(alloc)?;
        errdefer doc.deinit();

        const arena_alloc = doc.allocator() ?? return err.OutOfMemory;
        doc.root = MsgPack.decodeValue(arena_alloc, bytes)?;
        return doc;
    }
}
