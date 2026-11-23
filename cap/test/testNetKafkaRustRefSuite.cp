// Reference-hardened test suite against tychedelia/kafka-protocol-rs (MIT/Apache-2.0) & Apache Kafka Wire Spec
// Covers:
//   1. ZigZag Varint 32/64 extremes (0, -1, 1, i32::MIN/MAX, i64::MIN/MAX), 6/11-byte overflow & Nullable (-1 vs -2/i32::MIN)
//   2. ApiVersions v3 & Metadata v9 encode/decode + KafkaStreamFramer coalesced & 1-byte incremental reassembly + negative/oversize frame rejection
//   3. RecordBatch v2 (magic=2) encode/decode, Castagnoli CRC32C single-bit tamper rejection, bad magic & forged huge records_count guard
//   4. 16-lane @Vector(16, u8) SIMD simdKafkaCopyBytes & simdKafkaBytesEql 0-diff parity vs scalar
//   5. Deterministic PRNG Fuzz (>= 1,600 malformed Kafka frames / RecordBatch streams & random SIMD parity)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    KAFKA_MAX_FRAME_LEN,
    KafkaError,
    simdKafkaCopyBytes,
    simdKafkaCopyBytesScalar,
    simdKafkaBytesEql,
    simdKafkaBytesEqlScalar,
    KafkaReader,
    KafkaWriter,
} from "../../lib/net/kafkaWire.cp";
import {
    ApiKey,
    RequestHeader,
    ApiVersionRange,
    ApiVersionsResp,
    MetadataReq,
    KafkaStreamFramer,
} from "../../lib/net/kafkaProtocol.cp";
import {
    kafkaCrc32c,
    RecordHeader,
    Record,
    RecordBatch,
    encodeRecordBatch,
    decodeRecordBatch,
} from "../../lib/net/kafkaRecordBatch.cp";

fn nextRng(state: *u64): u64 {
    let x = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

fn readVarintExpectErr(raw: []const u8, expected: KafkaError): bool {
    let r = KafkaReader.init(raw);
    * = r.readVarint() catch (e) {
        return e == expected;
    };
    return false;
}

fn readVarlongExpectErr(raw: []const u8, expected: KafkaError): bool {
    let r = KafkaReader.init(raw);
    * = r.readVarlong() catch (e) {
        return e == expected;
    };
    return false;
}

fn readNullableStrExpectErr(raw: []const u8, expected: KafkaError): bool {
    let r = KafkaReader.init(raw);
    * = r.readNullableString() catch (e) {
        return e == expected;
    };
    return false;
}

fn readNullableBytesExpectErr(raw: []const u8, expected: KafkaError): bool {
    let r = KafkaReader.init(raw);
    * = r.readNullableBytes() catch (e) {
        return e == expected;
    };
    return false;
}

fn decodeBatchExpectErr(alloc: std.mem.Allocator, raw: []const u8, expected: KafkaError): bool {
    let b = decodeRecordBatch(alloc, raw) catch (e) {
        return e == expected;
    };
    b.deinit(alloc);
    return false;
}

fn framerExpectErr(framer: *KafkaStreamFramer, raw: []const u8, expected: KafkaError): bool {
    framer.reset();
    framer.push(raw) catch (e) {
        return e == expected;
    };
    const res = framer.nextFrame() catch (e) {
        return e == expected;
    };
    if (f := res) {
        framer.alloc.free(f);
    }
    return false;
}

fn fuzzDecodeBatch(alloc: std.mem.Allocator, raw: []const u8) {
    let b = decodeRecordBatch(alloc, raw) catch return;
    b.deinit(alloc);
}

fn fuzzFramerPush(alloc: std.mem.Allocator, framer: *KafkaStreamFramer, raw: []const u8) {
    framer.reset();
    framer.push(raw) catch return;
    const maybe_f = framer.nextFrame() catch return;
    if (f := maybe_f) {
        alloc.free(f);
    }
}

fn main() {
    test("1. ZigZag Varint 32/64 extremes, overflow rejection & Nullable (-1 vs <-1)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        let w = KafkaWriter.init(alloc);
        defer w.deinit();

        w.writeVarint(0) catch unreachable;
        w.writeVarint(-1) catch unreachable;
        w.writeVarint(1) catch unreachable;
        w.writeVarint(std.math.minInt(i32)) catch unreachable;
        w.writeVarint(std.math.maxInt(i32)) catch unreachable;
        w.writeVarlong(std.math.minInt(i64)) catch unreachable;
        w.writeVarlong(std.math.maxInt(i64)) catch unreachable;
        w.writeNullableString(null) catch unreachable;
        w.writeNullableString("kafka-client") catch unreachable;
        w.writeNullableBytes(null) catch unreachable;
        w.writeNullableBytes("payload") catch unreachable;

        let r = KafkaReader.init(w.items());
        expect_eq(r.readVarint() catch unreachable, 0 as i32);
        expect_eq(r.readVarint() catch unreachable, -1 as i32);
        expect_eq(r.readVarint() catch unreachable, 1 as i32);
        expect_eq(r.readVarint() catch unreachable, std.math.minInt(i32));
        expect_eq(r.readVarint() catch unreachable, std.math.maxInt(i32));
        expect_eq(r.readVarlong() catch unreachable, std.math.minInt(i64));
        expect_eq(r.readVarlong() catch unreachable, std.math.maxInt(i64));
        expect_eq((r.readNullableString() catch unreachable) == null, true);
        expect_eq((r.readNullableString() catch unreachable).?, "kafka-client");
        expect_eq((r.readNullableBytes() catch unreachable) == null, true);
        expect_eq((r.readNullableBytes() catch unreachable).?, "payload");

        // Overflow varints (5th byte > 0x0F / 6 bytes for i32; 10th byte > 0x01 / 11 bytes for i64)
        expect_eq(readVarintExpectErr("\x80\x80\x80\x80\x10", KafkaError.InvalidVarint), true);
        expect_eq(readVarintExpectErr("\x80\x80\x80\x80\x80\x01", KafkaError.InvalidVarint), true);
        expect_eq(readVarlongExpectErr("\x80\x80\x80\x80\x80\x80\x80\x80\x80\x02", KafkaError.InvalidVarint), true);
        expect_eq(readVarlongExpectErr("\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01", KafkaError.InvalidVarint), true);

        // Illegal negative lengths < -1 (-2 = 0xFFFE / 0xFFFFFFFE, i32::MIN = 0x80000000)
        expect_eq(readNullableStrExpectErr("\xff\xfe", KafkaError.InvalidLength), true);
        expect_eq(readNullableBytesExpectErr("\xff\xff\xff\xfe", KafkaError.InvalidLength), true);
        expect_eq(readNullableBytesExpectErr("\x80\x00\x00\x00", KafkaError.InvalidLength), true);
    });

    test("2. ApiVersions/Metadata framing, 1-byte stream reassembly & length limit guards", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(KAFKA_MAX_FRAME_LEN, (64 * 1024 * 1024) as usize);

        // Encode RequestHeader + MetadataReq v9
        let w = KafkaWriter.init(alloc);
        defer w.deinit();
        const hdr = RequestHeader{
            .api_key = @intFromEnum(ApiKey.Metadata),
            .api_version = 9,
            .correlation_id = 77,
            .client_id = "cp-kafka-client",
            .header_version = 2,
        };
        hdr.encode(&w) catch unreachable;
        const topics: [2][]const u8 = ["orders", "payments"];
        const mreq = MetadataReq{ .topics = topics[0..2], .allow_auto_topic_creation = false };
        mreq.encode(&w, 9) catch unreachable;

        const framed = KafkaStreamFramer.framePayload(alloc, w.items()) catch unreachable;
        defer alloc.free(framed);

        // Feed byte-by-byte into KafkaStreamFramer
        let framer = KafkaStreamFramer.init(alloc, 1024 * 1024);
        defer framer.deinit();
        let i: usize = 0;
        while (i < framed.len) : (i += 1) {
            framer.push(framed[i .. i + 1]) catch unreachable;
        }
        const out_frame = (framer.nextFrame() catch unreachable).?;
        defer alloc.free(out_frame);
        expect_eq(simdKafkaBytesEql(out_frame, w.items()), true);

        let r = KafkaReader.init(out_frame);
        const dec_hdr = RequestHeader.decode(&r, 2) catch unreachable;
        expect_eq(dec_hdr.correlation_id, 77 as i32);
        expect_eq(dec_hdr.client_id.?, "cp-kafka-client");
        let dec_mreq = MetadataReq.decode(alloc, &r, 9) catch unreachable;
        defer dec_mreq.deinit(alloc);
        expect_eq(dec_mreq.topics.?.len, 2 as usize);
        expect_eq(dec_mreq.topics.?[0], "orders");
        expect_eq(dec_mreq.allow_auto_topic_creation, false);

        // ApiVersionsResp v3 roundtrip
        let w2 = KafkaWriter.init(alloc);
        defer w2.deinit();
        const ranges: [2]ApiVersionRange = [
            ApiVersionRange{ .api_key = 0, .min_version = 0, .max_version = 9 },
            ApiVersionRange{ .api_key = 18, .min_version = 0, .max_version = 3 },
        ];
        const av = ApiVersionsResp{ .error_code = 0, .api_keys = ranges[0..2], .throttle_time_ms = 5 };
        av.encode(&w2, 3) catch unreachable;
        let r2 = KafkaReader.init(w2.items());
        let dec_av = ApiVersionsResp.decode(alloc, &r2, 3) catch unreachable;
        defer dec_av.deinit(alloc);
        expect_eq(dec_av.api_keys.len, 2 as usize);
        expect_eq(dec_av.throttle_time_ms, 5 as i32);

        // Negative frame length (-1) -> InvalidLength; Oversize frame length -> FrameTooLarge
        expect_eq(framerExpectErr(&framer, "\xff\xff\xff\xff", KafkaError.InvalidLength), true);
        expect_eq(framerExpectErr(&framer, "\x05\x00\x00\x00", KafkaError.FrameTooLarge), true);
    });

    test("3. RecordBatch v2 (magic=2) roundtrip, CRC32C tamper, bad magic & huge count guards", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const hdrs: [1]RecordHeader = [RecordHeader{ .key = "trace", .value = "abc-123" }];
        const recs: [2]Record = [
            Record{
                .attributes = 0,
                .timestamp_delta = 10,
                .offset_delta = 0,
                .key = "user-1",
                .value = "event_payload_over_16_bytes_0123456789",
                .headers = hdrs[0..1],
            },
            Record{
                .attributes = 0,
                .timestamp_delta = 25,
                .offset_delta = 1,
                .key = null,
                .value = "second_record",
                .headers = [],
            },
        ];
        const batch = RecordBatch{
            .base_offset = 1000,
            .partition_leader_epoch = 3,
            .attributes = 0,
            .last_offset_delta = 1,
            .base_timestamp = 1700000000000,
            .max_timestamp = 1700000000025,
            .producer_id = 42,
            .producer_epoch = 1,
            .base_sequence = 0,
            .records = recs[0..2],
        };

        const raw = encodeRecordBatch(alloc, &batch) catch unreachable;
        defer alloc.free(raw);

        let dec_batch = decodeRecordBatch(alloc, raw) catch unreachable;
        defer dec_batch.deinit(alloc);
        expect_eq(dec_batch.base_offset, 1000 as i64);
        expect_eq(dec_batch.records.len, 2 as usize);
        expect_eq(dec_batch.records[0].key.?, "user-1");
        expect_eq(dec_batch.records[0].value.?, "event_payload_over_16_bytes_0123456789");
        expect_eq(dec_batch.records[0].headers[0].key, "trace");
        expect_eq(dec_batch.records[1].key == null, true);

        // Single-bit CRC32C tamper in payload -> CrcMismatch
        let tampered: []u8 = alloc.dupe(u8, raw) catch unreachable;
        defer alloc.free(tampered);
        tampered[tampered.len - 1] ^= 0x01;
        expect_eq(decodeBatchExpectErr(alloc, tampered, KafkaError.CrcMismatch), true);

        // Invalid magic != 2 -> InvalidMagic
        let bad_magic: []u8 = alloc.dupe(u8, raw) catch unreachable;
        defer alloc.free(bad_magic);
        bad_magic[16] = 1;
        expect_eq(decodeBatchExpectErr(alloc, bad_magic, KafkaError.InvalidMagic), true);

        // Verify Castagnoli CRC32C RFC 3720 check value: CRC32C("123456789") == 0xE3069283
        expect_eq(kafkaCrc32c("123456789"), 0xE3069283 as u32);

        // Forged huge records_count (with recomputed valid CRC32C!) -> InvalidLength (no OOM!)
        let forged: []u8 = alloc.dupe(u8, raw) catch unreachable;
        defer alloc.free(forged);
        std.mem.writeInt(i32, forged[57..61], 999_999, .big);
        const new_crc = kafkaCrc32c(forged[21..]);
        std.mem.writeInt(u32, forged[17..21], new_crc, .big);
        expect_eq(decodeBatchExpectErr(alloc, forged, KafkaError.InvalidLength), true);
    });

    test("4. 16-lane SIMD simdKafkaCopyBytes & simdKafkaBytesEql 0-diff parity vs scalar", () => {
        let src: [80]u8 = undefined;
        let d_simd: [80]u8 = undefined;
        let d_scal: [80]u8 = undefined;
        let k: usize = 0;
        while (k < 80) : (k += 1) {
            src[k] = ((k * 37 + 11) & 0xFF) as u8;
        }
        let len: usize = 0;
        while (len <= 80) : (len += 1) {
            simdKafkaCopyBytes(d_simd[0..len], src[0..len]);
            simdKafkaCopyBytesScalar(d_scal[0..len], src[0..len]);
            expect_eq(simdKafkaBytesEql(d_simd[0..len], d_scal[0..len]), true);
            expect_eq(simdKafkaBytesEql(d_simd[0..len], d_scal[0..len]), simdKafkaBytesEqlScalar(d_simd[0..len], d_scal[0..len]));
            if (len > 0) {
                d_scal[len - 1] ^= 0x01;
                expect_eq(simdKafkaBytesEql(d_simd[0..len], d_scal[0..len]), false);
                expect_eq(simdKafkaBytesEql(d_simd[0..len], d_scal[0..len]), simdKafkaBytesEqlScalar(d_simd[0..len], d_scal[0..len]));
            }
        }
    });

    test("5. Deterministic PRNG Fuzz (>= 1,600 malformed Kafka frames/RecordBatch & SIMD parity)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        let seed: u64 = 0x5EED_CAFE_2026_0034;
        let buf: [96]u8 = undefined;
        let d1: [96]u8 = undefined;
        let d2: [96]u8 = undefined;

        let framer = KafkaStreamFramer.init(alloc, 4096);
        defer framer.deinit();

        // 5A. 1,000 malformed Kafka frame & RecordBatch fuzz iterations
        let iter: usize = 0;
        while (iter < 1000) : (iter += 1) {
            const flen: usize = ((nextRng(&seed) % 88) + 1) as usize;
            let k: usize = 0;
            while (k < flen) : (k += 1) {
                buf[k] = (nextRng(&seed) & 0xFF) as u8;
            }
            const sample = buf[0..flen];
            fuzzDecodeBatch(alloc, sample);
            fuzzFramerPush(alloc, &framer, sample);
            let rr = KafkaReader.init(sample);
            * = rr.readVarint() catch {};
            * = rr.readVarlong() catch {};
            * = rr.skipTaggedFields() catch {};
        }

        // 5B. 600 random byte-slice SIMD copy & equality parity checks
        let r_iter: usize = 0;
        while (r_iter < 600) : (r_iter += 1) {
            const rlen: usize = (nextRng(&seed) % 80) as usize;
            let j: usize = 0;
            while (j < rlen) : (j += 1) {
                buf[j] = (nextRng(&seed) & 0xFF) as u8;
            }
            const slice = buf[0..rlen];
            simdKafkaCopyBytes(d1[0..rlen], slice);
            simdKafkaCopyBytesScalar(d2[0..rlen], slice);
            expect_eq(simdKafkaBytesEql(d1[0..rlen], d2[0..rlen]), true);
            expect_eq(simdKafkaBytesEql(d1[0..rlen], d2[0..rlen]), simdKafkaBytesEqlScalar(d1[0..rlen], d2[0..rlen]));
        }
    });
}
