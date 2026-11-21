// Source: cp/cap/test/test_rdb_production_deep.cp (Production deep tests for cap:rdb)

import { test, expect_eq } from "cap:test";
import {
    RDB_MAGIC,
    RDB_HEADER_SIZE,
    OP_IDLE,
    OP_FREQ,
    OP_AUX,
    OP_RESIZEDB,
    OP_EXPIRETIME_MS,
    OP_EXPIRETIME,
    OP_SELECTDB,
    OP_EOF,
    RDB_TYPE_STRING,
    RdbLengthKind,
    RdbEventKind,
    RdbHeader,
    RdbLength,
    RdbStringVal,
    RdbEvent,
    isRdb,
    parseRdbHeader,
    readRdbLength,
    readRdbString,
    RdbReader,
} from "cap:rdb";

fn writeBytes(dst: []u8, off: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
    return off + src.len;
}

fn writePlainStr(dst: []u8, off: usize, s: []const u8): usize {
    dst[off] = s.len as u8; // 6-bit length (< 64)
    return writeBytes(dst, off + 1, s);
}

fn writeU32LE(dst: []u8, off: usize, val: u32): usize {
    dst[off + 0] = (val & 0xFF) as u8;
    dst[off + 1] = ((val >> 8) & 0xFF) as u8;
    dst[off + 2] = ((val >> 16) & 0xFF) as u8;
    dst[off + 3] = ((val >> 24) & 0xFF) as u8;
    return off + 4;
}

fn writeU64LE(dst: []u8, off: usize, val: u64): usize {
    let p = writeU32LE(dst, off, (val & 0xFFFFFFFF) as u32);
    p = writeU32LE(dst, p, ((val >> 32) & 0xFFFFFFFF) as u32);
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

test("1. streams full RDB v11 snapshot with AUX, SELECTDB, RESIZEDB, sec/ms expire String KVs, and EOF CRC64", () => {
    let raw: [256]u8 = undefined;
    let buf = raw[0..256];
    let p: usize = 0;

    // Header: "REDIS0011"
    p = writeBytes(buf, p, "REDIS0011");

    // OP_AUX: redis-ver = 7.2.4
    buf[p] = OP_AUX;
    p += 1;
    p = writePlainStr(buf, p, "redis-ver");
    p = writePlainStr(buf, p, "7.2.4");

    // OP_SELECTDB: db 2
    buf[p] = OP_SELECTDB;
    buf[p + 1] = 2;
    p += 2;

    // OP_RESIZEDB: db_size=10, expires_size=2
    buf[p] = OP_RESIZEDB;
    buf[p + 1] = 10;
    buf[p + 2] = 2;
    p += 3;

    // KV 1: OP_EXPIRETIME (seconds=1700000000) + RDB_TYPE_STRING "k1" => "hello"
    buf[p] = OP_EXPIRETIME;
    p = writeU32LE(buf, p + 1, 1700000000);
    buf[p] = RDB_TYPE_STRING;
    p += 1;
    p = writePlainStr(buf, p, "k1");
    p = writePlainStr(buf, p, "hello");

    // KV 2: OP_EXPIRETIME_MS (ms=1700000123456) + OP_FREQ + RDB_TYPE_STRING "k2" => int8(42)
    buf[p] = OP_EXPIRETIME_MS;
    p = writeU64LE(buf, p + 1, 1700000123456);
    buf[p] = OP_FREQ;
    buf[p + 1] = 5;
    p += 2;
    buf[p] = RDB_TYPE_STRING;
    p += 1;
    p = writePlainStr(buf, p, "k2");
    buf[p] = 0xC0; // int8 encoding
    buf[p + 1] = 42;
    p += 2;

    // KV 3: no expire + RDB_TYPE_STRING "k3" => "world"
    buf[p] = RDB_TYPE_STRING;
    p += 1;
    p = writePlainStr(buf, p, "k3");
    p = writePlainStr(buf, p, "world");

    // OP_EOF + 8-byte CRC64
    buf[p] = OP_EOF;
    p = writeU64LE(buf, p + 1, 0x1122334455667788);

    const rdb = buf[0..p];
    expect_eq(RDB_HEADER_SIZE, 9);
    expect_eq(isRdb(rdb), true);
    if (hdr := parseRdbHeader(rdb)) {
        expect_eq(hdr.version, 11);
    } else {
        expect_eq(true, false);
    }

    let reader = RdbReader.init(rdb);

    // Event 1: AUX
    if (ev := reader.next()) {
        expect_eq(ev.kind == RdbEventKind.aux, true);
        expect_eq(sliceEq(ev.key.raw, "redis-ver"), true);
        expect_eq(sliceEq(ev.value.raw, "7.2.4"), true);
    } else {
        expect_eq(true, false);
    }

    // Event 2: SELECTDB
    if (ev := reader.next()) {
        expect_eq(ev.kind == RdbEventKind.select_db, true);
        expect_eq(ev.db_index, 2);
    } else {
        expect_eq(true, false);
    }

    // Event 3: RESIZEDB
    if (ev := reader.next()) {
        expect_eq(ev.kind == RdbEventKind.resize_db, true);
        expect_eq(ev.db_size, 10);
        expect_eq(ev.expires_size, 2);
    } else {
        expect_eq(true, false);
    }

    // Event 4: KV 1 (sec expire -> ms)
    if (ev := reader.next()) {
        expect_eq(ev.kind == RdbEventKind.string_kv, true);
        expect_eq(sliceEq(ev.key.raw, "k1"), true);
        expect_eq(sliceEq(ev.value.raw, "hello"), true);
        if (exp := ev.expire_ms) {
            expect_eq(exp, 1700000000000);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Event 5: KV 2 (ms expire + int8 value)
    if (ev := reader.next()) {
        expect_eq(ev.kind == RdbEventKind.string_kv, true);
        expect_eq(sliceEq(ev.key.raw, "k2"), true);
        expect_eq(ev.value.kind == RdbLengthKind.int8, true);
        expect_eq(ev.value.int_val, 42);
        if (exp := ev.expire_ms) {
            expect_eq(exp, 1700000123456);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Event 6: KV 3 (no expire)
    if (ev := reader.next()) {
        expect_eq(ev.kind == RdbEventKind.string_kv, true);
        expect_eq(sliceEq(ev.key.raw, "k3"), true);
        expect_eq(sliceEq(ev.value.raw, "world"), true);
        expect_eq(ev.expire_ms == null, true);
    } else {
        expect_eq(true, false);
    }

    // Event 7: EOF + checksum
    if (ev := reader.next()) {
        expect_eq(ev.kind == RdbEventKind.eof, true);
        expect_eq(ev.checksum, 0x1122334455667788);
    } else {
        expect_eq(true, false);
    }

    expect_eq(reader.next() == null, true);
    expect_eq(reader.had_error, false);
});

test("2. decodes all 4 RDB plain length encodings (6-bit, 14-bit BE, 32-bit BE, 64-bit BE)", () => {
    // 6-bit: 0x2A => 42
    const b6: [1]u8 = [0x2A];
    if (l := readRdbLength(b6[0..1], 0)) {
        expect_eq(l.kind == RdbLengthKind.plain_len, true);
        expect_eq(l.value, 42);
        expect_eq(l.next_pos, 1);
    } else {
        expect_eq(true, false);
    }

    // 14-bit BE: 01_000010 0xBC => (2 << 8) | 0xBC = 700
    const b14: [2]u8 = [0x42, 0xBC];
    if (l := readRdbLength(b14[0..2], 0)) {
        expect_eq(l.value, 700);
        expect_eq(l.next_pos, 2);
    } else {
        expect_eq(true, false);
    }

    // 32-bit BE: 0x80 + 0x00010000 => 65536
    const b32: [5]u8 = [0x80, 0x00, 0x01, 0x00, 0x00];
    if (l := readRdbLength(b32[0..5], 0)) {
        expect_eq(l.value, 65536);
        expect_eq(l.next_pos, 5);
    } else {
        expect_eq(true, false);
    }

    // 64-bit BE: 0x81 + 0x00000001_00000005 => 4294967301
    const b64: [9]u8 = [0x81, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x05];
    if (l := readRdbLength(b64[0..9], 0)) {
        expect_eq(l.value, 4294967301);
        expect_eq(l.next_pos, 9);
    } else {
        expect_eq(true, false);
    }
});

test("3. decodes signed int8, int16, int32 compact integer strings", () => {
    // int8: -5 => 0xFB
    const s8: [2]u8 = [0xC0, 0xFB];
    if (v := readRdbString(s8[0..2], 0)) {
        expect_eq(v.kind == RdbLengthKind.int8, true);
        expect_eq(v.int_val, -5);
        expect_eq(v.next_pos, 2);
    } else {
        expect_eq(true, false);
    }

    // int16 LE: -1000 => 0xFC18 => [0x18, 0xFC]
    const s16: [3]u8 = [0xC1, 0x18, 0xFC];
    if (v := readRdbString(s16[0..3], 0)) {
        expect_eq(v.kind == RdbLengthKind.int16, true);
        expect_eq(v.int_val, -1000);
        expect_eq(v.next_pos, 3);
    } else {
        expect_eq(true, false);
    }

    // int32 LE: -100000 => 0xFFFE7960 => [0x60, 0x79, 0xFE, 0xFF]
    const s32: [5]u8 = [0xC2, 0x60, 0x79, 0xFE, 0xFF];
    if (v := readRdbString(s32[0..5], 0)) {
        expect_eq(v.kind == RdbLengthKind.int32, true);
        expect_eq(v.int_val, -100000);
        expect_eq(v.next_pos, 5);
    } else {
        expect_eq(true, false);
    }
});

test("4. decodes LZF compressed string boundaries and preserves raw compressed slice", () => {
    // 0xC3 (LZF), clen=4, ulen=20, 4 compressed bytes
    const lzf: [7]u8 = [0xC3, 4, 20, 0xAA, 0xBB, 0xCC, 0xDD];
    if (v := readRdbString(lzf[0..7], 0)) {
        expect_eq(v.kind == RdbLengthKind.lzf, true);
        expect_eq(v.raw.len, 4);
        expect_eq(v.raw[0], 0xAA);
        expect_eq(v.raw[3], 0xDD);
        expect_eq(v.uncompressed_len, 20);
        expect_eq(v.next_pos, 7);
    } else {
        expect_eq(true, false);
    }

    // Truncated LZF payload (clen=10, only 2 bytes left)
    const bad_lzf: [5]u8 = [0xC3, 10, 50, 0x01, 0x02];
    expect_eq(readRdbString(bad_lzf[0..5], 0) == null, true);
});

test("5. rejects truncated header, bad REDIS magic, non-digit version, and truncated EOF checksum", () => {
    expect_eq(isRdb("REDI"), false);
    expect_eq(isRdb("NOTRD0009"), false);
    expect_eq(isRdb("REDIS00AB"), false);
    expect_eq(parseRdbHeader("REDIS00AB") == null, true);

    // Invalid length prefix 0x82 and invalid special encoding 0xC4
    const bad1: [1]u8 = [0x82];
    const bad2: [1]u8 = [0xC4];
    expect_eq(readRdbLength(bad1[0..1], 0) == null, true);
    expect_eq(readRdbLength(bad2[0..1], 0) == null, true);

    // RDB v11 with OP_EOF but missing 8-byte CRC64 checksum
    const trunc_eof = "REDIS0011\xFF\x01\x02";
    let reader = RdbReader.init(trunc_eof);
    expect_eq(reader.next() == null, true);
    expect_eq(reader.had_error, true);
});
