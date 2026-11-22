// Production Deep Tests for cap:sql MySQL Length-Encoded Integer & String Codec (lib/sql/mysql_lenenc.cp)
import { test, expect_eq } from "cap:test";
import {
    MYSQL_LENENC_NULL_MARKER,
    MYSQL_LENENC_U16_MARKER,
    MYSQL_LENENC_U24_MARKER,
    MYSQL_LENENC_U64_MARKER,
    MYSQL_LENENC_ERR_MARKER,
    MySqlLenEncError,
    EncodedLengthInt,
    DecodeLengthIntResult,
    DecodeLengthStringResult,
    encodedLengthIntSize,
    encodeLengthInt,
    encodeLengthIntInto,
    decodeLengthInt,
    decodeLengthIntStrict,
    encodeLengthString,
    decodeLengthString,
} from "cap:sql";

test("sql mysql_lenenc: 1/3/4/9-byte boundary values roundtrip", fn() {
    const cases: [10]u64 = [
        0,
        1,
        250,
        251,
        0xffff,
        0x10000,
        0xffffff,
        0x1000000,
        0x0123456789abcdef,
        0xffffffffffffffff,
    ];
    const expected_lens: [10]usize = [1, 1, 1, 3, 3, 4, 4, 9, 9, 9];

    let i: usize = 0;
    while (i < cases.len) {
        const v = cases[i];
        const want_len = expected_lens[i];
        expect_eq(encodedLengthIntSize(v), want_len);

        const enc = encodeLengthInt(v);
        expect_eq(enc.len, want_len);

        const decoded = decodeLengthInt(enc.slice()) ?? unreachable;
        expect_eq(decoded.value, v);
        expect_eq(decoded.bytes_read, want_len);
        i += 1;
    }

    // Verify exact little-endian wire layout for 0x0102030405060708
    const le = encodeLengthInt(0x0102030405060708);
    expect_eq(le.bytes[0], MYSQL_LENENC_U64_MARKER);
    expect_eq(le.bytes[1], @as(u8, 0x08));
    expect_eq(le.bytes[2], @as(u8, 0x07));
    expect_eq(le.bytes[8], @as(u8, 0x01));
});

test("sql mysql_lenenc: encodeLengthIntInto exact fit and BufferTooSmall guard", fn() {
    let small: [2]u8 = [0, 0];
    // 250 needs 1 byte -> succeeds
    const n1 = encodeLengthIntInto(250, small[0..]) catch unreachable;
    expect_eq(n1, @as(usize, 1));
    expect_eq(small[0], @as(u8, 250));

    // 251 needs 3 bytes -> BufferTooSmall
    let too_small = false;
    * = encodeLengthIntInto(251, small[0..]) catch |e| {
        too_small = (e == MySqlLenEncError.BufferTooSmall);
        @as(usize, 0)
    };
    expect_eq(too_small, true);

    let fit3: [3]u8 = [0, 0, 0];
    const n3 = encodeLengthIntInto(0x1234, fit3[0..]) catch unreachable;
    expect_eq(n3, @as(usize, 3));
    expect_eq(fit3[0], MYSQL_LENENC_U16_MARKER);
    expect_eq(fit3[1], @as(u8, 0x34));
    expect_eq(fit3[2], @as(u8, 0x12));
});

test("sql mysql_lenenc: decodeLengthInt returns null on truncated wire frames", fn() {
    const empty: [0]u8 = [];
    expect_eq(decodeLengthInt(empty[0..]) == null, true);

    const trunc16: [2]u8 = [MYSQL_LENENC_U16_MARKER, 0x01];
    expect_eq(decodeLengthInt(trunc16[0..]) == null, true);

    const trunc24: [3]u8 = [MYSQL_LENENC_U24_MARKER, 0x01, 0x02];
    expect_eq(decodeLengthInt(trunc24[0..]) == null, true);

    const trunc64: [8]u8 = [MYSQL_LENENC_U64_MARKER, 1, 2, 3, 4, 5, 6, 7];
    expect_eq(decodeLengthInt(trunc64[0..]) == null, true);
});

test("sql mysql_lenenc: decodeLengthIntStrict distinguishes NULL, ERR, and Truncated", fn() {
    const null_pkt: [1]u8 = [MYSQL_LENENC_NULL_MARKER];
    let is_null = false;
    * = decodeLengthIntStrict(null_pkt[0..]) catch |e| {
        is_null = (e == MySqlLenEncError.NullMarker);
        DecodeLengthIntResult{ value: 0, bytes_read: 0 }
    };
    expect_eq(is_null, true);

    const err_pkt: [1]u8 = [MYSQL_LENENC_ERR_MARKER];
    let is_err = false;
    * = decodeLengthIntStrict(err_pkt[0..]) catch |e| {
        is_err = (e == MySqlLenEncError.ErrMarker);
        DecodeLengthIntResult{ value: 0, bytes_read: 0 }
    };
    expect_eq(is_err, true);

    const trunc_pkt: [2]u8 = [MYSQL_LENENC_U24_MARKER, 0xaa];
    let is_trunc = false;
    * = decodeLengthIntStrict(trunc_pkt[0..]) catch |e| {
        is_trunc = (e == MySqlLenEncError.TruncatedInput);
        DecodeLengthIntResult{ value: 0, bytes_read: 0 }
    };
    expect_eq(is_trunc, true);
});

test("sql mysql_lenenc: sequential encodeLengthString and zero-copy decodeLengthString", fn() {
    let wire: [512]u8 = [0; 512];
    let pos: usize = 0;

    // Field 1: empty string
    pos += encodeLengthString("", wire[pos..]) catch unreachable;
    // Field 2: short ASCII string
    pos += encodeLengthString("users_table", wire[pos..]) catch unreachable;
    // Field 3: 300-byte payload (forces 3-byte 0xfc length prefix)
    let big: [300]u8 = ['A'; 300];
    big[0] = 'X';
    big[299] = 'Z';
    pos += encodeLengthString(big[0..], wire[pos..]) catch unreachable;

    // Expected total: (1+0) + (1+11) + (3+300) = 316 bytes
    expect_eq(pos, @as(usize, 316));

    // Sequential zero-copy decode
    let off: usize = 0;
    const f1 = decodeLengthString(wire[off..pos]) catch unreachable;
    expect_eq(f1.data, "");
    expect_eq(f1.bytes_read, @as(usize, 1));
    off += f1.bytes_read;

    const f2 = decodeLengthString(wire[off..pos]) catch unreachable;
    expect_eq(f2.data, "users_table");
    expect_eq(f2.bytes_read, @as(usize, 12));
    off += f2.bytes_read;

    const f3 = decodeLengthString(wire[off..pos]) catch unreachable;
    expect_eq(f3.data.len, @as(usize, 300));
    expect_eq(f3.data[0], @as(u8, 'X'));
    expect_eq(f3.data[299], @as(u8, 'Z'));
    expect_eq(f3.bytes_read, @as(usize, 303));
    off += f3.bytes_read;

    expect_eq(off, pos);
});

test("sql mysql_lenenc: malformed length-encoded string overflow and truncation guards", fn() {
    // Malformed frame declaring u64::MAX payload length -> LengthOverflow
    const huge: [9]u8 = [
        MYSQL_LENENC_U64_MARKER,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    ];
    let overflowed = false;
    * = decodeLengthString(huge[0..]) catch |e| {
        overflowed = (e == MySqlLenEncError.LengthOverflow);
        DecodeLengthStringResult{ data: "", bytes_read: 0 }
    };
    expect_eq(overflowed, true);

    // Frame declares 10 bytes but only 3 bytes follow -> TruncatedInput
    const short_str: [4]u8 = [10, 'a', 'b', 'c'];
    let truncated = false;
    * = decodeLengthString(short_str[0..]) catch |e| {
        truncated = (e == MySqlLenEncError.TruncatedInput);
        DecodeLengthStringResult{ data: "", bytes_read: 0 }
    };
    expect_eq(truncated, true);

    // encodeLengthString into tiny buffer -> BufferTooSmall
    let tiny: [3]u8 = [0, 0, 0];
    let buf_err = false;
    * = encodeLengthString("abcd", tiny[0..]) catch |e| {
        buf_err = (e == MySqlLenEncError.BufferTooSmall);
        @as(usize, 0)
    };
    expect_eq(buf_err, true);
});
