import { test, expect } from "cap:test";
import {
    MYSQL_BINARY_CHARSET,
    MYSQL_BINARY_ROW_HEADER,
    MySqlBinaryValueError,
    MySqlBinaryValueKind,
    mysqlBinaryRowNullBitmapLen,
    isBinaryRowColNull,
    decodeMySqlBinaryValue,
} from "cap:mysql";

test("decodeMySqlBinaryValue integers (TINY/SHORT/INT24/LONG/LONGLONG) and IEEE-754 floats", () => {
    expect(MYSQL_BINARY_CHARSET).toBe(63);
    expect(MYSQL_BINARY_ROW_HEADER).toBe(0x00);

    // TINY: 0xFF -> -1 signed, 255 unsigned
    const b1: [1]u8 = [0xFF];
    const tiny_s = decodeMySqlBinaryValue(b1[0..], 0x01, false, false, 45)?;
    expect(tiny_s.kind == MySqlBinaryValueKind.Int64).toBe(true);
    expect(tiny_s.int_val).toBe(-1);
    expect(tiny_s.consumed).toBe(1);

    const tiny_u = decodeMySqlBinaryValue(b1[0..], 0x01, true, false, 45)?;
    expect(tiny_u.kind == MySqlBinaryValueKind.Uint64).toBe(true);
    expect(tiny_u.uint_val).toBe(255);

    // SHORT (-2 in LE: 0xFE, 0xFF) & YEAR (2026 = 0x07EA -> LE: 0xEA, 0x07)
    const b_short: [2]u8 = [0xFE, 0xFF];
    const short_s = decodeMySqlBinaryValue(b_short[0..], 0x02, false, false, 45)?;
    expect(short_s.int_val).toBe(-2);
    expect(short_s.consumed).toBe(2);

    const b_year: [2]u8 = [0xEA, 0x07];
    const year_v = decodeMySqlBinaryValue(b_year[0..], 0x0D, false, false, 45)?;
    expect(year_v.kind == MySqlBinaryValueKind.Uint64).toBe(true);
    expect(year_v.uint_val).toBe(2026);

    // INT24: 0xFFFFFF -> -1 signed, 16777215 unsigned
    const b24: [3]u8 = [0xFF, 0xFF, 0xFF];
    const i24_s = decodeMySqlBinaryValue(b24[0..], 0x09, false, false, 45)?;
    expect(i24_s.int_val).toBe(-1);
    expect(i24_s.consumed).toBe(3);

    const i24_u = decodeMySqlBinaryValue(b24[0..], 0x09, true, false, 45)?;
    expect(i24_u.uint_val).toBe(16777215);

    // LONG: 42 (0x2A, 0, 0, 0)
    const b32: [4]u8 = [0x2A, 0x00, 0x00, 0x00];
    const long_v = decodeMySqlBinaryValue(b32[0..], 0x03, false, false, 45)?;
    expect(long_v.int_val).toBe(42);
    expect(long_v.consumed).toBe(4);

    // LONGLONG unsigned: 0x0100000000 (4294967296)
    const b64: [8]u8 = [0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00];
    const ll_u = decodeMySqlBinaryValue(b64[0..], 0x08, true, false, 45)?;
    expect(ll_u.uint_val).toBe(4294967296);
    expect(ll_u.consumed).toBe(8);

    // IEEE-754 FLOAT 1.5 (0x3FC00000 -> LE: 00 00 C0 3F)
    const f32_le: [4]u8 = [0x00, 0x00, 0xC0, 0x3F];
    const fv = decodeMySqlBinaryValue(f32_le[0..], 0x04, false, false, 45)?;
    expect(fv.kind == MySqlBinaryValueKind.Float64).toBe(true);
    expect(fv.float_val == 1.5).toBe(true);

    // IEEE-754 DOUBLE -2.25 (0xC002000000000000)
    const f64_le: [8]u8 = [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xC0];
    const dv = decodeMySqlBinaryValue(f64_le[0..], 0x05, false, false, 45)?;
    expect(dv.float_val == -2.25).toBe(true);
});

test("decodeMySqlBinaryValue DATE/DATETIME/TIMESTAMP (0/4/7/11 bytes) and formatIso", () => {
    // 0-byte zero date
    const d0: [1]u8 = [0];
    const v0 = decodeMySqlBinaryValue(d0[0..], 0x0A, false, false, 45)?;
    expect(v0.kind == MySqlBinaryValueKind.DateTime).toBe(true);
    expect(v0.datetime_val.year).toBe(0);
    expect(v0.consumed).toBe(1);

    // 4-byte DATE: 2026-11-22 (2026 = 0x07EA)
    const d4: [5]u8 = [4, 0xEA, 0x07, 11, 22];
    const v4 = decodeMySqlBinaryValue(d4[0..], 0x0A, false, false, 45)?;
    expect(v4.consumed).toBe(5);
    let iso_buf: [32]u8 = [0; 32];
    const n4 = v4.datetime_val.formatIso(iso_buf[0..])?;
    expect(iso_buf[0..n4]).toBe("2026-11-22 00:00:00");

    // 11-byte DATETIME with microseconds: 2026-11-22 21:40:31.123456 (123456 = 0x0001E240)
    const d11: [12]u8 = [11, 0xEA, 0x07, 11, 22, 21, 40, 31, 0x40, 0xE2, 0x01, 0x00];
    const v11 = decodeMySqlBinaryValue(d11[0..], 0x0C, false, false, 45)?;
    expect(v11.consumed).toBe(12);
    const n11 = v11.datetime_val.formatIso(iso_buf[0..])?;
    expect(iso_buf[0..n11]).toBe("2026-11-22 21:40:31.123456");
});

test("decodeMySqlBinaryValue TIME (0/8/12 bytes, negative and multi-day hours) and formatHms", () => {
    let hms_buf: [32]u8 = [0; 32];

    // 0-byte zero TIME
    const t0: [1]u8 = [0];
    const v0 = decodeMySqlBinaryValue(t0[0..], 0x0B, false, false, 45)?;
    expect(v0.kind == MySqlBinaryValueKind.Time).toBe(true);
    const n0 = v0.time_val.formatHms(hms_buf[0..])?;
    expect(hms_buf[0..n0]).toBe("00:00:00");

    // 8-byte negative multi-day TIME: -(5 days, 4 hours, 9 min, 7 sec) -> "-124:09:07"
    const t8: [9]u8 = [8, 1, 5, 0, 0, 0, 4, 9, 7];
    const v8 = decodeMySqlBinaryValue(t8[0..], 0x0B, false, false, 45)?;
    expect(v8.consumed).toBe(9);
    expect(v8.time_val.negative).toBe(true);
    expect(v8.time_val.totalHours()).toBe(124);
    const n8 = v8.time_val.formatHms(hms_buf[0..])?;
    expect(hms_buf[0..n8]).toBe("-124:09:07");

    // 12-byte TIME with microseconds
    const t12: [13]u8 = [12, 0, 0, 0, 0, 0, 8, 30, 45, 0xE8, 0x03, 0x00, 0x00];
    const v12 = decodeMySqlBinaryValue(t12[0..], 0x0B, false, false, 45)?;
    expect(v12.consumed).toBe(13);
    expect(v12.time_val.microsecond).toBe(1000);
    const n12 = v12.time_val.formatHms(hms_buf[0..])?;
    expect(hms_buf[0..n12]).toBe("08:30:45");
});

test("decodeMySqlBinaryValue lenenc text/bytes, charset 63, NULL, and binary row NULL bitmap", () => {
    // VAR_STRING UTF-8 (charset 45) -> Text
    const s_pkt: [6]u8 = [5, 'h', 'e', 'l', 'l', 'o'];
    const txt = decodeMySqlBinaryValue(s_pkt[0..], 0xFD, false, false, 45)?;
    expect(txt.kind == MySqlBinaryValueKind.Text).toBe(true);
    expect(txt.slice_val).toBe("hello");
    expect(txt.consumed).toBe(6);

    // Same bytes with MYSQL_BINARY_CHARSET (63) -> Bytes
    const raw = decodeMySqlBinaryValue(s_pkt[0..], 0xFD, false, false, MYSQL_BINARY_CHARSET)?;
    expect(raw.kind == MySqlBinaryValueKind.Bytes).toBe(true);
    expect(raw.slice_val).toBe("hello");

    // NULL type consumes 0 bytes
    const nv = decodeMySqlBinaryValue(s_pkt[0..], 0x06, false, false, 45)?;
    expect(nv.kind == MySqlBinaryValueKind.Null).toBe(true);
    expect(nv.consumed).toBe(0);

    // Binary row NULL bitmap has 2-bit offset:
    // 6 columns -> (6 + 7 + 2)/8 = 1 byte; 7 columns -> (7 + 7 + 2)/8 = 2 bytes
    expect(mysqlBinaryRowNullBitmapLen(6)).toBe(1);
    expect(mysqlBinaryRowNullBitmapLen(7)).toBe(2);
    // Bit 2 (col 0) and bit 8 (col 6 -> byte 1, bit 0) set
    const bitmap: [2]u8 = [0x04, 0x01];
    expect(isBinaryRowColNull(bitmap[0..], 0)).toBe(true);
    expect(isBinaryRowColNull(bitmap[0..], 1)).toBe(false);
    expect(isBinaryRowColNull(bitmap[0..], 6)).toBe(true);
    expect(isBinaryRowColNull(bitmap[0..], 7)).toBe(false);
});

test("decodeMySqlBinaryValue error guards for Truncated, InvalidLength, InvalidLenEncInt, and UnsupportedFieldType", () => {
    const empty: [0]u8 = [];
    let got_trunc = false;
    * = decodeMySqlBinaryValue(empty[0..], 0x03, false, false, 45) catch |e| {
        if (e == MySqlBinaryValueError.Truncated) {
            got_trunc = true;
        }
        decodeMySqlBinaryValue(empty[0..], 0x06, false, false, 45) catch unreachable
    };
    expect(got_trunc).toBe(true);

    // Invalid DATE length = 5 (only 0, 4, 7, 11 allowed)
    const bad_date: [6]u8 = [5, 0, 0, 0, 0, 0];
    let got_bad_len = false;
    * = decodeMySqlBinaryValue(bad_date[0..], 0x0A, false, false, 45) catch |e| {
        if (e == MySqlBinaryValueError.InvalidLength) {
            got_bad_len = true;
        }
        decodeMySqlBinaryValue(empty[0..], 0x06, false, false, 45) catch unreachable
    };
    expect(got_bad_len).toBe(true);

    // Invalid lenenc prefix 0xFB
    const bad_le: [1]u8 = [0xFB];
    let got_bad_le = false;
    * = decodeMySqlBinaryValue(bad_le[0..], 0xFD, false, false, 45) catch |e| {
        if (e == MySqlBinaryValueError.InvalidLenEncInt) {
            got_bad_le = true;
        }
        decodeMySqlBinaryValue(empty[0..], 0x06, false, false, 45) catch unreachable
    };
    expect(got_bad_le).toBe(true);

    // Unsupported field type 0x77
    let got_unsup = false;
    * = decodeMySqlBinaryValue(bad_le[0..], 0x77, false, false, 45) catch |e| {
        if (e == MySqlBinaryValueError.UnsupportedFieldType) {
            got_unsup = true;
        }
        decodeMySqlBinaryValue(empty[0..], 0x06, false, false, 45) catch unreachable
    };
    expect(got_unsup).toBe(true);
});
