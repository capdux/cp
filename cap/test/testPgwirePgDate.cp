import { test, expect } from "cap:test";
import {
    PG_OID_DATE,
    PG_OID_TIMESTAMP,
    PG_OID_TIMESTAMPTZ,
    PG_EPOCH_UNIX_MS,
    PG_EPOCH_UNIX_DAYS,
    PG_MS_PER_DAY,
    PG_TIMESTAMP_POS_INFINITY,
    PG_TIMESTAMP_NEG_INFINITY,
    PG_DATE_POS_INFINITY,
    PG_DATE_NEG_INFINITY,
    PgDateError,
    PgTimestampKind,
    isPgDateTimeOid,
    PgTimestampCodec,
    PgDateCodec,
} from "cap:pgwire";

test("PgTimestampCodec epoch alignment, negative floor division, and f64 sub-millisecond precision", () => {
    let buf: [8]u8 = [0; 8];

    // 2000-01-01T00:00:00.000Z <-> pg_us = 0
    const n0 = PgTimestampCodec.encodeUnixMs(PG_EPOCH_UNIX_MS, buf[0..])?;
    expect(n0).toBe(8);
    expect(PgTimestampCodec.decodeMicros(buf[0..])?).toBe(0);
    expect(PgTimestampCodec.fromBinaryToUnixMs(buf[0..])?).toBe(PG_EPOCH_UNIX_MS);

    // 1970-01-01T00:00:00.000Z <-> pg_us = -946684800000000
    * = PgTimestampCodec.encodeUnixMs(0, buf[0..])?;
    expect(PgTimestampCodec.decodeMicros(buf[0..])?).toBe(-946684800000000);
    expect(PgTimestampCodec.fromBinaryToUnixMs(buf[0..])?).toBe(0);

    // Negative sub-millisecond: -1 us before 2000-01-01 floors to PG_EPOCH_UNIX_MS - 1
    * = PgTimestampCodec.encodeMicros(-1, buf[0..])?;
    expect(PgTimestampCodec.fromBinaryToUnixMs(buf[0..])?).toBe(PG_EPOCH_UNIX_MS - 1);

    // Positive sub-millisecond: +1500 us -> +1.5 ms in f64, +1 ms in floor i64
    * = PgTimestampCodec.encodeMicros(1500, buf[0..])?;
    expect(PgTimestampCodec.fromBinaryToUnixMs(buf[0..])?).toBe(PG_EPOCH_UNIX_MS + 1);
    const ms_f64 = PgTimestampCodec.fromBinaryToUnixMsF64(buf[0..])?;
    expect(ms_f64 == 946684800001.5).toBe(true);
});

test("PgDateCodec 4-byte big-endian days round-trip across Unix and pre-1970 dates", () => {
    let buf: [4]u8 = [0; 4];

    // 2000-01-01 -> pg_days = 0
    expect(PgDateCodec.encodeDays(0, buf[0..])?).toBe(4);
    expect(PgDateCodec.fromBinaryToUnixMs(buf[0..])?).toBe(PG_EPOCH_UNIX_MS);

    // 1970-01-01 -> pg_days = -10957
    expect(PgDateCodec.encodeUnixMs(0, buf[0..])?).toBe(4);
    expect(PgDateCodec.decodeDays(buf[0..])?).toBe(-PG_EPOCH_UNIX_DAYS);
    expect(PgDateCodec.fromBinaryToUnixMs(buf[0..])?).toBe(0);

    // 1969-12-31 23:59:59.999Z (-1 ms) floors to 1969-12-31 (-10958 days)
    expect(PgDateCodec.unixMsToPgDays(-1)?).toBe(-PG_EPOCH_UNIX_DAYS - 1);
    expect(PgDateCodec.pgDaysToUnixMs(-PG_EPOCH_UNIX_DAYS - 1)?).toBe(-PG_MS_PER_DAY);

    // 2024-01-01 (8766 days after 2000-01-01)
    expect(PgDateCodec.encodeDays(8766, buf[0..])?).toBe(4);
    const ms_2024 = PgDateCodec.fromBinaryToUnixMs(buf[0..])?;
    expect(ms_2024).toBe(1704067200000);
    expect(PgDateCodec.unixMsToPgDays(ms_2024 + 43200000)?).toBe(8766);
});

test("isPgDateTimeOid recognizes DATE, TIMESTAMP, and TIMESTAMPTZ OIDs", () => {
    expect(isPgDateTimeOid(PG_OID_DATE)).toBe(true);
    expect(isPgDateTimeOid(PG_OID_TIMESTAMP)).toBe(true);
    expect(isPgDateTimeOid(PG_OID_TIMESTAMPTZ)).toBe(true);
    expect(isPgDateTimeOid(23)).toBe(false);
    expect(isPgDateTimeOid(25)).toBe(false);
    expect(isPgDateTimeOid(0)).toBe(false);
});

test("PgTimestampCodec and PgDateCodec classify and guard PostgreSQL +/-infinity sentinels", () => {
    expect(PgTimestampCodec.classifyMicros(0) == PgTimestampKind.Finite).toBe(true);
    expect(PgTimestampCodec.classifyMicros(PG_TIMESTAMP_POS_INFINITY) == PgTimestampKind.PosInfinity).toBe(true);
    expect(PgTimestampCodec.classifyMicros(PG_TIMESTAMP_NEG_INFINITY) == PgTimestampKind.NegInfinity).toBe(true);

    expect(PgDateCodec.classifyDays(0) == PgTimestampKind.Finite).toBe(true);
    expect(PgDateCodec.classifyDays(PG_DATE_POS_INFINITY) == PgTimestampKind.PosInfinity).toBe(true);
    expect(PgDateCodec.classifyDays(PG_DATE_NEG_INFINITY) == PgTimestampKind.NegInfinity).toBe(true);

    let inf_ts_err = false;
    * = PgTimestampCodec.pgMicrosToUnixMs(PG_TIMESTAMP_POS_INFINITY) catch |e| {
        if (e == PgDateError.InfiniteTimestamp) {
            inf_ts_err = true;
        }
        0
    };
    expect(inf_ts_err).toBe(true);

    let inf_date_err = false;
    * = PgDateCodec.pgDaysToUnixMs(PG_DATE_NEG_INFINITY) catch |e| {
        if (e == PgDateError.InfiniteTimestamp) {
            inf_date_err = true;
        }
        0
    };
    expect(inf_date_err).toBe(true);
});

test("PgTimestampCodec and PgDateCodec enforce InvalidLength, BufferTooSmall, and OutOfRange guards", () => {
    const short7: [7]u8 = [0; 7];
    let got_bad_ts_len = false;
    * = PgTimestampCodec.decodeMicros(short7[0..]) catch |e| {
        if (e == PgDateError.InvalidLength) {
            got_bad_ts_len = true;
        }
        0
    };
    expect(got_bad_ts_len).toBe(true);

    const short3: [3]u8 = [0; 3];
    let got_bad_date_len = false;
    * = PgDateCodec.decodeDays(short3[0..]) catch |e| {
        if (e == PgDateError.InvalidLength) {
            got_bad_date_len = true;
        }
        0
    };
    expect(got_bad_date_len).toBe(true);

    let tiny: [3]u8 = [0; 3];
    let got_small_buf = false;
    * = PgDateCodec.encodeDays(10, tiny[0..]) catch |e| {
        if (e == PgDateError.BufferTooSmall) {
            got_small_buf = true;
        }
        0
    };
    expect(got_small_buf).toBe(true);

    let got_overflow = false;
    * = PgTimestampCodec.unixMsToPgMicros(9223372036854775807) catch |e| {
        if (e == PgDateError.OutOfRange) {
            got_overflow = true;
        }
        0
    };
    expect(got_overflow).toBe(true);
});
