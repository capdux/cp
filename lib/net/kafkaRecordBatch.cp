// Reference-hardened against tychedelia/kafka-protocol-rs (MIT/Apache-2.0)
// Source: cp/lib/kafka_wire/record_batch.cp (Kafka RecordBatch v2 magic=2 & CRC32C Castagnoli)
const std = @import("std");
import {
    KAFKA_MAX_FRAME_LEN,
    KAFKA_MAX_ARRAY_LEN,
    KafkaError,
    KafkaReader,
    KafkaWriter,
} from "./kafkaWire.cp";

/// Pure-CP Castagnoli CRC32C (polynomial 0x82F63B78, RFC 3720 / Kafka v2 RecordBatch).
pub fn kafkaCrc32c(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        crc ^= @as(u32, data[i]);
        let bit: usize = 0;
        while (bit < 8) : (bit += 1) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0x82F63B78;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

pub struct RecordHeader {
    key: []const u8,
    value: ([]const u8)?,
}

pub struct Record {
    attributes: i8,
    timestamp_delta: i64,
    offset_delta: i32,
    key: ([]const u8)?,
    value: ([]const u8)?,
    headers: []const RecordHeader,
}

pub struct RecordBatch {
    base_offset: i64,
    partition_leader_epoch: i32,
    attributes: i16,
    last_offset_delta: i32,
    base_timestamp: i64,
    max_timestamp: i64,
    producer_id: i64,
    producer_epoch: i16,
    base_sequence: i32,
    records: []const Record,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        for (rec of _.records) {
            alloc.free(rec.headers);
        }
        alloc.free(_.records);
    }
}

pub fn encodeRecordBatch(alloc: std.mem.Allocator, batch: *const RecordBatch): KafkaError![]u8 {
    let recs_writer = KafkaWriter.init(alloc);
    defer recs_writer.deinit();

    let rec_buf = KafkaWriter.init(alloc);
    defer rec_buf.deinit();

    for (rec of batch.records) {
        rec_buf.buf.items.len = 0;
        rec_buf.writeI8(rec.attributes)?;
        rec_buf.writeVarlong(rec.timestamp_delta)?;
        rec_buf.writeVarint(rec.offset_delta)?;
        rec_buf.writeVarintBytes(rec.key)?;
        rec_buf.writeVarintBytes(rec.value)?;
        rec_buf.writeVarint(@as(i32, @intCast(rec.headers.len)))?;
        for (hdr of rec.headers) {
            rec_buf.writeVarint(@as(i32, @intCast(hdr.key.len)))?;
            rec_buf.writeBytes(hdr.key)?;
            rec_buf.writeVarintBytes(hdr.value)?;
        }
        const rec_slice = rec_buf.items();
        recs_writer.writeVarint(@as(i32, @intCast(rec_slice.len)))?;
        recs_writer.writeBytes(rec_slice)?;
    }

    const records_bytes = recs_writer.items();
    if (49 + records_bytes.len > KAFKA_MAX_FRAME_LEN) {
        return KafkaError.FrameTooLarge;
    }
    const batch_length: i32 = @as(i32, @intCast(49 + records_bytes.len));

    let w = KafkaWriter.init(alloc);
    errdefer w.deinit();

    w.writeI64(batch.base_offset)?;
    w.writeI32(batch_length)?;
    w.writeI32(batch.partition_leader_epoch)?;
    w.writeI8(2)?;
    w.writeU32(0)?;
    w.writeI16(batch.attributes)?;
    w.writeI32(batch.last_offset_delta)?;
    w.writeI64(batch.base_timestamp)?;
    w.writeI64(batch.max_timestamp)?;
    w.writeI64(batch.producer_id)?;
    w.writeI16(batch.producer_epoch)?;
    w.writeI32(batch.base_sequence)?;
    w.writeI32(@as(i32, @intCast(batch.records.len)))?;
    w.writeBytes(records_bytes)?;

    const out = w.toOwnedSlice()?;
    const crc = kafkaCrc32c(out[21..]);
    std.mem.writeInt(u32, out[17..21], crc, .big);
    return out;
}

pub fn decodeRecordBatch(alloc: std.mem.Allocator, data: []const u8): KafkaError!RecordBatch {
    if (data.len < 61) {
        return KafkaError.UnexpectedEof;
    }

    let r = KafkaReader.init(data);
    const base_offset = r.readI64()?;
    const batch_length = r.readI32()?;
    if (batch_length < 49) {
        return KafkaError.InvalidLength;
    }
    const total_len: usize = @as(usize, 12) + @as(usize, @intCast(batch_length));
    if (total_len > data.len) {
        return KafkaError.UnexpectedEof;
    }

    const partition_leader_epoch = r.readI32()?;
    const magic = r.readI8()?;
    if (magic != 2) {
        return KafkaError.InvalidMagic;
    }

    const expected_crc = r.readU32()?;
    const actual_crc = kafkaCrc32c(data[21..total_len]);
    if (actual_crc != expected_crc) {
        return KafkaError.CrcMismatch;
    }

    const attributes = r.readI16()?;
    const last_offset_delta = r.readI32()?;
    const base_timestamp = r.readI64()?;
    const max_timestamp = r.readI64()?;
    const producer_id = r.readI64()?;
    const producer_epoch = r.readI16()?;
    const base_sequence = r.readI32()?;
    const records_count_raw = r.readI32()?;
    if (records_count_raw < 0) {
        return KafkaError.InvalidLength;
    }
    const records_count: usize = @as(usize, @intCast(records_count_raw));
    if (records_count > KAFKA_MAX_ARRAY_LEN || records_count > (total_len - 61)) {
        return KafkaError.InvalidLength;
    }

    let recs_reader = KafkaReader.init(data[61..total_len]);
    let records = std.ArrayList(Record).empty;
    errdefer {
        for (rec of records.items) {
            alloc.free(rec.headers);
        }
        records.deinit(alloc);
    }

    let i: usize = 0;
    while (i < records_count) {
        const rec_len_raw = recs_reader.readVarint()?;
        if (rec_len_raw < 0) {
            return KafkaError.InvalidLength;
        }
        const rec_slice = recs_reader.readBytes(@as(usize, @intCast(rec_len_raw)))?;
        let rr = KafkaReader.init(rec_slice);

        const r_attr = rr.readI8()?;
        const ts_delta = rr.readVarlong()?;
        const off_delta = rr.readVarint()?;
        const key = rr.readVarintBytes() catch (e) { return e; };
        const value = rr.readVarintBytes() catch (e) { return e; };
        const hdr_count_raw = rr.readVarint()?;
        if (hdr_count_raw < 0) {
            return KafkaError.InvalidLength;
        }
        const hdr_count: usize = @as(usize, @intCast(hdr_count_raw));
        if (hdr_count > KAFKA_MAX_ARRAY_LEN || hdr_count > rr.remaining()) {
            return KafkaError.InvalidLength;
        }

        let hdrs = std.ArrayList(RecordHeader).empty;
        errdefer hdrs.deinit(alloc);

        let hi: usize = 0;
        while (hi < hdr_count) {
            const hk_len_raw = rr.readVarint()?;
            if (hk_len_raw < 0) {
                return KafkaError.InvalidLength;
            }
            const hk = rr.readBytes(@as(usize, @intCast(hk_len_raw)))?;
            const hv = rr.readVarintBytes() catch (e) { return e; };
            hdrs.append(alloc, RecordHeader{
                .key = hk,
                .value = hv,
            }) catch return KafkaError.OutOfMemory;
            hi += 1;
        }

        const owned_hdrs = hdrs.toOwnedSlice(alloc) catch return KafkaError.OutOfMemory;
        records.append(alloc, Record{
            .attributes = r_attr,
            .timestamp_delta = ts_delta,
            .offset_delta = off_delta,
            .key = key,
            .value = value,
            .headers = owned_hdrs,
        }) catch return KafkaError.OutOfMemory;
        i += 1;
    }

    const owned_records = records.toOwnedSlice(alloc) catch return KafkaError.OutOfMemory;
    return RecordBatch{
        .base_offset = base_offset,
        .partition_leader_epoch = partition_leader_epoch,
        .attributes = attributes,
        .last_offset_delta = last_offset_delta,
        .base_timestamp = base_timestamp,
        .max_timestamp = max_timestamp,
        .producer_id = producer_id,
        .producer_epoch = producer_epoch,
        .base_sequence = base_sequence,
        .records = owned_records,
    };
}
