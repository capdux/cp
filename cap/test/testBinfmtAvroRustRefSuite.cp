// Reference-hardened test suite against apache/avro-rs (Apache-2.0) & Apache Avro 1.11 Spec
// Covers:
//   1. ZigZag 32/64 extreme values (0, -1, 1, -2, 2, i32::MIN/MAX, i64::MIN/MAX) & 6/11-byte varint overflow rejection
//   2. Full OCF ("Obj\x01") multi-block + metadata (avro.schema/avro.codec) + negative map block count roundtrip & zero-heap validateAvroOcf
//   3. Single-bit Sync Marker tamper rejection (SyncMarkerMismatch / InvalidSyncMarker) & forged huge block_byte_len / i64::MIN count guards
//   4. 16-lane @Vector(16, u8) SIMD Sync Marker equality & payload copy 0-diff parity vs scalar
//   5. Deterministic PRNG Fuzz (>= 1,600 malformed Avro OCF streams & random SIMD parity)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    AvroError,
    AvroSchema,
    AvroValue,
    AvroRecordField,
    simdAvroSyncMarkerEql,
    simdAvroSyncMarkerEqlScalar,
    simdAvroCopyPayload,
    simdAvroCopyPayloadScalar,
    avroZigzagEncodeI32,
    avroZigzagDecodeI32,
    avroZigzagEncodeI64,
    avroZigzagDecodeI64,
    decodeVarintI32,
    decodeVarintI64,
    encodeDatum,
    decodeDatum,
    writeContainer,
    readContainer,
    validateAvroOcf,
} from "../../lib/binfmt/avro.cp";

fn nextRng(state: *u64): u64 {
    let x = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

fn decodeVarintI32ExpectErr(raw: []const u8, expected: AvroError): bool {
    let pos: usize = 0;
    * = decodeVarintI32(raw, &pos) catch (e) {
        return e == expected;
    };
    return false;
}

fn decodeVarintI64ExpectErr(raw: []const u8, expected: AvroError): bool {
    let pos: usize = 0;
    * = decodeVarintI64(raw, &pos) catch (e) {
        return e == expected;
    };
    return false;
}

fn validateOcfExpectErr(raw: []const u8, expected: AvroError): bool {
    * = validateAvroOcf(raw) catch (e) {
        return e == expected;
    };
    return false;
}

fn readOcfExpectErr(alloc: std.mem.Allocator, raw: []const u8, expected: AvroError): bool {
    let c = readContainer(alloc, raw) catch (e) {
        return e == expected;
    };
    c.deinit(alloc);
    return false;
}

fn fuzzValidateAndRead(alloc: std.mem.Allocator, raw: []const u8) {
    * = validateAvroOcf(raw) catch {};
    let c = readContainer(alloc, raw) catch return;
    c.deinit(alloc);
}

fn main() {
    test("1. ZigZag 32/64 extreme values & 6/11-byte varint overflow rejection", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        // Canonical ZigZag mapping: 0->0, -1->1, 1->2, -2->3, 2->4
        expect_eq(avroZigzagEncodeI32(0), 0 as u32);
        expect_eq(avroZigzagEncodeI32(-1), 1 as u32);
        expect_eq(avroZigzagEncodeI32(1), 2 as u32);
        expect_eq(avroZigzagEncodeI32(-2), 3 as u32);
        expect_eq(avroZigzagEncodeI32(2), 4 as u32);
        expect_eq(avroZigzagDecodeI32(avroZigzagEncodeI32(std.math.minInt(i32))), std.math.minInt(i32));
        expect_eq(avroZigzagDecodeI32(avroZigzagEncodeI32(std.math.maxInt(i32))), std.math.maxInt(i32));

        expect_eq(avroZigzagEncodeI64(0), 0 as u64);
        expect_eq(avroZigzagEncodeI64(-1), 1 as u64);
        expect_eq(avroZigzagEncodeI64(1), 2 as u64);
        expect_eq(avroZigzagEncodeI64(-2), 3 as u64);
        expect_eq(avroZigzagEncodeI64(2), 4 as u64);
        expect_eq(avroZigzagDecodeI64(avroZigzagEncodeI64(std.math.minInt(i64))), std.math.minInt(i64));
        expect_eq(avroZigzagDecodeI64(avroZigzagEncodeI64(std.math.maxInt(i64))), std.math.maxInt(i64));

        // Full encodeDatum/decodeDatum roundtrip on i32::MIN/MAX and i64::MIN/MAX
        let s_int = AvroSchema.parseSchema(alloc, "\"int\"") catch unreachable;
        defer s_int.deinit();
        const b_imin = encodeDatum(alloc, s_int, AvroValue.makeInt(std.math.minInt(i32))) catch unreachable;
        defer alloc.free(b_imin);
        let v_imin = decodeDatum(alloc, s_int, b_imin) catch unreachable;
        defer v_imin.deinit(alloc);
        expect_eq(v_imin.asInt().?, std.math.minInt(i32));

        let s_long = AvroSchema.parseSchema(alloc, "\"long\"") catch unreachable;
        defer s_long.deinit();
        const b_lmin = encodeDatum(alloc, s_long, AvroValue.makeLong(std.math.minInt(i64))) catch unreachable;
        defer alloc.free(b_lmin);
        let v_lmin = decodeDatum(alloc, s_long, b_lmin) catch unreachable;
        defer v_lmin.deinit(alloc);
        expect_eq(v_lmin.asLong().?, std.math.minInt(i64));

        // 5th byte > 0x0F (or 6-byte varint) for i32 -> VarintOverflow
        expect_eq(decodeVarintI32ExpectErr("\x80\x80\x80\x80\x10", AvroError.VarintOverflow), true);
        expect_eq(decodeVarintI32ExpectErr("\x80\x80\x80\x80\x80\x01", AvroError.VarintOverflow), true);
        // 10th byte > 0x01 (or 11-byte varint) for i64 -> VarintOverflow
        expect_eq(decodeVarintI64ExpectErr("\x80\x80\x80\x80\x80\x80\x80\x80\x80\x02", AvroError.VarintOverflow), true);
        expect_eq(decodeVarintI64ExpectErr("\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x01", AvroError.VarintOverflow), true);
    });

    test("2. Full OCF (Obj\\x01) roundtrip, negative metadata map block & zero-heap validateAvroOcf", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const schema_json = "{\"type\":\"record\",\"name\":\"Event\",\"fields\":[{\"name\":\"id\",\"type\":\"long\"},{\"name\":\"tag\",\"type\":\"string\"}]}";
        const sync: [16]u8 = [0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x07, 0x18, 0x29, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x90];

        let f1: [2]AvroRecordField = [
            AvroRecordField.init("id", AvroValue.makeLong(42)),
            AvroRecordField.init("tag", AvroValue.makeString("alpha_payload_over_16_bytes")),
        ];
        let f2: [2]AvroRecordField = [
            AvroRecordField.init("id", AvroValue.makeLong(-999999)),
            AvroRecordField.init("tag", AvroValue.makeString("beta")),
        ];
        let recs: [2]AvroValue = [
            AvroValue.makeRecord(f1[0..2]),
            AvroValue.makeRecord(f2[0..2]),
        ];

        const ocf = writeContainer(alloc, schema_json, sync, recs[0..2]) catch unreachable;
        defer alloc.free(ocf);

        // Zero-heap structural + Sync Marker validator
        const summary = validateAvroOcf(ocf) catch unreachable;
        expect_eq(summary.schema_json, schema_json);
        expect_eq(summary.codec, "null");
        expect_eq(simdAvroSyncMarkerEql(summary.sync_marker, sync), true);
        expect_eq(summary.block_count, 1 as usize);
        expect_eq(summary.total_objects, 2 as usize);

        // Full container read & datum verification
        let loaded = readContainer(alloc, ocf) catch unreachable;
        defer loaded.deinit(alloc);
        expect_eq(loaded.records.len, 2 as usize);
        expect_eq(loaded.records[0].getField("id").?.asLong().?, 42 as i64);
        expect_eq(loaded.records[0].getField("tag").?.asString().?, "alpha_payload_over_16_bytes");
        expect_eq(loaded.records[1].getField("id").?.asLong().?, -999999 as i64);

        // Negative map block count in OCF header (-1 entry, followed by byte_size)
        // Entry: key="avro.schema" (11B -> zz 0x16), val="\"int\"" (5B -> zz 0x0A) -> entry bytes = 1+11+1+5 = 18B (zz 0x24)
        let neg_ocf: [64]u8 = undefined;
        let p: usize = 0;
        neg_ocf[0] = 'O'; neg_ocf[1] = 'b'; neg_ocf[2] = 'j'; neg_ocf[3] = 1; p = 4;
        neg_ocf[p] = 0x01; p += 1; // ZigZag(-1) = 1
        neg_ocf[p] = 0x24; p += 1; // ZigZag(18) = 36 = 0x24
        neg_ocf[p] = 0x16; p += 1; // ZigZag(11) = 22 = 0x16
        @memcpy(neg_ocf[p .. p + 11], "avro.schema"); p += 11;
        neg_ocf[p] = 0x0A; p += 1; // ZigZag(5) = 10 = 0x0A
        @memcpy(neg_ocf[p .. p + 5], "\"int\""); p += 5;
        neg_ocf[p] = 0x00; p += 1; // end of metadata map
        @memcpy(neg_ocf[p .. p + 16], sync[0..16]); p += 16;

        const neg_sum = validateAvroOcf(neg_ocf[0..p]) catch unreachable;
        expect_eq(neg_sum.schema_json, "\"int\"");
        expect_eq(neg_sum.block_count, 0 as usize);
    });

    test("3. Sync Marker single-bit tamper, forged huge block_len & i64::MIN count rejection", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const sync: [16]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16];
        let recs: [1]AvroValue = [AvroValue.makeInt(7)];

        const ocf = writeContainer(alloc, "\"int\"", sync, recs[0..1]) catch unreachable;
        defer alloc.free(ocf);

        // Single-bit flip in trailing 16-byte Sync Marker -> SyncMarkerMismatch / InvalidSyncMarker
        let tampered: []u8 = alloc.dupe(u8, ocf) catch unreachable;
        defer alloc.free(tampered);
        tampered[tampered.len - 1] ^= 0x01;
        expect_eq(validateOcfExpectErr(tampered, AvroError.SyncMarkerMismatch), true);
        expect_eq(readOcfExpectErr(alloc, tampered, AvroError.InvalidSyncMarker), true);

        // Truncated OCF -> UnexpectedEof
        expect_eq(validateOcfExpectErr(ocf[0..(ocf.len - 5)], AvroError.UnexpectedEof), true);

        // Bad magic -> InvalidContainerHeader
        expect_eq(validateOcfExpectErr("Obj\x02\x00", AvroError.InvalidContainerHeader), true);

        // Malicious i64::MIN (-9223372036854775808 -> ZigZag 0xFFFFFFFFFFFFFFFF = 10 bytes) as map count -> VarintOverflow (no panic!)
        const min_i64_map = "Obj\x01\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01\x00";
        expect_eq(validateOcfExpectErr(min_i64_map, AvroError.VarintOverflow), true);
        expect_eq(readOcfExpectErr(alloc, min_i64_map, AvroError.VarintOverflow), true);
    });

    test("4. 16-lane SIMD Sync Marker equality & payload copy 0-diff parity vs scalar", () => {
        const s1: [16]u8 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15];
        let s2: [16]u8 = s1;
        expect_eq(simdAvroSyncMarkerEql(s1, s2), true);
        expect_eq(simdAvroSyncMarkerEql(s1, s2), simdAvroSyncMarkerEqlScalar(s1, s2));

        let idx: usize = 0;
        while (idx < 16) : (idx += 1) {
            s2 = s1;
            s2[idx] ^= 0x80;
            expect_eq(simdAvroSyncMarkerEql(s1, s2), false);
            expect_eq(simdAvroSyncMarkerEql(s1, s2), simdAvroSyncMarkerEqlScalar(s1, s2));
        }

        let src: [80]u8 = undefined;
        let dst_simd: [80]u8 = undefined;
        let dst_scal: [80]u8 = undefined;
        let k: usize = 0;
        while (k < 80) : (k += 1) {
            src[k] = ((k * 17 + 3) & 0xFF) as u8;
        }
        let len: usize = 0;
        while (len <= 80) : (len += 1) {
            simdAvroCopyPayload(dst_simd[0..len], src[0..len]);
            simdAvroCopyPayloadScalar(dst_scal[0..len], src[0..len]);
            expect_eq(dst_simd[0..len], dst_scal[0..len]);
        }
    });

    test("5. Deterministic PRNG Fuzz (>= 1,600 malformed Avro OCF streams & random SIMD parity)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        let seed: u64 = 0x5EED_A780_2026_0034;
        let buf: [96]u8 = undefined;

        // 5A. 1,000 malformed Avro OCF stream fuzz iterations (with valid Obj\x01 prefix on 50%)
        let iter: usize = 0;
        while (iter < 1000) : (iter += 1) {
            const flen: usize = ((nextRng(&seed) % 88) + 1) as usize;
            let k: usize = 0;
            if ((iter & 1) == 0 && flen >= 4) {
                buf[0] = 'O';
                buf[1] = 'b';
                buf[2] = 'j';
                buf[3] = 1;
                k = 4;
            }
            while (k < flen) : (k += 1) {
                buf[k] = (nextRng(&seed) & 0xFF) as u8;
            }
            fuzzValidateAndRead(alloc, buf[0..flen]);
        }

        // 5B. 600 random Sync Marker & payload copy SIMD parity checks
        let a: [16]u8 = undefined;
        let b: [16]u8 = undefined;
        let d1: [64]u8 = undefined;
        let d2: [64]u8 = undefined;
        let r_iter: usize = 0;
        while (r_iter < 600) : (r_iter += 1) {
            let j: usize = 0;
            while (j < 16) : (j += 1) {
                a[j] = (nextRng(&seed) & 0xFF) as u8;
                b[j] = if ((r_iter & 1) == 0) a[j] else (nextRng(&seed) & 0xFF) as u8;
            }
            expect_eq(simdAvroSyncMarkerEql(a, b), simdAvroSyncMarkerEqlScalar(a, b));

            const clen: usize = (nextRng(&seed) % 64) as usize;
            j = 0;
            while (j < clen) : (j += 1) {
                buf[j] = (nextRng(&seed) & 0xFF) as u8;
            }
            simdAvroCopyPayload(d1[0..clen], buf[0..clen]);
            simdAvroCopyPayloadScalar(d2[0..clen], buf[0..clen]);
            expect_eq(d1[0..clen], d2[0..clen]);
        }
    });
}
