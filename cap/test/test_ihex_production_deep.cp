// Source: cp/cap/test/test_ihex_production_deep.cp (Production deep tests for cap:ihex)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    IhexRecordType,
    ihexRecordTypeFromByte,
    IhexRecord,
    IhexChunk,
    IhexStats,
    IhexIter,
    ihexChecksum,
    parseIhexLine,
    encodeIhexLine,
    summarizeIhex,
} from "cap:iot";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. STM32/ARM I32HEX firmware image (0x08000000 linear base + data + entry 05 + EOF)", () => {
    const stm32_hex =
        ":020000040800F2\r\n" +
        ":04000000005000208C\r\n" +
        ":0400040009010008E6\r\n" +
        ":0400000508000109E5\r\n" +
        ":00000001FF\r\n";

    let it = IhexIter.init(stm32_hex);

    const c0 = it.next() ?? unreachable;
    expect_eq(c0.abs_address, 0x08000000);
    expect_eq(c0.record.byte_count, 4);
    expect_eq(c0.record.dataByte(0) ?? 0xFF, 0x00);
    expect_eq(c0.record.dataByte(1) ?? 0xFF, 0x50);
    expect_eq(c0.record.dataByte(2) ?? 0xFF, 0x00);
    expect_eq(c0.record.dataByte(3) ?? 0xFF, 0x20);
    expect_eq(c0.record.dataByte(4) == null, true);

    const c1 = it.next() ?? unreachable;
    expect_eq(c1.abs_address, 0x08000004);
    expect_eq(c1.record.dataSlice().len, 4);
    expect_eq(c1.record.dataSlice()[0], 0x09);
    expect_eq(c1.record.dataSlice()[3], 0x08);

    // Exhausted at EOF; entry point 0x08000109 captured
    expect_eq(it.next() == null, true);
    expect_eq(it.has_eof, true);
    expect_eq(it.had_error, false);
    expect_eq(it.start_linear_address ?? 0, 0x08000109);
});

test("2. I16HEX extended segment address (type 02) calculation", () => {
    const i16_hex =
        ":020000021200EA\n" +
        ":02003000DEAD43\n" +
        ":00000001FF\n";

    let it = IhexIter.init(i16_hex);
    const c0 = it.next() ?? unreachable;
    // 0x1200 << 4 = 0x00012000; + 0x0030 = 0x00012030
    expect_eq(c0.abs_address, 0x00012030);
    expect_eq(c0.record.byte_count, 2);
    expect_eq(c0.record.dataByte(0) ?? 0, 0xDE);
    expect_eq(c0.record.dataByte(1) ?? 0, 0xAD);

    expect_eq(it.next() == null, true);
    expect_eq(it.has_eof, true);
    expect_eq(it.had_error, false);
});

test("3. encodeIhexLine -> parseIhexLine roundtrip and checksum verification", () => {
    // 1. Extended linear address 0x0800
    const ela_payload: [2]u8 = [0x08, 0x00];
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    const n1 = encodeIhexLine(0x0000, IhexRecordType.ext_linear_addr, ela_payload[0..], buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n1], ":020000040800F2\n"), true);

    const r1 = parseIhexLine(buf[0..n1]) ?? unreachable;
    expect_eq(r1.record_type, IhexRecordType.ext_linear_addr);
    expect_eq(r1.byte_count, 2);
    expect_eq(r1.checksum, 0xF2);

    // 2. Data record with arbitrary bytes
    const data_payload: [6]u8 = [0x01, 0x23, 0x45, 0x67, 0x89, 0xAB];
    const n2 = encodeIhexLine(0x1A2B, IhexRecordType.data, data_payload[0..], buf[0..]) ?? unreachable;
    const r2 = parseIhexLine(buf[0..n2]) ?? unreachable;
    expect_eq(r2.record_type, IhexRecordType.data);
    expect_eq(r2.address, 0x1A2B);
    expect_eq(r2.byte_count, 6);
    expect_eq(bytesEq(r2.dataSlice(), data_payload[0..]), true);
    expect_eq(r2.checksum, ihexChecksum(6, 0x1A2B, 0, data_payload[0..]));

    // 3. EOF record
    const empty: [0]u8 = [];
    const n3 = encodeIhexLine(0x0000, IhexRecordType.eof, empty[0..], buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n3], ":00000001FF\n"), true);

    // 4. Buffer too small -> null
    let tiny: [10]u8 = std.mem.zeroes([10]u8);
    expect_eq(encodeIhexLine(0x0000, IhexRecordType.eof, empty[0..], tiny[0..]) == null, true);
});

test("4. summarizeIhex address range, byte count, EOF, and entry point statistics", () => {
    const fw =
        ":020000040800F2\n" +
        ":04000000005000208C\n" +
        ":0400040009010008E6\n" +
        ":0400000508000109E5\n" +
        ":00000001FF\n";

    const stats = summarizeIhex(fw);
    expect_eq(stats.data_records, 2);
    expect_eq(stats.total_bytes, 8);
    expect_eq(stats.min_addr, 0x08000000);
    expect_eq(stats.max_addr, 0x08000007);
    expect_eq(stats.has_eof, true);
    expect_eq(stats.entry_addr ?? 0, 0x08000109);
});

test("5. Malformed lines: bad checksum, invalid hex, length mismatch, and type constraint violations", () => {
    // Bad checksum (last byte F3 instead of F2)
    expect_eq(parseIhexLine(":020000040800F3") == null, true);

    // Missing leading ':'
    expect_eq(parseIhexLine("00000001FF") == null, true);

    // Invalid hex character 'G'
    expect_eq(parseIhexLine(":00000001FG") == null, true);

    // Odd hex length / truncated line
    expect_eq(parseIhexLine(":00000001F") == null, true);
    expect_eq(parseIhexLine(":0400000000508C") == null, true);

    // Type constraint violations:
    // EOF with non-zero byte count
    expect_eq(parseIhexLine(":0100000100FE") == null, true);
    // Ext linear address (04) with byte_count != 2
    expect_eq(parseIhexLine(":0100000408F3") == null, true);
    // Start linear address (05) with byte_count != 4
    expect_eq(parseIhexLine(":020000050800F1") == null, true);
    // Unknown record type 0x06
    expect_eq(ihexRecordTypeFromByte(6), IhexRecordType.unknown);
    expect_eq(parseIhexLine(":00000006FA") == null, true);

    // Stream with corrupted line stops and sets had_error=true, has_eof=false
    const broken =
        ":04000000005000208C\n" +
        ":040004000901000800\n" +
        ":00000001FF\n";
    let it = IhexIter.init(broken);
    * = it.next() ?? unreachable;
    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, true);
    expect_eq(it.has_eof, false);

    const bstats = summarizeIhex(broken);
    expect_eq(bstats.data_records, 1);
    expect_eq(bstats.has_eof, false);
});
