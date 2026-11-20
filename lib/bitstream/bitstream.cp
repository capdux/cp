// Source: cp/lib/bitstream/bitstream.cp (Pure CP MSB/LSB BitStream Reader/Writer & LEB128/Exp-Golomb Codec)
// Zero-C-dependency bit-level I/O, H.264/HEVC 0-th order Exp-Golomb, and ULEB128/SLEB128 varint engine.

const std = @import("std");

/// Unified error set returned by `cap:bitstream` operations.
pub err BitStreamError {
    UnexpectedEof,
    InvalidBitCount,
    IntegerOverflow,
    InvalidExpGolomb,
    OutOfMemory,
}

/// Bit ordering within each byte for `BitWriter` and `BitReader`.
pub enum BitOrder {
    MsbFirst,
    LsbFirst,
}

/// Result returned by `Leb128.decodeUleb128`.
pub struct Uleb128Result {
    value: u64,
    bytes_read: usize,
}

/// Result returned by `Leb128.decodeSleb128`.
pub struct Sleb128Result {
    value: i64,
    bytes_read: usize,
}

/// Bit-level writer supporting `MsbFirst` and `LsbFirst` bit orders,
/// 1..64-bit unaligned writes, 0-th order Exp-Golomb encoding, and byte alignment.
pub struct BitWriter {
    alloc: std.mem.Allocator,
    order: BitOrder,
    buf: std.ArrayList(u8) = .empty,
    cur_byte: u8 = 0,
    bit_pos: u4 = 0,

    /// Initializes a new `BitWriter` with the given allocator and bit order.
    pub static fn init(allocator: std.mem.Allocator, order: BitOrder): BitWriter {
        return BitWriter{
            alloc: allocator,
            order: order,
            buf: .empty,
            cur_byte: 0,
            bit_pos: 0,
        };
    }

    /// Releases all internal buffer memory owned by this `BitWriter`.
    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
        _.cur_byte = 0;
        _.bit_pos = 0;
    }

    /// Writes a single bit (`0` or `1`) into the stream.
    pub fn writeBit(*_, bit: u1): void ! BitStreamError {
        if (bit == 1) {
            switch (_.order) {
                BitOrder.MsbFirst => {
                    _.cur_byte |= ((1 as u8) << ((7 - _.bit_pos) as u3));
                },
                BitOrder.LsbFirst => {
                    _.cur_byte |= ((1 as u8) << (_.bit_pos as u3));
                },
            }
        }
        _.bit_pos += 1;
        if (_.bit_pos == 8) {
            _.buf.append(_.alloc, _.cur_byte) catch return BitStreamError.OutOfMemory;
            _.cur_byte = 0;
            _.bit_pos = 0;
        }
    }

    /// Writes `num_bits` (1..64) from the lower bits of `value`.
    /// Returns `BitStreamError.InvalidBitCount` if `num_bits == 0` or `num_bits > 64`.
    pub fn writeBits(*_, value: u64, num_bits: u7): void ! BitStreamError {
        if (num_bits == 0 || num_bits > 64) {
            return BitStreamError.InvalidBitCount;
        }
        switch (_.order) {
            BitOrder.MsbFirst => {
                let i: u7 = num_bits;
                while (i > 0) {
                    i -= 1;
                    const bit: u1 = ((value >> (i as u6)) & 1) as u1;
                    _.writeBit(bit)?;
                }
            },
            BitOrder.LsbFirst => {
                let i: u7 = 0;
                while (i < num_bits) : (i += 1) {
                    const bit: u1 = ((value >> (i as u6)) & 1) as u1;
                    _.writeBit(bit)?;
                }
            },
        }
    }

    fn writeExpGolombCodeNum(*_, code_num: u64): void ! BitStreamError {
        const bit_len: u7 = (64 - @clz(code_num)) as u7;
        const leading_zeros: u7 = bit_len - 1;
        let z: u7 = 0;
        while (z < leading_zeros) : (z += 1) {
            _.writeBit(0)?;
        }
        _.writeBit(1)?;
        let s: u7 = leading_zeros;
        while (s > 0) {
            s -= 1;
            const bit: u1 = ((code_num >> (s as u6)) & 1) as u1;
            _.writeBit(bit)?;
        }
    }

    /// Writes an unsigned integer `v` using H.264/HEVC 0-th order Exp-Golomb (`ue(v)`).
    pub fn writeExpGolombU(*_, v: u32): void ! BitStreamError {
        const code_num: u64 = (v as u64) + 1;
        _.writeExpGolombCodeNum(code_num)?;
    }

    /// Writes a signed integer `v` using H.264/HEVC 0-th order Exp-Golomb (`se(v)`).
    pub fn writeExpGolombS(*_, v: i32): void ! BitStreamError {
        let mapped: u64 = 0;
        if (v > 0) {
            mapped = ((v as u64) * 2) - 1;
        } else if (v < 0) {
            const neg: i64 = -(v as i64);
            mapped = (neg as u64) * 2;
        }
        _.writeExpGolombCodeNum(mapped + 1)?;
    }

    /// Zero-pads the current partial byte (if any) to align the writer to a byte boundary.
    pub fn alignToByte(*_): void ! BitStreamError {
        if (_.bit_pos > 0) {
            _.buf.append(_.alloc, _.cur_byte) catch return BitStreamError.OutOfMemory;
            _.cur_byte = 0;
            _.bit_pos = 0;
        }
    }

    /// Aligns to the next byte boundary and appends `data` directly.
    pub fn writeAlignedBytes(*_, data: []const u8): void ! BitStreamError {
        _.alignToByte()?;
        if (data.len > 0) {
            _.buf.appendSlice(_.alloc, data) catch return BitStreamError.OutOfMemory;
        }
    }

    /// Aligns to the next byte boundary and transfers ownership of the written bytes to the caller.
    pub fn finish(*_): []u8 ! BitStreamError {
        _.alignToByte()?;
        return _.buf.toOwnedSlice(_.alloc) catch return BitStreamError.OutOfMemory;
    }

    /// Returns a read-only view of all complete flushed bytes currently in the buffer.
    pub fn bytes(): []const u8 {
        return _.buf.items;
    }
}

/// Bit-level reader supporting `MsbFirst` and `LsbFirst` bit orders,
/// 1..64-bit unaligned reads, non-destructive peeking, Exp-Golomb decoding, and skipping.
pub struct BitReader {
    data: []const u8,
    order: BitOrder,
    byte_pos: usize = 0,
    bit_pos: u4 = 0,

    /// Initializes a `BitReader` over `data` with the specified `order`.
    pub static fn init(data: []const u8, order: BitOrder): BitReader {
        return BitReader{
            data: data,
            order: order,
            byte_pos: 0,
            bit_pos: 0,
        };
    }

    /// Returns the number of unread bits remaining in the stream.
    pub fn bitsRemaining(): usize {
        if (_.byte_pos >= _.data.len) return 0;
        return (_.data.len - _.byte_pos) * 8 - (_.bit_pos as usize);
    }

    /// Reads and consumes a single bit (`0` or `1`).
    pub fn readBit(*_): u1 ! BitStreamError {
        if (_.byte_pos >= _.data.len) {
            return BitStreamError.UnexpectedEof;
        }
        const b = _.data[_.byte_pos];
        const bit: u1 = switch (_.order) {
            BitOrder.MsbFirst => ((b >> ((7 - _.bit_pos) as u3)) & 1) as u1,
            BitOrder.LsbFirst => ((b >> (_.bit_pos as u3)) & 1) as u1,
        };
        _.bit_pos += 1;
        if (_.bit_pos == 8) {
            _.bit_pos = 0;
            _.byte_pos += 1;
        }
        return bit;
    }

    /// Reads and consumes `num_bits` (1..64) into the lower bits of a `u64`.
    pub fn readBits(*_, num_bits: u7): u64 ! BitStreamError {
        if (num_bits == 0 || num_bits > 64) {
            return BitStreamError.InvalidBitCount;
        }
        if (_.bitsRemaining() < (num_bits as usize)) {
            return BitStreamError.UnexpectedEof;
        }
        let acc: u64 = 0;
        switch (_.order) {
            BitOrder.MsbFirst => {
                let i: u7 = 0;
                while (i < num_bits) : (i += 1) {
                    const bit = _.readBit()?;
                    acc = (acc << 1) | (bit as u64);
                }
            },
            BitOrder.LsbFirst => {
                let i: u7 = 0;
                while (i < num_bits) : (i += 1) {
                    const bit = _.readBit()?;
                    acc |= ((bit as u64) << (i as u6));
                }
            },
        }
        return acc;
    }

    /// Peeks `num_bits` (1..64) without advancing the reader position.
    pub fn peekBits(num_bits: u7): u64 ! BitStreamError {
        let copy = BitReader{
            data: _.data,
            order: _.order,
            byte_pos: _.byte_pos,
            bit_pos: _.bit_pos,
        };
        return copy.readBits(num_bits);
    }

    fn readExpGolombCodeVal(*_): u64 ! BitStreamError {
        let leading_zeros: u7 = 0;
        while (true) {
            const bit = _.readBit()?;
            if (bit == 1) break;
            leading_zeros += 1;
            if (leading_zeros > 32) {
                return BitStreamError.InvalidExpGolomb;
            }
        }
        let suffix: u64 = 0;
        let i: u7 = 0;
        while (i < leading_zeros) : (i += 1) {
            const b = _.readBit()?;
            suffix = (suffix << 1) | (b as u64);
        }
        const code_num: u64 = ((1 as u64) << (leading_zeros as u6)) | suffix;
        return code_num - 1;
    }

    /// Reads an unsigned integer using H.264/HEVC 0-th order Exp-Golomb (`ue(v)`).
    pub fn readExpGolombU(*_): u32 ! BitStreamError {
        const val = _.readExpGolombCodeVal()?;
        if (val > 0xFFFFFFFF) {
            return BitStreamError.InvalidExpGolomb;
        }
        return val as u32;
    }

    /// Reads a signed integer using H.264/HEVC 0-th order Exp-Golomb (`se(v)`).
    pub fn readExpGolombS(*_): i32 ! BitStreamError {
        const val = _.readExpGolombCodeVal()?;
        if (val == 0) return 0;
        if ((val & 1) == 1) {
            const pos = (val + 1) / 2;
            if (pos > 2147483647) {
                return BitStreamError.InvalidExpGolomb;
            }
            return pos as i32;
        }
        const neg_mag = val / 2;
        if (neg_mag > 2147483648) {
            return BitStreamError.InvalidExpGolomb;
        }
        if (neg_mag == 2147483648) {
            return -2147483648;
        }
        return -(neg_mag as i32);
    }

    /// Discards remaining bits in the current partial byte to align to the next byte boundary.
    pub fn alignToByte(*_) {
        if (_.bit_pos > 0) {
            _.bit_pos = 0;
            _.byte_pos += 1;
        }
    }

    /// Skips `n` bits forward, returning `BitStreamError.UnexpectedEof` if fewer than `n` bits remain.
    pub fn skipBits(*_, n: usize): void ! BitStreamError {
        if (_.bitsRemaining() < n) {
            return BitStreamError.UnexpectedEof;
        }
        const total_bits = (_.byte_pos * 8) + (_.bit_pos as usize) + n;
        _.byte_pos = total_bits / 8;
        _.bit_pos = (total_bits % 8) as u4;
    }
}

/// Compact LEB128 (Little-Endian Base 128) variable-length integer codec.
pub struct Leb128 {
    /// Encodes an unsigned 64-bit integer as ULEB128. Caller owns the returned slice.
    pub static fn encodeUleb128(allocator: std.mem.Allocator, v: u64): []u8 ! BitStreamError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);

        let val: u64 = v;
        while (true) {
            let byte: u8 = (val & 0x7F) as u8;
            val >>= 7;
            if (val != 0) {
                byte |= 0x80;
            }
            out.append(allocator, byte) catch return BitStreamError.OutOfMemory;
            if (val == 0) break;
        }
        return out.toOwnedSlice(allocator) catch return BitStreamError.OutOfMemory;
    }

    /// Decodes a ULEB128 integer from `bytes`.
    /// Rejects encodings exceeding 10 bytes or 64-bit range with `IntegerOverflow`,
    /// and truncated streams with `UnexpectedEof`.
    pub static fn decodeUleb128(bytes: []const u8): Uleb128Result ! BitStreamError {
        let result: u64 = 0;
        let shift: u7 = 0;
        let idx: usize = 0;

        while (idx < bytes.len) {
            if (idx >= 10) {
                return BitStreamError.IntegerOverflow;
            }
            const b = bytes[idx];
            idx += 1;
            const low: u64 = (b & 0x7F) as u64;

            if (idx == 10) {
                if ((b & 0x80) != 0 || low > 1) {
                    return BitStreamError.IntegerOverflow;
                }
            }

            result |= (low << (shift as u6));
            if ((b & 0x80) == 0) {
                return Uleb128Result{
                    value: result,
                    bytes_read: idx,
                };
            }
            shift += 7;
        }

        return BitStreamError.UnexpectedEof;
    }

    /// Encodes a signed 64-bit integer as SLEB128. Caller owns the returned slice.
    pub static fn encodeSleb128(allocator: std.mem.Allocator, v: i64): []u8 ! BitStreamError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);

        let val: i64 = v;
        let more = true;
        while (more) {
            let byte: u8 = @as(u8, @intCast(val & 0x7F));
            val >>= 7;
            const sign_bit = (byte & 0x40) != 0;
            if ((val == 0 && !sign_bit) || (val == -1 && sign_bit)) {
                more = false;
            } else {
                byte |= 0x80;
            }
            out.append(allocator, byte) catch return BitStreamError.OutOfMemory;
        }
        return out.toOwnedSlice(allocator) catch return BitStreamError.OutOfMemory;
    }

    /// Decodes an SLEB128 integer from `bytes` with proper sign extension.
    /// Rejects encodings exceeding 10 bytes or 64-bit signed range with `IntegerOverflow`,
    /// and truncated streams with `UnexpectedEof`.
    pub static fn decodeSleb128(bytes: []const u8): Sleb128Result ! BitStreamError {
        let result: u64 = 0;
        let shift: u7 = 0;
        let idx: usize = 0;

        while (idx < bytes.len) {
            if (idx >= 10) {
                return BitStreamError.IntegerOverflow;
            }
            const b = bytes[idx];
            idx += 1;
            const low: u64 = (b & 0x7F) as u64;

            if (idx == 10) {
                if ((b & 0x80) != 0) {
                    return BitStreamError.IntegerOverflow;
                }
                if (b != 0x00 && b != 0x7F) {
                    return BitStreamError.IntegerOverflow;
                }
            }

            result |= (low << (shift as u6));
            shift += 7;

            if ((b & 0x80) == 0) {
                if (shift < 64 && (b & 0x40) != 0) {
                    result |= (~(0 as u64)) << (shift as u6);
                }
                return Sleb128Result{
                    value: @as(i64, @bitCast(result)),
                    bytes_read: idx,
                };
            }
        }

        return BitStreamError.UnexpectedEof;
    }
}
