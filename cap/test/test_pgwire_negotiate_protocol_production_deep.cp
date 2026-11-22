// Production Deep Tests for cap:pgwire NegotiateProtocolVersion (lib/pgwire/negotiate_protocol.cp)
import { test, expect_eq } from "cap:test";
import {
    PG_TAG_NEGOTIATE_PROTOCOL_VERSION,
    PG_NEGOTIATE_MAX_OPTIONS,
    PgNegotiateError,
    PgNegotiateProtocolVersion,
} from "cap:pgwire";

test("pgwire negotiate_protocol: roundtrip encodeFrame / decodeFrame with multiple options", fn() {
    expect_eq(PG_TAG_NEGOTIATE_PROTOCOL_VERSION, @as(u8, 'v'));
    expect_eq(PG_NEGOTIATE_MAX_OPTIONS, @as(usize, 32));

    let msg = PgNegotiateProtocolVersion.init(0);
    expect_eq(msg.majorVersion(), @as(u16, 3));
    expect_eq(msg.minorVersion(), @as(u16, 0));
    expect_eq(msg.count(), @as(usize, 0));

    const idx0 = msg.addOption("_pq_.compression") catch unreachable;
    const idx1 = msg.addOption("_pq_.trace_id") catch unreachable;
    expect_eq(idx0, @as(usize, 0));
    expect_eq(idx1, @as(usize, 1));
    expect_eq(msg.count(), @as(usize, 2));
    expect_eq(msg.containsOption("_pq_.compression"), true);
    expect_eq(msg.containsOption("_pq_.missing"), false);

    // Payload = 8 + (16 + 1) + (13 + 1) = 39; Frame = 5 + 39 = 44
    expect_eq(msg.encodedPayloadLen(), @as(usize, 39));
    expect_eq(msg.encodedFrameLen(), @as(usize, 44));

    let buf: [64]u8 = [0; 64];
    const written = msg.encodeFrame(buf[0..]) catch unreachable;
    expect_eq(written, @as(usize, 44));
    expect_eq(buf[0], @as(u8, 'v'));
    // Big-endian length = 4 + 39 = 43 = 0x0000002B
    expect_eq(buf[1], @as(u8, 0));
    expect_eq(buf[2], @as(u8, 0));
    expect_eq(buf[3], @as(u8, 0));
    expect_eq(buf[4], @as(u8, 43));

    const decoded = PgNegotiateProtocolVersion.decodeFrame(buf[0..written]) catch unreachable;
    expect_eq(decoded.eql(msg), true);
    expect_eq(decoded.optionAt(0) ?? "", "_pq_.compression");
    expect_eq(decoded.optionAt(1) ?? "", "_pq_.trace_id");
    expect_eq(decoded.optionAt(2) == null, true);
});

test("pgwire negotiate_protocol: zero unrecognized options and full major.minor version", fn() {
    // Packed 3.2 version = (3 << 16) | 2 = 0x00030002
    const msg = PgNegotiateProtocolVersion.init(0x00030002);
    expect_eq(msg.majorVersion(), @as(u16, 3));
    expect_eq(msg.minorVersion(), @as(u16, 2));
    expect_eq(msg.encodedPayloadLen(), @as(usize, 8));
    expect_eq(msg.encodedFrameLen(), @as(usize, 13));

    let buf: [32]u8 = [0; 32];
    const n = msg.encodeFrame(buf[0..]) catch unreachable;
    expect_eq(n, @as(usize, 13));

    const decoded = PgNegotiateProtocolVersion.decodeFrame(buf[0..n]) catch unreachable;
    expect_eq(decoded.eql(msg), true);
    expect_eq(decoded.majorVersion(), @as(u16, 3));
    expect_eq(decoded.minorVersion(), @as(u16, 2));
    expect_eq(decoded.count(), @as(usize, 0));
});

test("pgwire negotiate_protocol: decodeFrame tag, length, and trailing-bytes validation", fn() {
    // Wrong tag ('E' instead of 'v')
    const bad_tag: [13]u8 = ['E', 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0];
    let got_tag_err = false;
    * = PgNegotiateProtocolVersion.decodeFrame(bad_tag[0..]) catch |e| blk: {
        expect_eq(e, PgNegotiateError.InvalidTag);
        got_tag_err = true;
        break :blk PgNegotiateProtocolVersion{};
    };
    expect_eq(got_tag_err, true);

    // Length field < 12
    const short_len: [13]u8 = ['v', 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0];
    let got_len_err = false;
    * = PgNegotiateProtocolVersion.decodeFrame(short_len[0..]) catch |e| blk: {
        expect_eq(e, PgNegotiateError.InvalidLength);
        got_len_err = true;
        break :blk PgNegotiateProtocolVersion{};
    };
    expect_eq(got_len_err, true);

    // Frame length mismatch (declared 14 bytes, supplied 13)
    const mismatch_len: [13]u8 = ['v', 0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 0];
    let got_mismatch = false;
    * = PgNegotiateProtocolVersion.decodeFrame(mismatch_len[0..]) catch |e| blk: {
        expect_eq(e, PgNegotiateError.InvalidLength);
        got_mismatch = true;
        break :blk PgNegotiateProtocolVersion{};
    };
    expect_eq(got_mismatch, true);

    // Trailing unexpected bytes after declared options
    const trailing: [9]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0x41];
    let got_trailing = false;
    * = PgNegotiateProtocolVersion.decodePayload(trailing[0..]) catch |e| blk: {
        expect_eq(e, PgNegotiateError.TrailingBytes);
        got_trailing = true;
        break :blk PgNegotiateProtocolVersion{};
    };
    expect_eq(got_trailing, true);
});

test("pgwire negotiate_protocol: truncated payload and missing NUL terminator protection", fn() {
    // Payload shorter than 8 bytes
    const too_short: [6]u8 = [0, 0, 0, 0, 0, 0];
    let got_trunc1 = false;
    * = PgNegotiateProtocolVersion.decodePayload(too_short[0..]) catch |e| blk: {
        expect_eq(e, PgNegotiateError.Truncated);
        got_trunc1 = true;
        break :blk PgNegotiateProtocolVersion{};
    };
    expect_eq(got_trunc1, true);

    // Count = 1, option bytes lack NUL terminator
    const no_nul: [11]u8 = [0, 0, 0, 0, 0, 0, 0, 1, 'f', 'o', 'o'];
    let got_trunc2 = false;
    * = PgNegotiateProtocolVersion.decodePayload(no_nul[0..]) catch |e| blk: {
        expect_eq(e, PgNegotiateError.Truncated);
        got_trunc2 = true;
        break :blk PgNegotiateProtocolVersion{};
    };
    expect_eq(got_trunc2, true);
});

test("pgwire negotiate_protocol: negative/excessive option count & BufferTooSmall protection", fn() {
    // Negative Int32 count (0xFFFFFFFF) -> TooManyOptions
    const neg_cnt: [8]u8 = [0, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF];
    let got_neg = false;
    * = PgNegotiateProtocolVersion.decodePayload(neg_cnt[0..]) catch |e| blk: {
        expect_eq(e, PgNegotiateError.TooManyOptions);
        got_neg = true;
        break :blk PgNegotiateProtocolVersion{};
    };
    expect_eq(got_neg, true);

    // Exceeding PG_NEGOTIATE_MAX_OPTIONS via addOption
    let full = PgNegotiateProtocolVersion.init(0);
    let i: usize = 0;
    while (i < PG_NEGOTIATE_MAX_OPTIONS) {
        * = full.addOption("opt") catch unreachable;
        i += 1;
    }
    let got_full = false;
    * = full.addOption("overflow") catch |e| blk: {
        expect_eq(e, PgNegotiateError.TooManyOptions);
        got_full = true;
        break :blk 0;
    };
    expect_eq(got_full, true);

    // Encode into undersized buffer -> BufferTooSmall
    let small: [8]u8 = [0; 8];
    let got_small = false;
    * = full.encodeFrame(small[0..]) catch |e| blk: {
        expect_eq(e, PgNegotiateError.BufferTooSmall);
        got_small = true;
        break :blk 0;
    };
    expect_eq(got_small, true);
});
