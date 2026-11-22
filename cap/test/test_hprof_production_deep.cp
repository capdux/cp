// Source: cp/cap/test/test_hprof_production_deep.cp (Production deep tests for cap:hprof)

import { test, expect_eq } from "cap:test";
import {
    TAG_UTF8,
    TAG_LOAD_CLASS,
    TAG_HEAP_SUMMARY,
    TAG_HEAP_DUMP,
    TAG_HEAP_DUMP_SEGMENT,
    TAG_HEAP_DUMP_END,
    SUB_CLASS_DUMP,
    SUB_INSTANCE_DUMP,
    SUB_PRIM_ARRAY_DUMP,
    HprofBasicType,
    decodeHprofBasicType,
    hprofBasicTypeSize,
    HprofHeader,
    HprofRecord,
    HprofUtf8,
    HprofLoadClass,
    HprofHeapSummary,
    HprofStats,
    isHprof,
    parseHprofHeader,
    HprofRecordIter,
    parseHprofUtf8,
    parseHprofLoadClass,
    parseHprofHeapSummary,
    summarizeHprof,
} from "cap:binfmt";

fn writeBytes(dst: []u8, off: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
    return off + src.len;
}

fn writeU32BE(dst: []u8, off: usize, val: u32): usize {
    dst[off + 0] = ((val >> 24) & 0xFF) as u8;
    dst[off + 1] = ((val >> 16) & 0xFF) as u8;
    dst[off + 2] = ((val >> 8) & 0xFF) as u8;
    dst[off + 3] = (val & 0xFF) as u8;
    return off + 4;
}

fn writeU64BE(dst: []u8, off: usize, val: u64): usize {
    let p = writeU32BE(dst, off, ((val >> 32) & 0xFFFFFFFF) as u32);
    p = writeU32BE(dst, p, (val & 0xFFFFFFFF) as u32);
    return p;
}

fn writeHeader(dst: []u8, fmt: []const u8, id_size: u32, ts_ms: u64): usize {
    let p = writeBytes(dst, 0, fmt);
    dst[p] = 0;
    p += 1;
    p = writeU32BE(dst, p, id_size);
    p = writeU64BE(dst, p, ts_ms);
    return p;
}

fn writeRecordHeader(dst: []u8, off: usize, tag: u8, time_us: u32, len: u32): usize {
    dst[off] = tag;
    let p = writeU32BE(dst, off + 1, time_us);
    p = writeU32BE(dst, p, len);
    return p;
}

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. parses 64-bit HPROF 1.0.2 header and UTF8 / LOAD_CLASS / HEAP_SUMMARY / HEAP_DUMP_SEGMENT stream", () => {
    let raw: [256]u8 = undefined;
    let buf = raw[0..256];

    let p = writeHeader(buf, "JAVA PROFILE 1.0.2", 8, 1700000123456);
    expect_eq(p, 31);

    // Record 1: TAG_UTF8 (id=0x100, text="java/lang/String")
    const cls_name = "java/lang/String";
    p = writeRecordHeader(buf, p, TAG_UTF8, 10, (8 + cls_name.len) as u32);
    p = writeU64BE(buf, p, 0x0000000100000100);
    p = writeBytes(buf, p, cls_name);

    // Record 2: TAG_LOAD_CLASS (serial=7, obj=0x200, stack=3, name_id=0x100)
    p = writeRecordHeader(buf, p, TAG_LOAD_CLASS, 20, 24);
    p = writeU32BE(buf, p, 7);
    p = writeU64BE(buf, p, 0x0000000100000200);
    p = writeU32BE(buf, p, 3);
    p = writeU64BE(buf, p, 0x0000000100000100);

    // Record 3: TAG_HEAP_SUMMARY (24 bytes)
    p = writeRecordHeader(buf, p, TAG_HEAP_SUMMARY, 30, 24);
    p = writeU32BE(buf, p, 65536); // live bytes
    p = writeU32BE(buf, p, 512); // live instances
    p = writeU64BE(buf, p, 1048576); // allocated bytes
    p = writeU64BE(buf, p, 4096); // allocated instances

    // Record 4: TAG_HEAP_DUMP_SEGMENT (4 bytes dummy subrecord)
    p = writeRecordHeader(buf, p, TAG_HEAP_DUMP_SEGMENT, 40, 4);
    buf[p] = SUB_INSTANCE_DUMP;
    buf[p + 1] = 0;
    buf[p + 2] = 0;
    buf[p + 3] = 0;
    p += 4;

    // Record 5: TAG_HEAP_DUMP_END (0 bytes)
    p = writeRecordHeader(buf, p, TAG_HEAP_DUMP_END, 50, 0);

    const hprof = buf[0..p];
    expect_eq(isHprof(hprof), true);

    if (hdr := parseHprofHeader(hprof)) {
        expect_eq(sliceEq(hdr.format, "JAVA PROFILE 1.0.2"), true);
        expect_eq(hdr.id_size, 8);
        expect_eq(hdr.timestamp_ms, 1700000123456);
        expect_eq(hdr.records_offset, 31);

        let it = HprofRecordIter.init(hprof, hdr);

        // 1. UTF8
        if (r1 := it.next()) {
            expect_eq(r1.tag, TAG_UTF8);
            expect_eq(r1.time_offset_us, 10);
            if (u := parseHprofUtf8(r1.body, hdr.id_size)) {
                expect_eq(u.id, 0x0000000100000100);
                expect_eq(sliceEq(u.text, cls_name), true);
            } else { expect_eq(true, false); }
        } else { expect_eq(true, false); }

        // 2. LOAD_CLASS
        if (r2 := it.next()) {
            expect_eq(r2.tag, TAG_LOAD_CLASS);
            if (lc := parseHprofLoadClass(r2.body, hdr.id_size)) {
                expect_eq(lc.class_serial, 7);
                expect_eq(lc.class_obj_id, 0x0000000100000200);
                expect_eq(lc.stack_trace_serial, 3);
                expect_eq(lc.class_name_id, 0x0000000100000100);
            } else { expect_eq(true, false); }
        } else { expect_eq(true, false); }

        // 3. HEAP_SUMMARY
        if (r3 := it.next()) {
            expect_eq(r3.tag, TAG_HEAP_SUMMARY);
            if (hs := parseHprofHeapSummary(r3.body)) {
                expect_eq(hs.total_live_bytes, 65536);
                expect_eq(hs.total_live_instances, 512);
                expect_eq(hs.total_bytes_allocated, 1048576);
                expect_eq(hs.total_instances_allocated, 4096);
            } else { expect_eq(true, false); }
        } else { expect_eq(true, false); }

        // 4. HEAP_DUMP_SEGMENT
        if (r4 := it.next()) {
            expect_eq(r4.tag, TAG_HEAP_DUMP_SEGMENT);
            expect_eq(r4.body.len, 4);
            expect_eq(r4.body[0], SUB_INSTANCE_DUMP);
        } else { expect_eq(true, false); }

        // 5. HEAP_DUMP_END
        if (r5 := it.next()) {
            expect_eq(r5.tag, TAG_HEAP_DUMP_END);
            expect_eq(r5.length, 0);
        } else { expect_eq(true, false); }

        expect_eq(it.next() == null, true);
        expect_eq(it.had_error, false);
    } else {
        expect_eq(true, false);
    }
});

test("2. parses 32-bit HPROF 1.0.1 header with id_size=4 UTF8 and LOAD_CLASS records", () => {
    let raw: [128]u8 = undefined;
    let buf = raw[0..128];

    let p = writeHeader(buf, "JAVA PROFILE 1.0.1", 4, 1600000000000);
    // UTF8 with 4-byte ID
    p = writeRecordHeader(buf, p, TAG_UTF8, 0, 9);
    p = writeU32BE(buf, p, 0x12345678);
    p = writeBytes(buf, p, "Hello");

    // LOAD_CLASS with 4-byte IDs (8 + 2*4 = 16 bytes)
    p = writeRecordHeader(buf, p, TAG_LOAD_CLASS, 1, 16);
    p = writeU32BE(buf, p, 1);
    p = writeU32BE(buf, p, 0xAABBCCDD);
    p = writeU32BE(buf, p, 2);
    p = writeU32BE(buf, p, 0x12345678);

    const hprof = buf[0..p];
    if (hdr := parseHprofHeader(hprof)) {
        expect_eq(sliceEq(hdr.format, "JAVA PROFILE 1.0.1"), true);
        expect_eq(hdr.id_size, 4);

        let it = HprofRecordIter.init(hprof, hdr);
        if (r1 := it.next()) {
            if (u := parseHprofUtf8(r1.body, 4)) {
                expect_eq(u.id, 0x12345678);
                expect_eq(sliceEq(u.text, "Hello"), true);
            } else { expect_eq(true, false); }
        } else { expect_eq(true, false); }

        if (r2 := it.next()) {
            if (lc := parseHprofLoadClass(r2.body, 4)) {
                expect_eq(lc.class_obj_id, 0xAABBCCDD);
                expect_eq(lc.class_name_id, 0x12345678);
            } else { expect_eq(true, false); }
        } else { expect_eq(true, false); }
    } else {
        expect_eq(true, false);
    }
});

test("3. hprofBasicTypeSize covers all Java basic types for 32-bit and 64-bit object IDs", () => {
    expect_eq(hprofBasicTypeSize(HprofBasicType.object, 4) ?? 0, 4);
    expect_eq(hprofBasicTypeSize(HprofBasicType.object, 8) ?? 0, 8);
    expect_eq(hprofBasicTypeSize(HprofBasicType.object, 16) == null, true);
    expect_eq(hprofBasicTypeSize(HprofBasicType.boolean, 8) ?? 0, 1);
    expect_eq(hprofBasicTypeSize(HprofBasicType.byte, 8) ?? 0, 1);
    expect_eq(hprofBasicTypeSize(HprofBasicType.char, 8) ?? 0, 2);
    expect_eq(hprofBasicTypeSize(HprofBasicType.short, 8) ?? 0, 2);
    expect_eq(hprofBasicTypeSize(HprofBasicType.float, 8) ?? 0, 4);
    expect_eq(hprofBasicTypeSize(HprofBasicType.int, 8) ?? 0, 4);
    expect_eq(hprofBasicTypeSize(HprofBasicType.double, 8) ?? 0, 8);
    expect_eq(hprofBasicTypeSize(HprofBasicType.long, 8) ?? 0, 8);
    expect_eq(hprofBasicTypeSize(HprofBasicType.unknown, 8) == null, true);
    expect_eq(decodeHprofBasicType(10) == HprofBasicType.int, true);
    expect_eq(decodeHprofBasicType(99) == HprofBasicType.unknown, true);
    expect_eq(SUB_CLASS_DUMP, 0x20);
    expect_eq(SUB_PRIM_ARRAY_DUMP, 0x23);
});

test("4. summarizeHprof counts UTF8, LOAD_CLASS, HEAP_DUMP/SEGMENT, and total records in one pass", () => {
    let raw: [256]u8 = undefined;
    let buf = raw[0..256];
    let p = writeHeader(buf, "JAVA PROFILE 1.0.2", 8, 1700000000000);

    // 2 UTF8, 1 LOAD_CLASS, 1 HEAP_DUMP, 2 HEAP_DUMP_SEGMENT, 1 HEAP_DUMP_END => 7 total
    p = writeRecordHeader(buf, p, TAG_UTF8, 0, 8);
    p = writeU64BE(buf, p, 1);
    p = writeRecordHeader(buf, p, TAG_UTF8, 0, 8);
    p = writeU64BE(buf, p, 2);
    p = writeRecordHeader(buf, p, TAG_LOAD_CLASS, 0, 24);
    p += 24;
    p = writeRecordHeader(buf, p, TAG_HEAP_DUMP, 0, 0);
    p = writeRecordHeader(buf, p, TAG_HEAP_DUMP_SEGMENT, 0, 0);
    p = writeRecordHeader(buf, p, TAG_HEAP_DUMP_SEGMENT, 0, 0);
    p = writeRecordHeader(buf, p, TAG_HEAP_DUMP_END, 0, 0);

    if (stats := summarizeHprof(buf[0..p])) {
        expect_eq(stats.total_records, 7);
        expect_eq(stats.utf8_count, 2);
        expect_eq(stats.load_class_count, 1);
        expect_eq(stats.heap_dump_segments, 3);
    } else {
        expect_eq(true, false);
    }
});

test("5. rejects bad magic, invalid id_size (16), truncated header, and out-of-bounds record length", () => {
    expect_eq(isHprof("NOT A HPROF FILE"), false);

    let raw: [64]u8 = undefined;
    let buf = raw[0..64];

    // Invalid id_size = 16
    let p = writeHeader(buf, "JAVA PROFILE 1.0.2", 16, 0);
    expect_eq(isHprof(buf[0..p]), false);
    expect_eq(parseHprofHeader(buf[0..p]) == null, true);

    // Truncated header (missing timestamp)
    p = writeHeader(buf, "JAVA PROFILE 1.0.2", 8, 0);
    expect_eq(parseHprofHeader(buf[0..25]) == null, true);

    // Out-of-bounds record length (claims 500 bytes, only 4 available)
    p = writeHeader(buf, "JAVA PROFILE 1.0.2", 8, 0);
    p = writeRecordHeader(buf, p, TAG_UTF8, 0, 500);
    const bad = buf[0 .. p + 4];
    expect_eq(summarizeHprof(bad) == null, true);

    let it = HprofRecordIter.fromData(bad);
    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, true);

    // Truncated sub-parsers
    expect_eq(parseHprofUtf8(buf[0..3], 8) == null, true);
    expect_eq(parseHprofLoadClass(buf[0..10], 8) == null, true);
    expect_eq(parseHprofHeapSummary(buf[0..20]) == null, true);
});
