// Source: cp/lib/parquet/rle.cp (Parquet RLE / Bit-Packing Hybrid encoder and decoder)
const std = @import("std");
import { ParquetError } from "./types.cp";

fn writeVarintU32(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, value: u32): !void {
    let v = value;
    while (true) {
        if ((v & ~@as(u32, 0x7F)) == 0) {
            try buf.append(alloc, @as(u8, @intCast(v)));
            return;
        }
        try buf.append(alloc, @as(u8, @intCast((v & 0x7F) | 0x80)));
        v >>= 7;
    }
}

fn readVarintU32(data: []const u8, pos: *usize): !u32 {
    let result: u32 = 0;
    let shift: u5 = 0;
    let count: usize = 0;
    while (count < 5) : (count += 1) {
        if (pos.* >= data.len) {
            return ParquetError.TruncatedData;
        }
        const b = data[pos.*];
        pos.* += 1;
        result |= @as(u32, b & 0x7F) << shift;
        if ((b & 0x80) == 0) {
            return result;
        }
        if (shift >= 28) {
            return ParquetError.InvalidVarint;
        }
        shift += 7;
    }
    return ParquetError.InvalidVarint;
}

pub fn bitWidthForMaxVal(max_val: u32): u8 {
    if (max_val == 0) {
        return 0;
    }
    return @as(u8, @intCast(32 - @clz(max_val)));
}

fn flushBitPackedRun(
    buf: *std.ArrayList(u8),
    alloc: std.mem.Allocator,
    vals: []const u32,
    bit_width: u8,
): !void {
    if (vals.len == 0) {
        return;
    }
    const num_groups: usize = (vals.len + 7) / 8;
    const header: u32 = (@as(u32, @intCast(num_groups)) << 1) | 1;
    try writeVarintU32(buf, alloc, header);

    const total_bytes: usize = num_groups * @as(usize, bit_width);
    const start_len = buf.items.len;
    try buf.resize(alloc, start_len + total_bytes);
    @memset(buf.items[start_len..], 0);

    let bit_offset: usize = 0;
    let i: usize = 0;
    const bw: usize = @as(usize, bit_width);
    while (i < num_groups * 8) : (i += 1) {
        const v: u32 = if (i < vals.len) vals[i] else 0;
        let b: usize = 0;
        while (b < bw) : (b += 1) {
            if (((v >> @as(u5, @intCast(b))) & 1) != 0) {
                const abs_bit = bit_offset + b;
                const byte_idx = start_len + (abs_bit >> 3);
                const bit_in_byte: u3 = @intCast(abs_bit & 7);
                buf.items[byte_idx] |= (@as(u8, 1) << bit_in_byte);
            }
        }
        bit_offset += bw;
    }
}

fn flushRleRun(
    buf: *std.ArrayList(u8),
    alloc: std.mem.Allocator,
    val: u32,
    count: usize,
    bit_width: u8,
): !void {
    const header: u32 = @as(u32, @intCast(count)) << 1;
    try writeVarintU32(buf, alloc, header);
    const byte_width: usize = (@as(usize, bit_width) + 7) / 8;
    let i: usize = 0;
    while (i < byte_width) : (i += 1) {
        const shift: u5 = @intCast(i * 8);
        try buf.append(alloc, @as(u8, @intCast((val >> shift) & 0xFF)));
    }
}

pub fn encodeRleBitPacked(alloc: std.mem.Allocator, values: []const u32, bit_width: u8): ![]u8 {
    if (bit_width > 32) {
        return ParquetError.InvalidBitWidth;
    }
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);

    if (values.len == 0) {
        return buf.toOwnedSlice(alloc);
    }

    if (bit_width == 0) {
        try flushRleRun(&buf, alloc, 0, values.len, 0);
        return buf.toOwnedSlice(alloc);
    }

    let literal_buf = std.ArrayList(u32).empty;
    defer literal_buf.deinit(alloc);

    let i: usize = 0;
    while (i < values.len) {
        let run_len: usize = 1;
        while (i + run_len < values.len and values[i + run_len] == values[i]) {
            run_len += 1;
        }

        if (run_len >= 8) {
            if (literal_buf.items.len > 0) {
                try flushBitPackedRun(&buf, alloc, literal_buf.items, bit_width);
                literal_buf.clearRetainingCapacity();
            }
            try flushRleRun(&buf, alloc, values[i], run_len, bit_width);
            i += run_len;
        } else {
            let k: usize = 0;
            while (k < run_len) : (k += 1) {
                try literal_buf.append(alloc, values[i + k]);
                if (literal_buf.items.len == 504) {
                    try flushBitPackedRun(&buf, alloc, literal_buf.items, bit_width);
                    literal_buf.clearRetainingCapacity();
                }
            }
            i += run_len;
        }
    }

    if (literal_buf.items.len > 0) {
        try flushBitPackedRun(&buf, alloc, literal_buf.items, bit_width);
    }

    return buf.toOwnedSlice(alloc);
}

pub fn decodeRleBitPacked(
    alloc: std.mem.Allocator,
    data: []const u8,
    bit_width: u8,
    num_values: usize,
): ![]u32 {
    if (bit_width > 32) {
        return ParquetError.InvalidBitWidth;
    }
    let out = try alloc.alloc(u32, num_values);
    errdefer alloc.free(out);

    if (num_values == 0) {
        return out;
    }

    let pos: usize = 0;
    let out_idx: usize = 0;
    const bw: usize = @as(usize, bit_width);
    const byte_width: usize = (bw + 7) / 8;

    while (out_idx < num_values) {
        const header = try readVarintU32(data, &pos);
        if ((header & 1) == 0) {
            const run_len: usize = @intCast(header >> 1);
            if (run_len == 0) {
                return ParquetError.InvalidRleStream;
            }
            if (byte_width > data.len - pos) {
                return ParquetError.TruncatedData;
            }
            let val: u32 = 0;
            let b: usize = 0;
            while (b < byte_width) : (b += 1) {
                val |= @as(u32, data[pos + b]) << @as(u5, @intCast(b * 8));
            }
            pos += byte_width;

            const to_write = @min(run_len, num_values - out_idx);
            let k: usize = 0;
            while (k < to_write) : (k += 1) {
                out[out_idx + k] = val;
            }
            out_idx += to_write;
        } else {
            const num_groups: usize = @intCast(header >> 1);
            if (num_groups == 0) {
                return ParquetError.InvalidRleStream;
            }
            const total_bytes = num_groups * bw;
            if (total_bytes > data.len - pos) {
                return ParquetError.TruncatedData;
            }
            const total_vals = num_groups * 8;
            const to_write = @min(total_vals, num_values - out_idx);

            let bit_offset: usize = 0;
            let k: usize = 0;
            while (k < to_write) : (k += 1) {
                let val: u32 = 0;
                let b: usize = 0;
                while (b < bw) : (b += 1) {
                    const abs_bit = bit_offset + b;
                    const byte_idx = pos + (abs_bit >> 3);
                    const bit_in_byte: u3 = @intCast(abs_bit & 7);
                    if (((data[byte_idx] >> bit_in_byte) & 1) != 0) {
                        val |= (@as(u32, 1) << @as(u5, @intCast(b)));
                    }
                }
                out[out_idx + k] = val;
                bit_offset += bw;
            }
            pos += total_bytes;
            out_idx += to_write;
        }
    }

    return out;
}
