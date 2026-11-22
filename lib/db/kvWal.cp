// Source: cp/lib/kv/wal.cp (CRC32-Protected Append-Only WAL Record Codec)
// Pure CP Bitcask-style Write-Ahead Log record framing and IEEE 802.3 CRC32 codec.
// Frame layout:
//   [magic: 4B "CPKV"][crc32: u32 LE][op: u8][key_len: u32 LE][val_len: u32 LE][key][val]
// CRC32 protects bytes [op + key_len + val_len + key + val].

const std = @import("std");

pub const WAL_MAGIC: [4]u8 = ['C', 'P', 'K', 'V'];
pub const WAL_HEADER_SIZE: usize = 17;

pub err KvError {
    IoError,
    CorruptRecord,
    InvalidKey,
    OutOfMemory,
}

pub enum WalOp: u8 {
    Put = 1,
    Delete = 2,
}

pub struct WalRecord {
    op: WalOp = .Put,
    key: string = "",
    value: string = "",

    pub static fn put(key: string, value: string): WalRecord {
        return WalRecord{ op: .Put, key: key, value: value };
    }

    pub static fn del(key: string): WalRecord {
        return WalRecord{ op: .Delete, key: key, value: "" };
    }
}

pub struct WalFrame {
    record: WalRecord = WalRecord{},
    next_offset: usize = 0,
}

pub enum WalDecodeResult {
    Record: WalFrame,
    Eof,
    TornTail,
    CorruptRecord,

    pub fn isEof(): bool {
        return switch (_) {
            .Eof => true,
            else => false,
        };
    }

    pub fn isTornTail(): bool {
        return switch (_) {
            .TornTail => true,
            else => false,
        };
    }

    pub fn isCorrupt(): bool {
        return switch (_) {
            .CorruptRecord => true,
            else => false,
        };
    }

    pub fn asRecord(): WalFrame? {
        return switch (_) {
            .Record frame => frame,
            else => null,
        };
    }
}

fn writeU32LE(dst: []u8, val: u32) {
    dst[0] = (val & 0xFF) as u8;
    dst[1] = ((val >> 8) & 0xFF) as u8;
    dst[2] = ((val >> 16) & 0xFF) as u8;
    dst[3] = ((val >> 24) & 0xFF) as u8;
}

fn readU32LE(src: []const u8): u32 {
    return (src[0] as u32) |
        ((src[1] as u32) << 8) |
        ((src[2] as u32) << 16) |
        ((src[3] as u32) << 24);
}

pub static struct WalCodec {
    /// Computes IEEE 802.3 CRC32 checksum over the given byte slice.
    pub fn crc32(data: []const u8): u32 {
        return std.hash.Crc32.hash(data);
    }

    /// Encodes a `WalRecord` into a newly allocated CRC32-protected binary frame.
    pub fn encode(alloc: std.mem.Allocator, rec: WalRecord): ([]u8)! {
        const max_u32: usize = std.math.maxInt(u32) as usize;
        if (rec.key.len == 0 || rec.key.len > max_u32) {
            return KvError.InvalidKey;
        }
        if (rec.value.len > max_u32) {
            return KvError.InvalidKey;
        }

        const total_len: usize = WAL_HEADER_SIZE + rec.key.len + rec.value.len;
        let buf = alloc.alloc(u8, total_len) catch return KvError.OutOfMemory;

        buf[0] = WAL_MAGIC[0];
        buf[1] = WAL_MAGIC[1];
        buf[2] = WAL_MAGIC[2];
        buf[3] = WAL_MAGIC[3];

        buf[8] = rec.op as u8;
        writeU32LE(buf[9..13], rec.key.len as u32);
        writeU32LE(buf[13..17], rec.value.len as u32);

        if (rec.key.len > 0) {
            @memcpy(buf[17 .. 17 + rec.key.len], rec.key);
        }
        if (rec.value.len > 0) {
            @memcpy(buf[17 + rec.key.len .. total_len], rec.value);
        }

        const sum = std.hash.Crc32.hash(buf[8..total_len]);
        writeU32LE(buf[4..8], sum);
        return buf;
    }

    /// Decodes the next WAL record starting at `offset` in `bytes`.
    /// Precisely distinguishes normal EOF, partial tail truncation (`TornTail`),
    /// and mid-stream / magic / op / CRC32 corruption (`CorruptRecord`).
    pub fn decodeNext(bytes: []const u8, offset: usize): WalDecodeResult {
        if (offset >= bytes.len) {
            return .Eof;
        }

        const rem = bytes.len - offset;
        if (rem < 4) {
            let i: usize = 0;
            while (i < rem) : (i += 1) {
                if (bytes[offset + i] != WAL_MAGIC[i]) {
                    return .CorruptRecord;
                }
            }
            return .TornTail;
        }

        if (bytes[offset] != WAL_MAGIC[0] ||
            bytes[offset + 1] != WAL_MAGIC[1] ||
            bytes[offset + 2] != WAL_MAGIC[2] ||
            bytes[offset + 3] != WAL_MAGIC[3])
        {
            return .CorruptRecord;
        }

        if (rem < WAL_HEADER_SIZE) {
            return .TornTail;
        }

        const expected_crc = readU32LE(bytes[offset + 4 .. offset + 8]);
        const op_raw = bytes[offset + 8];
        let op: WalOp = .Put;
        if (op_raw == 1) {
            op = .Put;
        } else if (op_raw == 2) {
            op = .Delete;
        } else {
            return .CorruptRecord;
        }

        const key_len = readU32LE(bytes[offset + 9 .. offset + 13]) as usize;
        const val_len = readU32LE(bytes[offset + 13 .. offset + 17]) as usize;
        if (key_len == 0) {
            return .CorruptRecord;
        }

        const frame_len_u64: u64 = (WAL_HEADER_SIZE as u64) + (key_len as u64) + (val_len as u64);
        if (frame_len_u64 > (rem as u64)) {
            return .TornTail;
        }

        const frame_len: usize = frame_len_u64 as usize;
        const actual_crc = std.hash.Crc32.hash(bytes[offset + 8 .. offset + frame_len]);
        if (actual_crc != expected_crc) {
            return .CorruptRecord;
        }

        const key_start = offset + WAL_HEADER_SIZE;
        const val_start = key_start + key_len;
        const key_slice = bytes[key_start..val_start];
        const val_slice = bytes[val_start .. offset + frame_len];

        return WalDecodeResult.Record(WalFrame{
            record: WalRecord{
                op: op,
                key: key_slice,
                value: val_slice,
            },
            next_offset: offset + frame_len,
        });
    }
}
