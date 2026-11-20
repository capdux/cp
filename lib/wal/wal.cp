// Source: cp/lib/wal/wal.cp (CRC32C Write-Ahead Log Framing, Recovery & Compaction Engine)
// Pure CP zero-C-dependency Write-Ahead Log framing, Castagnoli CRC32C integrity verification,
// strict & tail-truncation crash recovery, and LSN-based log compaction.
//
// Frame layout (24-byte fixed little-endian header + payload):
//   [magic:4 ("WAL1")][lsn:u64 LE][payload_len:u32 LE][record_type:u8][reserved:3][crc32c:u32 LE][payload:N]
// CRC32C (Castagnoli 0x82F63B78) covers header[0..20] + payload[0..N].

const std = @import("std");

pub const WAL_MAGIC: [4]u8 = ['W', 'A', 'L', '1'];
pub const WAL_HEADER_SIZE: usize = 24;
pub const WAL_DEFAULT_MAX_RECORD_BYTES: usize = 16 * 1024 * 1024;

pub err WalError {
    CorruptedFrame,
    ChecksumMismatch,
    TruncatedTail,
    SequenceOutOfOrder,
    PayloadTooLarge,
    OutOfMemory,
}

pub enum WalRecordType: u8 {
    Data = 1,
    Checkpoint = 2,
    Tombstone = 3,
}

fn recordTypeToU8(rt: WalRecordType): u8 {
    return switch (rt) {
        WalRecordType.Data => 1,
        WalRecordType.Checkpoint => 2,
        WalRecordType.Tombstone => 3,
    };
}

fn recordTypeFromU8(v: u8): WalRecordType ! WalError {
    if (v == 1) return WalRecordType.Data;
    if (v == 2) return WalRecordType.Checkpoint;
    if (v == 3) return WalRecordType.Tombstone;
    return WalError.CorruptedFrame;
}

fn makeCrc32cTable(): [256]u32 {
    let table: [256]u32 = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        let crc: u32 = i as u32;
        let bit: usize = 0;
        while (bit < 8) : (bit += 1) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0x82F63B78;
            } else {
                crc = crc >> 1;
            }
        }
        table[i] = crc;
    }
    return table;
}

const CRC32C_TABLE: [256]u32 = makeCrc32cTable();

pub fn crc32c(data: []const u8): u32 {
    return crc32cUpdate(0, data);
}

fn crc32cUpdate(initial: u32, data: []const u8): u32 {
    let crc: u32 = initial ^ 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        const idx: usize = ((crc ^ (data[i] as u32)) & 0xFF) as usize;
        crc = CRC32C_TABLE[idx] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

fn writeU32Le(buf: []u8, v: u32) {
    buf[0] = (v & 0xFF) as u8;
    buf[1] = ((v >> 8) & 0xFF) as u8;
    buf[2] = ((v >> 16) & 0xFF) as u8;
    buf[3] = ((v >> 24) & 0xFF) as u8;
}

fn writeU64Le(buf: []u8, v: u64) {
    buf[0] = (v & 0xFF) as u8;
    buf[1] = ((v >> 8) & 0xFF) as u8;
    buf[2] = ((v >> 16) & 0xFF) as u8;
    buf[3] = ((v >> 24) & 0xFF) as u8;
    buf[4] = ((v >> 32) & 0xFF) as u8;
    buf[5] = ((v >> 40) & 0xFF) as u8;
    buf[6] = ((v >> 48) & 0xFF) as u8;
    buf[7] = ((v >> 56) & 0xFF) as u8;
}

fn readU32Le(buf: []const u8): u32 {
    return (buf[0] as u32) |
        ((buf[1] as u32) << 8) |
        ((buf[2] as u32) << 16) |
        ((buf[3] as u32) << 24);
}

fn readU64Le(buf: []const u8): u64 {
    return (buf[0] as u64) |
        ((buf[1] as u64) << 8) |
        ((buf[2] as u64) << 16) |
        ((buf[3] as u64) << 24) |
        ((buf[4] as u64) << 32) |
        ((buf[5] as u64) << 40) |
        ((buf[6] as u64) << 48) |
        ((buf[7] as u64) << 56);
}

fn computeFrameCrc32c(header_prefix: []const u8, payload: []const u8): u32 {
    const h_crc = crc32cUpdate(0, header_prefix);
    return crc32cUpdate(h_crc, payload);
}

fn hasValidMagic(buf: []const u8): bool {
    return buf[0] == WAL_MAGIC[0] &&
        buf[1] == WAL_MAGIC[1] &&
        buf[2] == WAL_MAGIC[2] &&
        buf[3] == WAL_MAGIC[3];
}

struct ByteBuf {
    alloc: std.mem.Allocator = undefined,
    data: []u8 = [],
    len: usize = 0,

    static fn init(alloc: std.mem.Allocator): ByteBuf {
        return ByteBuf{
            alloc: alloc,
            data: [],
            len: 0,
        };
    }

    fn deinit(*_) {
        if (_.data.len > 0) {
            _.alloc.free(_.data);
            _.data = [];
        }
        _.len = 0;
    }

    fn ensureCapacity(*_, needed: usize): ! WalError {
        if (needed <= _.data.len) {
            return;
        }
        let new_cap: usize = if (_.data.len == 0) 128 else _.data.len;
        while (new_cap < needed) {
            if (new_cap > (usize.max / 2)) {
                new_cap = needed;
                break;
            }
            new_cap = new_cap * 2;
        }
        const new_buf = _.alloc.alloc(u8, new_cap) catch return WalError.OutOfMemory;
        if (_.len > 0) {
            std.mem.copyForwards(u8, new_buf[0.._.len], _.data[0.._.len]);
        }
        if (_.data.len > 0) {
            _.alloc.free(_.data);
        }
        _.data = new_buf;
    }

    fn appendSlice(*_, s: []const u8): ! WalError {
        if (s.len == 0) {
            return;
        }
        _.ensureCapacity(_.len + s.len)?;
        std.mem.copyForwards(u8, _.data[_.len .. _.len + s.len], s);
        _.len += s.len;
    }

    fn slice(): []const u8 {
        return _.data[0.._.len];
    }
}

pub struct WalRecord {
    lsn: u64 = 0,
    record_type: WalRecordType = WalRecordType.Data,
    payload: []const u8 = "",
}

struct RecordVec {
    alloc: std.mem.Allocator = undefined,
    items: []WalRecord = [],
    len: usize = 0,

    static fn init(alloc: std.mem.Allocator): RecordVec {
        return RecordVec{
            alloc: alloc,
            items: [],
            len: 0,
        };
    }

    fn deinitWithPayloads(*_) {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (_.items[i].payload.len > 0) {
                _.alloc.free(_.items[i].payload);
            }
        }
        if (_.items.len > 0) {
            _.alloc.free(_.items);
            _.items = [];
        }
        _.len = 0;
    }

    fn push(*_, rec: WalRecord): ! WalError {
        if (_.len == _.items.len) {
            let new_cap: usize = if (_.items.len == 0) 16 else _.items.len * 2;
            const new_items = _.alloc.alloc(WalRecord, new_cap) catch return WalError.OutOfMemory;
            if (_.len > 0) {
                std.mem.copyForwards(WalRecord, new_items[0.._.len], _.items[0.._.len]);
            }
            if (_.items.len > 0) {
                _.alloc.free(_.items);
            }
            _.items = new_items;
        }
        _.items[_.len] = rec;
        _.len += 1;
    }

    fn toOwnedSlice(*_): ([]WalRecord) ! WalError {
        if (_.len == 0) {
            if (_.items.len > 0) {
                _.alloc.free(_.items);
                _.items = [];
            }
            return [];
        }
        const out = _.alloc.alloc(WalRecord, _.len) catch return WalError.OutOfMemory;
        std.mem.copyForwards(WalRecord, out, _.items[0.._.len]);
        _.alloc.free(_.items);
        _.items = [];
        _.len = 0;
        return out;
    }
}

pub struct WalRecoveryResult {
    records: []WalRecord = [],
    valid_bytes: usize = 0,
    truncated_tail_bytes: usize = 0,
    last_lsn: u64 = 0,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.records.len) : (i += 1) {
            if (_.records[i].payload.len > 0) {
                alloc.free(_.records[i].payload);
            }
        }
        if (_.records.len > 0) {
            alloc.free(_.records);
        }
        _.records = [];
        _.valid_bytes = 0;
        _.truncated_tail_bytes = 0;
        _.last_lsn = 0;
    }
}

pub struct WalLog {
    alloc: std.mem.Allocator = undefined,
    max_record_bytes: usize = WAL_DEFAULT_MAX_RECORD_BYTES,
    buf: ByteBuf = ByteBuf{},
    last_lsn: u64 = 0,
    record_count: usize = 0,

    pub static fn init(alloc: std.mem.Allocator, max_record_bytes: usize): WalLog {
        return WalLog{
            alloc: alloc,
            max_record_bytes: max_record_bytes,
            buf: ByteBuf.init(alloc),
            last_lsn: 0,
            record_count: 0,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit();
        _.last_lsn = 0;
        _.record_count = 0;
    }

    pub fn bytes(): []const u8 {
        return _.buf.slice();
    }

    pub fn lastLsn(): u64 {
        return _.last_lsn;
    }

    pub fn recordCount(): usize {
        return _.record_count;
    }

    pub fn byteLen(): usize {
        return _.buf.len;
    }

    pub fn append(*_, payload: []const u8): u64 ! WalError {
        return _.appendTyped(WalRecordType.Data, payload);
    }

    pub fn appendTyped(*_, record_type: WalRecordType, payload: []const u8): u64 ! WalError {
        const next_lsn: u64 = _.last_lsn + 1;
        return _.appendWithLsn(next_lsn, record_type, payload);
    }

    pub fn appendWithLsn(*_, lsn: u64, record_type: WalRecordType, payload: []const u8): u64 ! WalError {
        if (lsn == 0 || lsn <= _.last_lsn) {
            return WalError.SequenceOutOfOrder;
        }
        const max_u32: usize = std.math.maxInt(u32) as usize;
        if (payload.len > _.max_record_bytes || payload.len > max_u32) {
            return WalError.PayloadTooLarge;
        }

        const total_len: usize = WAL_HEADER_SIZE + payload.len;
        _.buf.ensureCapacity(_.buf.len + total_len)?;

        let hdr: [24]u8 = [0; 24];
        hdr[0] = WAL_MAGIC[0];
        hdr[1] = WAL_MAGIC[1];
        hdr[2] = WAL_MAGIC[2];
        hdr[3] = WAL_MAGIC[3];
        writeU64Le(hdr[4..12], lsn);
        writeU32Le(hdr[12..16], payload.len as u32);
        hdr[16] = recordTypeToU8(record_type);
        hdr[17] = 0;
        hdr[18] = 0;
        hdr[19] = 0;

        const crc: u32 = computeFrameCrc32c(hdr[0..20], payload);
        writeU32Le(hdr[20..24], crc);

        _.buf.appendSlice(hdr[0..24])?;
        _.buf.appendSlice(payload)?;
        _.last_lsn = lsn;
        _.record_count += 1;
        return lsn;
    }

    pub fn appendBatch(*_, payloads: []const []const u8): u64 ! WalError {
        const max_u32: usize = std.math.maxInt(u32) as usize;
        let total_needed: usize = 0;
        let i: usize = 0;
        while (i < payloads.len) : (i += 1) {
            const p = payloads[i];
            if (p.len > _.max_record_bytes || p.len > max_u32) {
                return WalError.PayloadTooLarge;
            }
            total_needed += WAL_HEADER_SIZE + p.len;
        }
        _.buf.ensureCapacity(_.buf.len + total_needed)?;

        i = 0;
        while (i < payloads.len) : (i += 1) {
            * = _.appendTyped(WalRecordType.Data, payloads[i])?;
        }
        return _.last_lsn;
    }

    pub fn compactBefore(*_, alloc: std.mem.Allocator, min_lsn: u64): ! WalError {
        const raw = _.buf.slice();
        if (raw.len == 0) {
            return;
        }

        let new_buf = ByteBuf.init(alloc);
        errdefer new_buf.deinit();

        let pos: usize = 0;
        let kept_count: usize = 0;
        let prev_lsn: u64 = 0;
        let has_prev: bool = false;

        while (pos < raw.len) {
            if (raw.len - pos < WAL_HEADER_SIZE) {
                return WalError.TruncatedTail;
            }
            const hdr = raw[pos .. pos + WAL_HEADER_SIZE];
            if (!hasValidMagic(hdr)) {
                return WalError.CorruptedFrame;
            }
            const lsn = readU64Le(hdr[4..12]);
            const payload_len: usize = readU32Le(hdr[12..16]) as usize;
            const frame_end = pos + WAL_HEADER_SIZE;
            if (payload_len > raw.len - frame_end) {
                return WalError.TruncatedTail;
            }
            const payload = raw[frame_end .. frame_end + payload_len];
            const expected_crc = readU32Le(hdr[20..24]);
            const actual_crc = computeFrameCrc32c(hdr[0..20], payload);
            if (actual_crc != expected_crc) {
                return WalError.ChecksumMismatch;
            }
            * = recordTypeFromU8(hdr[16])?;
            if (lsn == 0 || (has_prev && lsn <= prev_lsn)) {
                return WalError.SequenceOutOfOrder;
            }
            prev_lsn = lsn;
            has_prev = true;

            const total_frame_len = WAL_HEADER_SIZE + payload_len;
            if (lsn >= min_lsn) {
                new_buf.appendSlice(raw[pos .. pos + total_frame_len])?;
                kept_count += 1;
            }
            pos += total_frame_len;
        }

        _.buf.deinit();
        _.buf = new_buf;
        _.record_count = kept_count;
    }

    pub static fn recoverStrict(alloc: std.mem.Allocator, raw_bytes: []const u8): WalRecoveryResult ! WalError {
        return recoverInternal(alloc, raw_bytes, false);
    }

    pub static fn recoverWithTailTruncation(alloc: std.mem.Allocator, raw_bytes: []const u8): WalRecoveryResult ! WalError {
        return recoverInternal(alloc, raw_bytes, true);
    }

    pub static fn restoreTruncated(
        alloc: std.mem.Allocator,
        max_record_bytes: usize,
        raw_bytes: []const u8,
    ): WalLog ! WalError {
        let res = WalLog.recoverWithTailTruncation(alloc, raw_bytes)?;
        defer res.deinit(alloc);

        let log = WalLog.init(alloc, max_record_bytes);
        errdefer log.deinit();

        if (res.valid_bytes > 0) {
            log.buf.appendSlice(raw_bytes[0..res.valid_bytes])?;
        }
        log.last_lsn = res.last_lsn;
        log.record_count = res.records.len;
        return log;
    }
}

fn recoverInternal(
    alloc: std.mem.Allocator,
    raw_bytes: []const u8,
    allow_tail_truncation: bool,
): WalRecoveryResult ! WalError {
    let vec = RecordVec.init(alloc);
    errdefer vec.deinitWithPayloads();

    let pos: usize = 0;
    let last_lsn: u64 = 0;
    let has_prev: bool = false;

    while (pos < raw_bytes.len) {
        if (raw_bytes.len - pos < WAL_HEADER_SIZE) {
            if (allow_tail_truncation) {
                break;
            }
            return WalError.TruncatedTail;
        }

        const hdr = raw_bytes[pos .. pos + WAL_HEADER_SIZE];
        if (!hasValidMagic(hdr)) {
            if (allow_tail_truncation) {
                break;
            }
            return WalError.CorruptedFrame;
        }

        const lsn = readU64Le(hdr[4..12]);
        const payload_len: usize = readU32Le(hdr[12..16]) as usize;
        const payload_start = pos + WAL_HEADER_SIZE;
        if (payload_len > raw_bytes.len - payload_start) {
            if (allow_tail_truncation) {
                break;
            }
            return WalError.TruncatedTail;
        }

        const payload = raw_bytes[payload_start .. payload_start + payload_len];
        const expected_crc = readU32Le(hdr[20..24]);
        const actual_crc = computeFrameCrc32c(hdr[0..20], payload);
        if (actual_crc != expected_crc) {
            if (allow_tail_truncation) {
                break;
            }
            return WalError.ChecksumMismatch;
        }

        const rt = recordTypeFromU8(hdr[16]) catch |e| {
            if (allow_tail_truncation) {
                break;
            }
            return e;
        };

        if (lsn == 0 || (has_prev && lsn <= last_lsn)) {
            return WalError.SequenceOutOfOrder;
        }

        const owned_payload: []const u8 = if (payload.len > 0)
            (alloc.dupe(u8, payload) catch return WalError.OutOfMemory)
        else
            "";
        errdefer if (owned_payload.len > 0) alloc.free(owned_payload);

        vec.push(WalRecord{
            lsn: lsn,
            record_type: rt,
            payload: owned_payload,
        })?;

        last_lsn = lsn;
        has_prev = true;
        pos = payload_start + payload_len;
    }

    const owned_records = vec.toOwnedSlice()?;
    return WalRecoveryResult{
        records: owned_records,
        valid_bytes: pos,
        truncated_tail_bytes: raw_bytes.len - pos,
        last_lsn: last_lsn,
    };
}
