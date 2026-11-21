// Source: cp/cap/test/test_fit_production_deep.cp (Production deep tests for cap:fit)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    FIT_MAGIC,
    FIT_HEADER_SIZE_12,
    FIT_HEADER_SIZE_14,
    FIT_MESG_FILE_ID,
    FIT_MESG_RECORD,
    FitHeader,
    FitRecordKind,
    FitRecord,
    FitStats,
    FitRecordIter,
    fitCrc16,
    isFit,
    parseFitHeader,
    verifyFileCrc,
    summarizeFit,
} from "cap:fit";

fn writeU16Le(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn writeU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn buildFitFile(out: []u8, hdr_size: u8, body: []const u8): usize {
    const hlen = usize(hdr_size);
    out[0] = hdr_size;
    out[1] = 0x20; // Protocol 2.0
    writeU16Le(out, 2, 2112); // Profile 21.12
    writeU32Le(out, 4, body.len as u32);
    out[8] = '.';
    out[9] = 'F';
    out[10] = 'I';
    out[11] = 'T';
    if (hdr_size == FIT_HEADER_SIZE_14) {
        const hcrc = fitCrc16(out[0..12]);
        writeU16Le(out, 12, hcrc);
    }
    let i: usize = 0;
    while (i < body.len) {
        out[hlen + i] = body[i];
        i += 1;
    }
    const body_end = hlen + body.len;
    const fcrc = fitCrc16(out[0..body_end]);
    writeU16Le(out, body_end, fcrc);
    return body_end + 2;
}

test("1. 14B header CRC, 12B legacy header, and full file trailing CRC verification", () => {
    // Known CRC-16/ARC nibble-table vector for single byte [0x01] -> 0xC0C1
    const single: [1]u8 = [0x01];
    expect_eq(fitCrc16(single[0..]), 0xC0C1);
    expect_eq(FIT_MAGIC.len, 4);

    // 14B header with empty body
    let buf14: [64]u8 = std.mem.zeroes([64]u8);
    const empty: [0]u8 = [];
    const len14 = buildFitFile(buf14[0..], FIT_HEADER_SIZE_14, empty[0..]);
    expect_eq(len14, 16);
    expect_eq(isFit(buf14[0..len14]), true);

    const h14 = parseFitHeader(buf14[0..len14]) ?? unreachable;
    expect_eq(h14.header_size, 14);
    expect_eq(h14.protocol_version, 0x20);
    expect_eq(h14.profile_version, 2112);
    expect_eq(h14.data_size, 0);
    expect_eq(h14.header_crc != 0, true);
    expect_eq(verifyFileCrc(buf14[0..len14]), true);

    // 12B legacy header
    let buf12: [64]u8 = std.mem.zeroes([64]u8);
    const len12 = buildFitFile(buf12[0..], FIT_HEADER_SIZE_12, empty[0..]);
    expect_eq(len12, 14);
    const h12 = parseFitHeader(buf12[0..len12]) ?? unreachable;
    expect_eq(h12.header_size, 12);
    expect_eq(h12.header_crc, 0);
    expect_eq(verifyFileCrc(buf12[0..len12]), true);
});

test("2. LE and BE Definition + Data Message streaming iteration and summarizeFit", () => {
    // Body:
    // 1) Def local=0, LE, global=0 (file_id), 2 fields: (0, size=1, type=0), (1, size=2, type=132) -> data_len=3
    // 2) Data local=0: [0x04, 0x01, 0x00]
    // 3) Def local=1, BE, global=20 (record, BE: 0x00, 0x14), 1 field: (3, size=1, type=2) -> data_len=1
    // 4) Data local=1: [152] (heart_rate = 152 bpm)
    // 5) Data local=1: [158] (heart_rate = 158 bpm)
    const body: [29]u8 = [
        // Def local=0 (LE, file_id=0)
        0x40, 0x00, 0x00, 0x00, 0x00, 0x02,
        0x00, 0x01, 0x00,
        0x01, 0x02, 0x84,
        // Data local=0
        0x00, 0x04, 0x01, 0x00,
        // Def local=1 (BE, record=20 -> 0x00, 0x14)
        0x41, 0x00, 0x01, 0x00, 0x14, 0x01,
        0x03, 0x01, 0x02,
        // Data local=1 (#1)
        0x01, 152,
        // Data local=1 (#2)
        0x01, 158,
    ];

    let file_buf: [128]u8 = std.mem.zeroes([128]u8);
    const flen = buildFitFile(file_buf[0..], FIT_HEADER_SIZE_14, body[0..]);
    expect_eq(verifyFileCrc(file_buf[0..flen]), true);

    let it = FitRecordIter.init(file_buf[0..flen]) ?? unreachable;

    const r0 = it.next() ?? unreachable;
    expect_eq(r0.kind, FitRecordKind.definition);
    expect_eq(r0.local_mesg_type, 0);
    expect_eq(r0.global_mesg_num, FIT_MESG_FILE_ID);
    expect_eq(r0.is_big_endian, false);
    expect_eq(r0.field_count, 2);

    const r1 = it.next() ?? unreachable;
    expect_eq(r1.kind, FitRecordKind.data);
    expect_eq(r1.global_mesg_num, FIT_MESG_FILE_ID);
    expect_eq(r1.payload.len, 3);
    expect_eq(r1.payload[0], 0x04);

    const r2 = it.next() ?? unreachable;
    expect_eq(r2.kind, FitRecordKind.definition);
    expect_eq(r2.local_mesg_type, 1);
    expect_eq(r2.global_mesg_num, FIT_MESG_RECORD);
    expect_eq(r2.is_big_endian, true);
    expect_eq(r2.field_count, 1);

    const r3 = it.next() ?? unreachable;
    expect_eq(r3.kind, FitRecordKind.data);
    expect_eq(r3.global_mesg_num, FIT_MESG_RECORD);
    expect_eq(r3.payload.len, 1);
    expect_eq(r3.payload[0], 152);

    const r4 = it.next() ?? unreachable;
    expect_eq(r4.kind, FitRecordKind.data);
    expect_eq(r4.payload[0], 158);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);

    const stats = summarizeFit(file_buf[0..flen]);
    expect_eq(stats.definition_count, 2);
    expect_eq(stats.data_count, 3);
    expect_eq(stats.record_mesg_count, 2);
    expect_eq(stats.file_id_count, 1);
    expect_eq(stats.crc_valid, true);
});

test("3. FIT 2.0 Developer Data Fields (0x20) extension skipping and payload sizing", () => {
    // Def local=2 with Developer Data flag (0x40 | 0x20 | 0x02 = 0x62):
    // - 1 standard field of size 2
    // - 1 developer field of size 4 -> total data_byte_len = 6
    const body: [20]u8 = [
        0x62, 0x00, 0x00, 0x14, 0x00, 0x01,
        0x00, 0x02, 0x84,       // std field: size=2
        0x01,                   // 1 developer field
        0x00, 0x04, 0x00,       // dev field: size=4, dev_idx=0
        // Data local=2 (6 bytes: 2 std + 4 dev)
        0x02, 0xAA, 0xBB, 0x11, 0x22, 0x33, 0x44,
    ];

    let file_buf: [128]u8 = std.mem.zeroes([128]u8);
    const flen = buildFitFile(file_buf[0..], FIT_HEADER_SIZE_14, body[0..]);

    let it = FitRecordIter.init(file_buf[0..flen]) ?? unreachable;
    const def_rec = it.next() ?? unreachable;
    expect_eq(def_rec.kind, FitRecordKind.definition);
    expect_eq(def_rec.local_mesg_type, 2);
    expect_eq(def_rec.global_mesg_num, FIT_MESG_RECORD);

    const data_rec = it.next() ?? unreachable;
    expect_eq(data_rec.kind, FitRecordKind.data);
    expect_eq(data_rec.payload.len, 6);
    expect_eq(data_rec.payload[0], 0xAA);
    expect_eq(data_rec.payload[5], 0x44);
    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("4. Compressed Timestamp Data Message (0x80) decoding", () => {
    // Def local=1 (record=20, 1 field of size 2)
    // Compressed Timestamp header: 0x80 | (local=1 << 5) | (offset=17) = 0xB1
    const body: [12]u8 = [
        0x41, 0x00, 0x00, 0x14, 0x00, 0x01,
        0x05, 0x02, 0x84,
        0xB1, 0x34, 0x12,
    ];

    let file_buf: [128]u8 = std.mem.zeroes([128]u8);
    const flen = buildFitFile(file_buf[0..], FIT_HEADER_SIZE_14, body[0..]);

    let it = FitRecordIter.init(file_buf[0..flen]) ?? unreachable;
    * = it.next() ?? unreachable; // skip definition

    const ct_rec = it.next() ?? unreachable;
    expect_eq(ct_rec.kind, FitRecordKind.data);
    expect_eq(ct_rec.local_mesg_type, 1);
    expect_eq(ct_rec.global_mesg_num, FIT_MESG_RECORD);
    expect_eq(ct_rec.time_offset ?? 0, 17);
    expect_eq(ct_rec.payload.len, 2);
    expect_eq(ct_rec.payload[0], 0x34);
    expect_eq(ct_rec.payload[1], 0x12);
    expect_eq(it.had_error, false);
});

test("5. Fault tolerance: bad magic, bad header/file CRC, undefined local type, and truncation", () => {
    let file_buf: [128]u8 = std.mem.zeroes([128]u8);
    const valid_body: [10]u8 = [
        0x40, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
        0x00, // truncated! missing 1-byte data payload
    ];
    const flen = buildFitFile(file_buf[0..], FIT_HEADER_SIZE_14, valid_body[0..]);

    // 1. Truncated data payload sets had_error=true
    let it_trunc = FitRecordIter.init(file_buf[0..flen]) ?? unreachable;
    * = it_trunc.next() ?? unreachable; // definition ok
    expect_eq(it_trunc.next() == null, true);
    expect_eq(it_trunc.had_error, true);

    // 2. Corrupted trailing file CRC -> verifyFileCrc returns false
    file_buf[flen - 1] = file_buf[flen - 1] ^ 0xFF;
    expect_eq(verifyFileCrc(file_buf[0..flen]), false);
    const bad_crc_stats = summarizeFit(file_buf[0..flen]);
    expect_eq(bad_crc_stats.crc_valid, false);

    // 3. Corrupted 14B header CRC -> parseFitHeader returns null
    file_buf[12] = file_buf[12] ^ 0x01;
    expect_eq(parseFitHeader(file_buf[0..flen]) == null, true);

    // 4. Wrong magic ".BAD" -> isFit / parseFitHeader return false/null
    file_buf[9] = 'B';
    expect_eq(isFit(file_buf[0..flen]), false);
    expect_eq(parseFitHeader(file_buf[0..flen]) == null, true);

    // 5. Undefined local_mesg_type in data record -> had_error=true
    const undef_body: [2]u8 = [0x05, 0x99];
    let ubuf: [64]u8 = std.mem.zeroes([64]u8);
    const ulen = buildFitFile(ubuf[0..], FIT_HEADER_SIZE_14, undef_body[0..]);
    let it_undef = FitRecordIter.init(ubuf[0..ulen]) ?? unreachable;
    expect_eq(it_undef.next() == null, true);
    expect_eq(it_undef.had_error, true);
});
