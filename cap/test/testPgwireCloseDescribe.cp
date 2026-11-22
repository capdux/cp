// Deep production regression tests for cap:pgwire closeDescribe.cp
// Covers Close ('C'), Describe ('D'), CloseComplete ('3'), named & unnamed
// prepared statements ('S') and portals ('P'), plus malformed frame guards.

import { test, expect, expect_eq } from "cap:test";
import {
    PG_TAG_CLOSE,
    PG_TAG_DESCRIBE,
    PG_TAG_CLOSE_COMPLETE,
    PG_TARGET_PREPARED_STATEMENT,
    PG_TARGET_PORTAL,
    PgCloseDescribeError,
    PgTargetKind,
    PgTargetRef,
    PgCloseMsg,
    PgDescribeMsg,
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

fn expectUsizeErr(res: PgCloseDescribeError!usize, want: PgCloseDescribeError) {
    let got_err = false;
    * = res catch |e| cb: {
        expect_eq(e, want);
        got_err = true;
        break :cb 0;
    };
    expect(got_err);
}

fn expectCloseErr(res: PgCloseDescribeError!PgCloseMsg, want: PgCloseDescribeError) {
    let got_err = false;
    * = res catch |e| cb: {
        expect_eq(e, want);
        got_err = true;
        break :cb PgCloseMsg{};
    };
    expect(got_err);
}

fn expectDescribeErr(res: PgCloseDescribeError!PgDescribeMsg, want: PgCloseDescribeError) {
    let got_err = false;
    * = res catch |e| cb: {
        expect_eq(e, want);
        got_err = true;
        break :cb PgDescribeMsg{};
    };
    expect(got_err);
}

fn expectBoolErr(res: PgCloseDescribeError!bool, want: PgCloseDescribeError) {
    let got_err = false;
    * = res catch |e| cb: {
        expect_eq(e, want);
        got_err = true;
        break :cb false;
    };
    expect(got_err);
}

test("pgwire closeDescribe: Close ('C') prepared statement and portal frame round-trip", () => {
    expect(PG_TAG_CLOSE == 'C');
    expect(PG_TAG_DESCRIBE == 'D');
    expect(PG_TAG_CLOSE_COMPLETE == '3');
    expect(PG_TARGET_PREPARED_STATEMENT == 'S');
    expect(PG_TARGET_PORTAL == 'P');

    const msg = PgCloseMsg.statement("stmt_42");
    expect(msg.target.kind == .PreparedStatement);
    expect(msg.target.tag() == 'S');
    expect(!msg.target.isUnnamed());
    expect(msg.encodedPayloadLen() == 9);
    expect(msg.encodedFrameLen() == 14);

    let buf: [64]u8 = [0; 64];
    const n = msg.encodeFrame(buf[0..]) catch unreachable;
    expect(n == 14);
    // 'C' + length (4 + 9 = 13) + 'S' + "stmt_42\0"
    expect(buf[0] == 'C');
    expect(buf[1] == 0 && buf[2] == 0 && buf[3] == 0 && buf[4] == 13);
    expect(buf[5] == 'S');
    expect(buf[13] == 0);

    const decoded = PgCloseMsg.decodeFrame(buf[0..n]) catch unreachable;
    expect(decoded.eql(msg));
    expect(strEq(decoded.target.name, "stmt_42"));

    // Portal close
    const pmsg = PgCloseMsg.portal("cur_orders");
    const pn = pmsg.encodeFrame(buf[0..]) catch unreachable;
    const pdec = PgCloseMsg.decodeFrame(buf[0..pn]) catch unreachable;
    expect(pdec.eql(pmsg));
    expect(pdec.target.kind == .Portal);
    expect(strEq(pdec.target.kind.asStr(), "portal"));
});

test("pgwire closeDescribe: Describe ('D') named and unnamed targets", () => {
    let buf: [64]u8 = [0; 64];

    const d_stmt = PgDescribeMsg.statement("q_select_user");
    const n1 = d_stmt.encodeFrame(buf[0..]) catch unreachable;
    expect(buf[0] == 'D' && buf[5] == 'S');
    const dec1 = PgDescribeMsg.decodeFrame(buf[0..n1]) catch unreachable;
    expect(dec1.eql(d_stmt));

    // Unnamed portal (empty name -> 2-byte payload: ['P', 0])
    const unnamed = PgDescribeMsg.init(PgTargetRef.unnamedPortal());
    expect(unnamed.target.isUnnamed());
    expect(unnamed.encodedPayloadLen() == 2);
    expect(unnamed.encodedFrameLen() == 7);

    const n2 = unnamed.encodeFrame(buf[0..]) catch unreachable;
    expect(n2 == 7);
    expect(buf[0] == 'D' && buf[4] == 6 && buf[5] == 'P' && buf[6] == 0);

    const dec2 = PgDescribeMsg.decodeFrame(buf[0..n2]) catch unreachable;
    expect(dec2.eql(unnamed));
    expect(dec2.target.isUnnamed());
});

test("pgwire closeDescribe: CloseComplete ('3') 5-byte frame codec and guards", () => {
    let buf: [8]u8 = [0; 8];
    const n = PgCloseMsg.encodeCloseCompleteFrame(buf[0..]) catch unreachable;
    expect(n == 5);
    expect(buf[0] == '3' && buf[1] == 0 && buf[2] == 0 && buf[3] == 0 && buf[4] == 4);
    expect(PgCloseMsg.decodeCloseCompleteFrame(buf[0..5]) catch unreachable);

    // Guards: small buffer, truncated, wrong tag, wrong length, trailing bytes
    let tiny: [4]u8 = [0; 4];
    expectUsizeErr(PgCloseMsg.encodeCloseCompleteFrame(tiny[0..]), PgCloseDescribeError.BufferTooSmall);
    expectBoolErr(PgCloseMsg.decodeCloseCompleteFrame(buf[0..4]), PgCloseDescribeError.Truncated);

    buf[5] = 0xFF;
    expectBoolErr(PgCloseMsg.decodeCloseCompleteFrame(buf[0..6]), PgCloseDescribeError.TrailingBytes);

    let bad_tag: [5]u8 = ['2', 0, 0, 0, 4];
    expectBoolErr(PgCloseMsg.decodeCloseCompleteFrame(bad_tag[0..]), PgCloseDescribeError.InvalidTag);

    let bad_len: [5]u8 = ['3', 0, 0, 0, 5];
    expectBoolErr(PgCloseMsg.decodeCloseCompleteFrame(bad_len[0..]), PgCloseDescribeError.InvalidLength);
});

test("pgwire closeDescribe: encode guards reject embedded NUL and undersized buffers", () => {
    let bad_name: [4]u8 = ['s', 0, '1', '2'];
    let buf: [32]u8 = [0; 32];

    const c_nul = PgCloseMsg.statement(bad_name[0..]);
    expectUsizeErr(c_nul.encodeFrame(buf[0..]), PgCloseDescribeError.EmbeddedNul);
    expectUsizeErr(c_nul.encodePayload(buf[0..]), PgCloseDescribeError.EmbeddedNul);

    const d_nul = PgDescribeMsg.portal(bad_name[0..]);
    expectUsizeErr(d_nul.encodeFrame(buf[0..]), PgCloseDescribeError.EmbeddedNul);

    const ok = PgCloseMsg.statement("stmt_long_name");
    let tiny: [8]u8 = [0; 8];
    expectUsizeErr(ok.encodeFrame(tiny[0..]), PgCloseDescribeError.BufferTooSmall);
    expectUsizeErr(ok.encodePayload(tiny[0..]), PgCloseDescribeError.BufferTooSmall);
});

test("pgwire closeDescribe: decode guards reject invalid tag, target kind, length, missing NUL and trailing bytes", () => {
    let buf: [32]u8 = [0; 32];
    const msg = PgCloseMsg.statement("s1");
    const n = msg.encodeFrame(buf[0..]) catch unreachable;

    // Cross-tag decode ('C' decoded as Describe 'D') -> InvalidTag
    expectDescribeErr(PgDescribeMsg.decodeFrame(buf[0..n]), PgCloseDescribeError.InvalidTag);

    // Truncated header & body
    expectCloseErr(PgCloseMsg.decodeFrame(buf[0..4]), PgCloseDescribeError.Truncated);
    expectCloseErr(PgCloseMsg.decodeFrame(buf[0 .. n - 1]), PgCloseDescribeError.Truncated);

    // Trailing byte
    buf[n] = 0xAA;
    expectCloseErr(PgCloseMsg.decodeFrame(buf[0 .. n + 1]), PgCloseDescribeError.TrailingBytes);

    // Invalid wire length (< 6)
    let bad_len: [6]u8 = ['C', 0, 0, 0, 5, 'S'];
    expectCloseErr(PgCloseMsg.decodeFrame(bad_len[0..]), PgCloseDescribeError.InvalidLength);

    // Invalid target kind ('X' instead of 'S'/'P')
    let bad_kind: [8]u8 = ['C', 0, 0, 0, 7, 'X', 'a', 0];
    expectCloseErr(PgCloseMsg.decodeFrame(bad_kind[0..]), PgCloseDescribeError.InvalidTargetKind);

    // Missing NUL terminator in payload
    let no_nul: [3]u8 = ['S', 'a', 'b'];
    expectCloseErr(PgCloseMsg.decodePayload(no_nul[0..]), PgCloseDescribeError.MissingNulTerminator);

    // Trailing bytes after NUL in payload
    let trail_p: [4]u8 = ['P', 'a', 0, 'z'];
    expectDescribeErr(PgDescribeMsg.decodePayload(trail_p[0..]), PgCloseDescribeError.TrailingBytes);
});

test("pgwire closeDescribe: PgTargetKind.fromTag and unnamed statement round-trip", () => {
    expect(PgTargetKind.fromTag('S').? == .PreparedStatement);
    expect(PgTargetKind.fromTag('P').? == .Portal);
    expect(PgTargetKind.fromTag('Q') == null);
    expect(strEq(PgTargetKind.PreparedStatement.asStr(), "prepared_statement"));

    let pbuf: [8]u8 = [0; 8];
    const u_stmt = PgCloseMsg.init(PgTargetRef.unnamedStatement());
    const pn = u_stmt.encodePayload(pbuf[0..]) catch unreachable;
    expect(pn == 2 && pbuf[0] == 'S' && pbuf[1] == 0);

    const decoded = PgCloseMsg.decodePayload(pbuf[0..pn]) catch unreachable;
    expect(decoded.eql(u_stmt));
    expect(decoded.target.isUnnamed());
});
