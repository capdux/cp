// Production deep test suite for cap:mysql ResultSetHeader, response classifier & AuthSwitchResponseCodec.

import { test, expect } from "cap:test";
import {
    MYSQL_PKT_OK,
    MYSQL_PKT_LOCAL_INFILE,
    MYSQL_PKT_EOF,
    MYSQL_PKT_ERR,
    MySqlResponsePacketKind,
    classifyQueryResponse,
    MySqlResultSetHeader,
    AuthSwitchResponseCodec,
} from "cap:mysql";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("mysql ResultSetHeader: 1-byte, 3-byte, 4-byte, and 9-byte lenenc-int roundtrip", fn() {
    let buf: [32]u8 = [0; 32];

    const counts: [4]u64 = [7, 300, 70000, 0x0001_0000_0005];
    const expected_payload_lens: [4]usize = [1, 3, 4, 9];

    let i: usize = 0;
    while (i < 4) {
        const hdr = MySqlResultSetHeader.init(counts[i]);
        expect(hdr.encodedPayloadLen() == expected_payload_lens[i]);
        expect(hdr.encodedFrameLen() == 4 + expected_payload_lens[i]);
        expect(hdr.expectedColumnDefPackets() == counts[i]);

        const n = hdr.encodeFrame(1, buf[0..]) catch unreachable;
        expect(n == hdr.encodedFrameLen());
        expect(buf[3] == 1);

        const decoded = MySqlResultSetHeader.decodeFrame(buf[0..n], false) catch unreachable;
        expect(decoded.eql(hdr));
        expect(decoded.field_count == counts[i]);
        expect(!decoded.has_metadata_flag);
        expect(decoded.metadata_follows);
        i += 1;
    }
});

test("mysql ResultSetHeader: MySQL 8.0+ CLIENT_OPTIONAL_RESULTSET_METADATA mode", fn() {
    let buf: [32]u8 = [0; 32];

    // 1. metadata_follows = true
    const with_meta = MySqlResultSetHeader.initWithMetadata(15, true);
    expect(with_meta.encodedPayloadLen() == 2);
    expect(with_meta.expectedColumnDefPackets() == 15);
    const n1 = with_meta.encodeFrame(2, buf[0..]) catch unreachable;
    const d1 = MySqlResultSetHeader.decodeFrame(buf[0..n1], true) catch unreachable;
    expect(d1.eql(with_meta));
    expect(d1.has_metadata_flag && d1.metadata_follows);

    // 2. metadata_follows = false (server skips ColumnDefinition41 packets)
    const no_meta = MySqlResultSetHeader.initWithMetadata(15, false);
    expect(no_meta.expectedColumnDefPackets() == 0);
    const n2 = no_meta.encodeFrame(2, buf[0..]) catch unreachable;
    const d2 = MySqlResultSetHeader.decodeFrame(buf[0..n2], true) catch unreachable;
    expect(d2.eql(no_meta));
    expect(d2.has_metadata_flag && !d2.metadata_follows);
    expect(d2.expectedColumnDefPackets() == 0);
});

test("mysql classifyQueryResponse: OK, ERR, EOF, LOCAL INFILE, and ResultSetHeader", fn() {
    const ok_pkt: [7]u8 = [MYSQL_PKT_OK, 0, 0, 2, 0, 0, 0];
    expect((classifyQueryResponse(ok_pkt[0..]) catch unreachable) == MySqlResponsePacketKind.Ok);

    const err_pkt: [3]u8 = [MYSQL_PKT_ERR, 0x15, 0x04];
    expect((classifyQueryResponse(err_pkt[0..]) catch unreachable) == MySqlResponsePacketKind.Error);

    const infile_pkt: [5]u8 = [MYSQL_PKT_LOCAL_INFILE, 'a', '.', 'c', 's'];
    expect((classifyQueryResponse(infile_pkt[0..]) catch unreachable) == MySqlResponsePacketKind.LocalInfile);

    const eof_pkt: [5]u8 = [MYSQL_PKT_EOF, 0, 0, 2, 0];
    expect((classifyQueryResponse(eof_pkt[0..]) catch unreachable) == MySqlResponsePacketKind.Eof);

    // 9-byte 0xFE lenenc-int is ResultSetHeader (not EOF which is < 9 bytes)
    const big_rs: [9]u8 = [MYSQL_PKT_EOF, 1, 0, 0, 0, 1, 0, 0, 0];
    expect((classifyQueryResponse(big_rs[0..]) catch unreachable) == MySqlResponsePacketKind.ResultSetHeader);

    const small_rs: [1]u8 = [5];
    expect((classifyQueryResponse(small_rs[0..]) catch unreachable) == MySqlResponsePacketKind.ResultSetHeader);

    // Short 0x00 (< 7 bytes) is invalid for both OK and ResultSetHeader
    const bad_zero: [2]u8 = [0, 0];
    let bad_ok = false;
    * = classifyQueryResponse(bad_zero[0..]) catch { bad_ok = true; };
    expect(bad_ok);

    // Empty payload rejected
    let empty_err = false;
    * = classifyQueryResponse(ok_pkt[0..0]) catch { empty_err = true; };
    expect(empty_err);
});

test("mysql AuthSwitchResponseCodec: scrambled and empty payload roundtrip", fn() {
    let buf: [64]u8 = [0; 64];

    const token = "12345678901234567890";
    expect(AuthSwitchResponseCodec.encodedFrameLen(token) == 24);
    const n1 = AuthSwitchResponseCodec.encodeFrame(token, 3, buf[0..]) catch unreachable;
    expect(n1 == 24);
    expect(buf[0] == 20 && buf[1] == 0 && buf[2] == 0 && buf[3] == 3);
    const d1 = AuthSwitchResponseCodec.decodeFrame(buf[0..n1]) catch unreachable;
    expect(strEql(d1, token));

    // Empty auth response (e.g. empty password)
    const n2 = AuthSwitchResponseCodec.encodeFrame("", 4, buf[0..]) catch unreachable;
    expect(n2 == 4);
    const d2 = AuthSwitchResponseCodec.decodeFrame(buf[0..n2]) catch unreachable;
    expect(d2.len == 0);
});

test("mysql ResultSetHeader: malformed payloads, zero columns, bad metadata flag, truncation guards", fn() {
    // 1. Zero column count rejected on encode & decode
    let out: [16]u8 = [0; 16];
    let e1 = false;
    * = MySqlResultSetHeader.init(0).encodePayload(out[0..]) catch { e1 = true; };
    expect(e1);

    const zero_p: [1]u8 = [0];
    let e2 = false;
    * = MySqlResultSetHeader.decodePayload(zero_p[0..], false) catch { e2 = true; };
    expect(e2);

    // 2. Invalid lenenc prefix (0xFB / 0xFF)
    const bad_prefix: [1]u8 = [0xFB];
    let e3 = false;
    * = MySqlResultSetHeader.decodePayload(bad_prefix[0..], false) catch { e3 = true; };
    expect(e3);

    // 3. Truncated 3-byte lenenc-int
    const trunc_lenenc: [2]u8 = [0xFC, 0x01];
    let e4 = false;
    * = MySqlResultSetHeader.decodePayload(trunc_lenenc[0..], false) catch { e4 = true; };
    expect(e4);

    // 4. Invalid metadata_follows flag (2 != 0|1)
    const bad_meta: [2]u8 = [2, 3];
    let e5 = false;
    * = MySqlResultSetHeader.decodePayload(bad_meta[0..], true) catch { e5 = true; };
    expect(e5);

    // 5. Trailing bytes after lenenc-int
    const trail_p: [2]u8 = [3, 0xFF];
    let e6 = false;
    * = MySqlResultSetHeader.decodePayload(trail_p[0..], false) catch { e6 = true; };
    expect(e6);
});

test("mysql ResultSetHeader & AuthSwitchResponseCodec: framing and buffer guards", fn() {
    let tiny: [3]u8 = [0; 3];

    let e1 = false;
    * = MySqlResultSetHeader.init(4).encodeFrame(1, tiny[0..]) catch { e1 = true; };
    expect(e1);

    let e2 = false;
    * = AuthSwitchResponseCodec.encodeFrame("abc", 1, tiny[0..]) catch { e2 = true; };
    expect(e2);

    // Truncated 4-byte MySQL packet header
    let e3 = false;
    * = MySqlResultSetHeader.decodeFrame(tiny[0..], false) catch { e3 = true; };
    expect(e3);

    let e4 = false;
    * = AuthSwitchResponseCodec.decodeFrame(tiny[0..]) catch { e4 = true; };
    expect(e4);

    // Declared payload length > available bytes
    const trunc_frame: [5]u8 = [5, 0, 0, 1, 'a'];
    let e5 = false;
    * = AuthSwitchResponseCodec.decodeFrame(trunc_frame[0..]) catch { e5 = true; };
    expect(e5);

    // Trailing bytes after declared packet length
    const trail_frame: [6]u8 = [1, 0, 0, 1, 'a', 'b'];
    let e6 = false;
    * = AuthSwitchResponseCodec.decodeFrame(trail_frame[0..]) catch { e6 = true; };
    expect(e6);
});
