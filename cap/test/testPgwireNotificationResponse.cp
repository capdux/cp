// Deep production regression tests for cap:pgwire notificationResponse.cp
// Covers NotificationResponse ('A') round-trip encoding/decoding, empty payload,
// edge PIDs, malformed frame guards (tag, length, truncation, missing NUL,
// embedded NUL, empty channel, trailing bytes, small buffer).

import { test, expect, expect_eq } from "cap:test";
import {
    PG_TAG_NOTIFICATION_RESPONSE,
    PG_NOTIFY_DEFAULT_MAX_PAYLOAD,
    PgNotificationError,
    PgNotificationResponse,
} from "cap:pgwire";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn expectEncodeErr(res: PgNotificationError!usize, want: PgNotificationError) {
    let got_err = false;
    * = res catch |e| cb: {
        expect_eq(e, want);
        got_err = true;
        break :cb 0;
    };
    expect(got_err);
}

fn expectDecodeErr(res: PgNotificationError!PgNotificationResponse, want: PgNotificationError) {
    let got_err = false;
    * = res catch |e| cb: {
        expect_eq(e, want);
        got_err = true;
        break :cb PgNotificationResponse{};
    };
    expect(got_err);
}

test("pgwire notificationResponse: frame round-trip with JSON payload and wire layout verification", () => {
    expect(PG_TAG_NOTIFICATION_RESPONSE == 'A');
    expect(PG_NOTIFY_DEFAULT_MAX_PAYLOAD == 8000);

    const msg = PgNotificationResponse.init(12345, "orders_ch", "{\"op\":\"insert\",\"id\":42}");
    expect(msg.hasPayload());
    expect(msg.isChannel("orders_ch"));
    expect(!msg.isChannel("other_ch"));
    expect(msg.isWithinDefaultPayloadLimit());

    // Payload len = 4 (pid) + 9 (channel) + 1 + 23 (payload) + 1 = 38
    expect(msg.encodedPayloadLen() == 38);
    expect(msg.encodedFrameLen() == 43);

    let buf: [64]u8 = [0; 64];
    const written = msg.encodeFrame(buf[0..]) catch unreachable;
    expect(written == 43);
    expect(buf[0] == 'A');
    // Wire length = 4 + 38 = 42 = 0x0000002A
    expect(buf[1] == 0 && buf[2] == 0 && buf[3] == 0 && buf[4] == 42);
    // PID = 12345 = 0x00003039
    expect(buf[5] == 0 && buf[6] == 0 && buf[7] == 0x30 && buf[8] == 0x39);

    const decoded = PgNotificationResponse.decodeFrame(buf[0..written]) catch unreachable;
    expect(decoded.eql(msg));
    expect(decoded.pid == 12345);
    expect(strEq(decoded.channel, "orders_ch"));
    expect(strEq(decoded.payload, "{\"op\":\"insert\",\"id\":42}"));
});

test("pgwire notificationResponse: empty payload and payload-only codec round-trip", () => {
    const msg = PgNotificationResponse.init(99, "heartbeat", "");
    expect(!msg.hasPayload());
    expect(msg.encodedPayloadLen() == 4 + 9 + 1 + 0 + 1);

    let pbuf: [32]u8 = [0; 32];
    const pn = msg.encodePayload(pbuf[0..]) catch unreachable;
    expect(pn == 15);
    expect(pbuf[pn - 2] == 0 && pbuf[pn - 1] == 0);

    const decoded_p = PgNotificationResponse.decodePayload(pbuf[0..pn]) catch unreachable;
    expect(decoded_p.eql(msg));
    expect(!decoded_p.hasPayload());
    expect(decoded_p.payload.len == 0);
});

test("pgwire notificationResponse: encode guards reject empty channel, embedded NUL, and small buffers", () => {
    let buf: [64]u8 = [0; 64];

    // Empty channel rejected
    const empty_ch = PgNotificationResponse.init(1, "", "hi");
    expectEncodeErr(empty_ch.encodeFrame(buf[0..]), PgNotificationError.EmptyChannel);

    // Embedded NUL in channel rejected
    let bad_ch: [4]u8 = ['a', 0, 'b', 'c'];
    const nul_ch = PgNotificationResponse.init(1, bad_ch[0..], "ok");
    expectEncodeErr(nul_ch.encodeFrame(buf[0..]), PgNotificationError.EmbeddedNul);

    // Embedded NUL in payload rejected
    let bad_pl: [3]u8 = ['x', 0, 'y'];
    const nul_pl = PgNotificationResponse.init(1, "ch", bad_pl[0..]);
    expectEncodeErr(nul_pl.encodePayload(buf[0..]), PgNotificationError.EmbeddedNul);

    // Buffer too small rejected
    const valid = PgNotificationResponse.init(7, "events", "payload");
    let tiny: [10]u8 = [0; 10];
    expectEncodeErr(valid.encodeFrame(tiny[0..]), PgNotificationError.BufferTooSmall);
    expectEncodeErr(valid.encodePayload(tiny[0..]), PgNotificationError.BufferTooSmall);
});

test("pgwire notificationResponse: decodeFrame rejects invalid tag, invalid length, truncation and trailing bytes", () => {
    const valid = PgNotificationResponse.init(55, "ch1", "ok");
    let buf: [64]u8 = [0; 64];
    const n = valid.encodeFrame(buf[0..]) catch unreachable;

    // Truncated header (< 5 bytes)
    expectDecodeErr(PgNotificationResponse.decodeFrame(buf[0..4]), PgNotificationError.Truncated);

    // Truncated body
    expectDecodeErr(PgNotificationResponse.decodeFrame(buf[0 .. n - 1]), PgNotificationError.Truncated);

    // Trailing garbage byte after complete frame
    buf[n] = 0xAA;
    expectDecodeErr(PgNotificationResponse.decodeFrame(buf[0 .. n + 1]), PgNotificationError.TrailingBytes);

    // Wrong message tag
    let bad_tag: [32]u8 = [0; 32];
    let i: usize = 0;
    while (i < n) {
        bad_tag[i] = buf[i];
        i += 1;
    }
    bad_tag[0] = 'E';
    expectDecodeErr(PgNotificationResponse.decodeFrame(bad_tag[0..n]), PgNotificationError.InvalidTag);

    // Declared length too small (< 10)
    let bad_len: [12]u8 = ['A', 0, 0, 0, 8, 0, 0, 0, 1, 0, 0, 0];
    expectDecodeErr(PgNotificationResponse.decodeFrame(bad_len[0..9]), PgNotificationError.InvalidLength);
});

test("pgwire notificationResponse: decodePayload rejects missing NUL terminators, empty channel and trailing bytes", () => {
    // Missing second NUL terminator
    let no_pl_nul: [8]u8 = [0, 0, 0, 1, 'a', 0, 'b', 'c'];
    expectDecodeErr(PgNotificationResponse.decodePayload(no_pl_nul[0..]), PgNotificationError.MissingNulTerminator);

    // Missing first NUL terminator
    let no_ch_nul: [8]u8 = [0, 0, 0, 1, 'a', 'b', 'c', 'd'];
    expectDecodeErr(PgNotificationResponse.decodePayload(no_ch_nul[0..]), PgNotificationError.MissingNulTerminator);

    // Empty channel in wire payload
    let empty_ch: [6]u8 = [0, 0, 0, 1, 0, 0];
    expectDecodeErr(PgNotificationResponse.decodePayload(empty_ch[0..]), PgNotificationError.EmptyChannel);

    // Extra trailing bytes after second NUL
    let trailing: [9]u8 = [0, 0, 0, 1, 'a', 0, 'b', 0, 'X'];
    expectDecodeErr(PgNotificationResponse.decodePayload(trailing[0..]), PgNotificationError.TrailingBytes);
});

test("pgwire notificationResponse: edge i32 PIDs and equality comparison", () => {
    let buf: [64]u8 = [0; 64];

    const m_zero = PgNotificationResponse.init(0, "sys", "boot");
    const n0 = m_zero.encodeFrame(buf[0..]) catch unreachable;
    const d0 = PgNotificationResponse.decodeFrame(buf[0..n0]) catch unreachable;
    expect(d0.pid == 0 && d0.eql(m_zero));

    const m_max = PgNotificationResponse.init(2147483647, "sys", "max");
    const n1 = m_max.encodeFrame(buf[0..]) catch unreachable;
    const d1 = PgNotificationResponse.decodeFrame(buf[0..n1]) catch unreachable;
    expect(d1.pid == 2147483647 && d1.eql(m_max));
    expect(!d1.eql(d0));
});
