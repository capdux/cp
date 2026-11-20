// Source: cp/lib/cbor/cbor.cp (RFC 8949 CBOR Encoder & Decoder)
// Pure CP RFC 8949 Concise Binary Object Representation (CBOR) codec supporting
// all 8 major types (0-7), minimal integer & shortest IEEE 754 float encoding,
// Float16/Float32/Float64 decoding, semantic tags, indefinite-length decoding,
// max nesting depth 64 guard, and strict EOF / trailing-bytes validation.

const std = @import("std");
import { BinError, BinTag, BinMapEntry, BinValue, BinDocument } from "./value.cp";

pub const CBOR_MAX_DEPTH: usize = 64;

pub const CBOR_TAG_DATETIME_TEXT: u64 = 0;
pub const CBOR_TAG_EPOCH_TIME: u64 = 1;
pub const CBOR_TAG_ENCODED_CBOR: u64 = 24;
pub const CBOR_TAG_SELF_DESCRIBE: u64 = 55799;

struct CborReader {
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

    fn peekByte(): u8? {
        if (_.pos >= _.src.len) return null;
        return _.src[_.pos];
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

    fn readArg(*_, ai: u8): u64! {
        if (ai < 24) {
            return ai as u64;
        }
        if (ai == 24) {
            return (_.readByte()?) as u64;
        }
        if (ai == 25) {
            return (_.readUint16()?) as u64;
        }
        if (ai == 26) {
            return (_.readUint32()?) as u64;
        }
        if (ai == 27) {
            return _.readUint64()?;
        }
        return err.InvalidFormat;
    }

    fn readDefiniteBytes(*_, ai: u8): ([]const u8)! {
        const len_u64 = _.readArg(ai)?;
        if (len_u64 > (_.remaining() as u64)) {
            return err.UnexpectedEof;
        }
        const raw = _.readSlice(len_u64 as usize)?;
        return _.alloc.dupe(u8, raw) catch return err.OutOfMemory;
    }

    fn readIndefiniteChunks(*_, expected_major: u8, validate_utf8: bool): ([]const u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(_.alloc);

        while (true) {
            const next_b = _.peekByte() ?? return err.UnexpectedEof;
            if (next_b == 0xFF) {
                * = _.readByte()?;
                break;
            }
            const chunk_ib = _.readByte()?;
            const chunk_major = chunk_ib >> 5;
            const chunk_ai = chunk_ib & 0x1F;
            if (chunk_major != expected_major || chunk_ai == 31) {
                return err.InvalidFormat;
            }
            const len_u64 = _.readArg(chunk_ai)?;
            if (len_u64 > (_.remaining() as u64)) {
                return err.UnexpectedEof;
            }
            const chunk = _.readSlice(len_u64 as usize)?;
            if (validate_utf8 && !std.unicode.utf8ValidateSlice(chunk)) {
                return err.InvalidFormat;
            }
            out.appendSlice(_.alloc, chunk) catch return err.OutOfMemory;
        }

        return out.toOwnedSlice(_.alloc) catch return err.OutOfMemory;
    }

    fn readValue(*_, depth: usize): BinValue! {
        const ib = _.readByte()?;
        const major = ib >> 5;
        const ai = ib & 0x1F;

        switch (major) {
            0 => {
                if (ai == 31) return err.InvalidFormat;
                const v = _.readArg(ai)?;
                return BinValue.Uint(v);
            },
            1 => {
                if (ai == 31) return err.InvalidFormat;
                const n = _.readArg(ai)?;
                if (n > 9223372036854775807) {
                    return err.UnsupportedFeature;
                }
                return BinValue.Int(-1 - (n as i64));
            },
            2 => {
                if (ai == 31) {
                    const joined = _.readIndefiniteChunks(2, false)?;
                    return BinValue.Bytes(joined);
                }
                const duped = _.readDefiniteBytes(ai)?;
                return BinValue.Bytes(duped);
            },
            3 => {
                if (ai == 31) {
                    const joined = _.readIndefiniteChunks(3, true)?;
                    return BinValue.Text(joined);
                }
                const len_u64 = _.readArg(ai)?;
                if (len_u64 > (_.remaining() as u64)) {
                    return err.UnexpectedEof;
                }
                const raw = _.readSlice(len_u64 as usize)?;
                if (!std.unicode.utf8ValidateSlice(raw)) {
                    return err.InvalidFormat;
                }
                const duped = _.alloc.dupe(u8, raw) catch return err.OutOfMemory;
                return BinValue.Text(duped);
            },
            4 => {
                if (depth >= CBOR_MAX_DEPTH) {
                    return err.MaxDepthExceeded;
                }
                if (ai == 31) {
                    let list: std.ArrayList(BinValue) = .empty;
                    errdefer list.deinit(_.alloc);
                    while (true) {
                        const next_b = _.peekByte() ?? return err.UnexpectedEof;
                        if (next_b == 0xFF) {
                            * = _.readByte()?;
                            break;
                        }
                        const item = _.readValue(depth + 1)?;
                        list.append(_.alloc, item) catch return err.OutOfMemory;
                    }
                    const items = list.toOwnedSlice(_.alloc) catch return err.OutOfMemory;
                    return BinValue.Array(items);
                }
                const count_u64 = _.readArg(ai)?;
                if (count_u64 > (_.remaining() as u64)) {
                    return err.UnexpectedEof;
                }
                const count = count_u64 as usize;
                let items = _.alloc.alloc(BinValue, count) catch return err.OutOfMemory;
                let i: usize = 0;
                while (i < count) : (i += 1) {
                    items[i] = _.readValue(depth + 1)?;
                }
                return BinValue.Array(items);
            },
            5 => {
                if (depth >= CBOR_MAX_DEPTH) {
                    return err.MaxDepthExceeded;
                }
                if (ai == 31) {
                    let list: std.ArrayList(BinMapEntry) = .empty;
                    errdefer list.deinit(_.alloc);
                    while (true) {
                        const next_b = _.peekByte() ?? return err.UnexpectedEof;
                        if (next_b == 0xFF) {
                            * = _.readByte()?;
                            break;
                        }
                        const k = _.readValue(depth + 1)?;
                        const v = _.readValue(depth + 1)?;
                        list.append(_.alloc, BinMapEntry{ key: k, val: v }) catch return err.OutOfMemory;
                    }
                    const entries = list.toOwnedSlice(_.alloc) catch return err.OutOfMemory;
                    return BinValue.Map(entries);
                }
                const count_u64 = _.readArg(ai)?;
                if (count_u64 > ((_.remaining() / 2) as u64)) {
                    return err.UnexpectedEof;
                }
                const count = count_u64 as usize;
                let entries = _.alloc.alloc(BinMapEntry, count) catch return err.OutOfMemory;
                let i: usize = 0;
                while (i < count) : (i += 1) {
                    const k = _.readValue(depth + 1)?;
                    const v = _.readValue(depth + 1)?;
                    entries[i] = BinMapEntry{ key: k, val: v };
                }
                return BinValue.Map(entries);
            },
            6 => {
                if (depth >= CBOR_MAX_DEPTH) {
                    return err.MaxDepthExceeded;
                }
                if (ai == 31) return err.InvalidFormat;
                const tag_num = _.readArg(ai)?;
                const inner = _.readValue(depth + 1)?;
                return BinValue.allocTag(_.alloc, tag_num, inner);
            },
            7 => {
                if (ai < 20) {
                    return err.UnsupportedFeature;
                }
                if (ai == 20) return BinValue.Bool(false);
                if (ai == 21) return BinValue.Bool(true);
                if (ai == 22 || ai == 23) return BinValue.Null;
                if (ai == 24) {
                    const s = _.readByte()?;
                    if (s < 32) return err.InvalidFormat;
                    return err.UnsupportedFeature;
                }
                if (ai == 25) {
                    const bits = _.readUint16()?;
                    let f: f64 = 0.0;
                    zig {
                        const h: f16 = @bitCast(bits);
                        f = @floatCast(h);
                    }
                    return BinValue.Float(f);
                }
                if (ai == 26) {
                    const bits = _.readUint32()?;
                    let f: f64 = 0.0;
                    zig {
                        const s: f32 = @bitCast(bits);
                        f = @floatCast(s);
                    }
                    return BinValue.Float(f);
                }
                if (ai == 27) {
                    const bits = _.readUint64()?;
                    let f: f64 = 0.0;
                    zig {
                        f = @bitCast(bits);
                    }
                    return BinValue.Float(f);
                }
                return err.InvalidFormat;
            },
            else => return err.InvalidFormat,
        }
    }
}

pub struct Cbor {
    static fn writeTypeAndArg(
        out: *std.ArrayList(u8),
        alloc: std.mem.Allocator,
        major: u8,
        arg: u64,
    ): ! {
        const major_bits: u8 = (major & 0x07) << 5;
        if (arg <= 23) {
            out.append(alloc, major_bits | (arg as u8)) catch return err.OutOfMemory;
            return;
        }
        if (arg <= 0xFF) {
            let buf: [2]u8 = [major_bits | 24, arg as u8];
            out.appendSlice(alloc, buf[0..2]) catch return err.OutOfMemory;
            return;
        }
        if (arg <= 0xFFFF) {
            let buf: [3]u8 = [
                major_bits | 25,
                ((arg >> 8) & 0xFF) as u8,
                (arg & 0xFF) as u8,
            ];
            out.appendSlice(alloc, buf[0..3]) catch return err.OutOfMemory;
            return;
        }
        if (arg <= 0xFFFF_FFFF) {
            let buf: [5]u8 = [
                major_bits | 26,
                ((arg >> 24) & 0xFF) as u8,
                ((arg >> 16) & 0xFF) as u8,
                ((arg >> 8) & 0xFF) as u8,
                (arg & 0xFF) as u8,
            ];
            out.appendSlice(alloc, buf[0..5]) catch return err.OutOfMemory;
            return;
        }
        let buf: [9]u8 = [
            major_bits | 27,
            ((arg >> 56) & 0xFF) as u8,
            ((arg >> 48) & 0xFF) as u8,
            ((arg >> 40) & 0xFF) as u8,
            ((arg >> 32) & 0xFF) as u8,
            ((arg >> 24) & 0xFF) as u8,
            ((arg >> 16) & 0xFF) as u8,
            ((arg >> 8) & 0xFF) as u8,
            (arg & 0xFF) as u8,
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
                out.appendSlice(alloc, &[_]u8{ 0xF9, 0x7E, 0x00 }) catch return error.OutOfMemory;
                return;
            }
            const bits64: u64 = @bitCast(v);
            const f16_v: f16 = @floatCast(v);
            if (@as(u64, @bitCast(@as(f64, @floatCast(f16_v)))) == bits64) {
                const b16: u16 = @bitCast(f16_v);
                const buf = [3]u8{
                    0xF9,
                    @intCast((b16 >> 8) & 0xFF),
                    @intCast(b16 & 0xFF),
                };
                out.appendSlice(alloc, &buf) catch return error.OutOfMemory;
                return;
            }
            const f32_v: f32 = @floatCast(v);
            if (@as(u64, @bitCast(@as(f64, @floatCast(f32_v)))) == bits64) {
                const b32: u32 = @bitCast(f32_v);
                const buf = [5]u8{
                    0xFA,
                    @intCast((b32 >> 24) & 0xFF),
                    @intCast((b32 >> 16) & 0xFF),
                    @intCast((b32 >> 8) & 0xFF),
                    @intCast(b32 & 0xFF),
                };
                out.appendSlice(alloc, &buf) catch return error.OutOfMemory;
                return;
            }
            const buf = [9]u8{
                0xFB,
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

    static fn encodeValue(
        out: *std.ArrayList(u8),
        alloc: std.mem.Allocator,
        val: BinValue,
        depth: usize,
    ): ! {
        switch (val) {
            .Null => {
                out.append(alloc, 0xF6) catch return err.OutOfMemory;
            },
            .Bool b => {
                out.append(alloc, if (b) 0xF5 else 0xF4) catch return err.OutOfMemory;
            },
            .Uint u => {
                Cbor.writeTypeAndArg(out, alloc, 0, u)?;
            },
            .Int i => {
                if (i >= 0) {
                    Cbor.writeTypeAndArg(out, alloc, 0, i as u64)?;
                } else {
                    let n: u64 = 0;
                    zig {
                        n = @intCast(-(i +% 1));
                    }
                    Cbor.writeTypeAndArg(out, alloc, 1, n)?;
                }
            },
            .Float f => {
                Cbor.writeFloat(out, alloc, f)?;
            },
            .Bytes bs => {
                Cbor.writeTypeAndArg(out, alloc, 2, bs.len as u64)?;
                out.appendSlice(alloc, bs) catch return err.OutOfMemory;
            },
            .Text s => {
                Cbor.writeTypeAndArg(out, alloc, 3, s.len as u64)?;
                out.appendSlice(alloc, s) catch return err.OutOfMemory;
            },
            .Array items => {
                if (depth >= CBOR_MAX_DEPTH) {
                    return err.MaxDepthExceeded;
                }
                Cbor.writeTypeAndArg(out, alloc, 4, items.len as u64)?;
                for (item of items) {
                    Cbor.encodeValue(out, alloc, item, depth + 1)?;
                }
            },
            .Map entries => {
                if (depth >= CBOR_MAX_DEPTH) {
                    return err.MaxDepthExceeded;
                }
                Cbor.writeTypeAndArg(out, alloc, 5, entries.len as u64)?;
                for (entry of entries) {
                    Cbor.encodeValue(out, alloc, entry.key, depth + 1)?;
                    Cbor.encodeValue(out, alloc, entry.val, depth + 1)?;
                }
            },
            .Tag t => {
                if (depth >= CBOR_MAX_DEPTH) {
                    return err.MaxDepthExceeded;
                }
                Cbor.writeTypeAndArg(out, alloc, 6, t.tag)?;
                Cbor.encodeValue(out, alloc, t.value.*, depth + 1)?;
            },
        }
    }

    pub static fn encode(alloc: std.mem.Allocator, val: BinValue): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        Cbor.encodeValue(&out, alloc, val, 0)?;
        return out.toOwnedSlice(alloc) catch return err.OutOfMemory;
    }

    pub static fn encodeDocument(alloc: std.mem.Allocator, doc: BinDocument): ([]u8)! {
        return Cbor.encode(alloc, doc.root);
    }

    /// Decodes CBOR bytes into a `BinValue` allocated directly from `alloc` (intended for Arena allocators).
    pub static fn decodeValue(alloc: std.mem.Allocator, bytes: []const u8): BinValue! {
        if (bytes.len == 0) {
            return err.UnexpectedEof;
        }
        let reader = CborReader{
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

    /// Decodes CBOR bytes into an Arena-owned `BinDocument`. Call `doc.deinit()` when finished.
    pub static fn decode(alloc: std.mem.Allocator, bytes: []const u8): BinDocument! {
        let doc = BinDocument.create(alloc)?;
        errdefer doc.deinit();

        const arena_alloc = doc.allocator() ?? return err.OutOfMemory;
        doc.root = Cbor.decodeValue(arena_alloc, bytes)?;
        return doc;
    }
}
