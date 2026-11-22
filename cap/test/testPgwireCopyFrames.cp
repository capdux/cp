// Deep production regression tests for cap:pgwire copyFrames.cp
// Covers CopyData ('d') binary & empty chunks, CopyDone ('c') 5-byte frame,
// CopyFail ('f') abort message round-trip, and strict malformed frame guards.

import { test, expect, expect_eq } from "cap:test";
import {
    PG_TAG_COPY_DATA,
    PG_TAG_COPY_DONE,
    PG_TAG_COPY_FAIL,
    PG_COPY_DONE_FRAME_LEN,
    PgCopyError,
    PgCopyData,
    PgCopyDone,
    PgCopyFail,
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

fn expectUsizeErr(res: PgCopyError!usize, want: PgCopyError) {
    let got_err = false;
    * = res catch |e| cb: {
        expect_eq(e, want);
        got_err = true;
        break :cb 0;
    };
    expect(got_err);
}

fn expectBoolErr(res: PgCopyError!bool, want: PgCopyError) {
    let got_err = false;
    * = res catch |e| cb: {
        expect_eq(e, want);
        got_err = true;
        break :cb false;
    };
    expect(got_err);
}

fn expectDataErr(res: PgCopyError!PgCopyData, want: PgCopyError) {
    let got_err = false;
    * = res catch |e| cb: {
        expect_eq(e, want);
        got_err = true;
        break :cb PgCopyData{};
    };
    expect(got_err);
}

fn expectFailErr(res: PgCopyError!PgCopyFail, want: PgCopyError) {
    let got_err = false;
    * = res catch |e| cb: {
        expect_eq(e, want);
        got_err = true;
        break :cb PgCopyFail{};
    };
    expect(got_err);
}

test("pgwire copyFrames: CopyData ('d') round-trip with binary NUL bytes and empty chunk", () => {
    expect(PG_TAG_COPY_DATA == 'd');
    expect(PG_TAG_COPY_DONE == 'c');
    expect(PG_TAG_COPY_FAIL == 'f');
    expect(PG_COPY_DONE_FRAME_LEN == 5);

    // Binary COPY chunk containing embedded 0x00 bytes
    let bin_chunk: [6]u8 = [0x50, 0x47, 0x43, 0x00, 0xFF, 0x01];
    const cd = PgCopyData.init(bin_chunk[0..]);
    expect(!cd.isEmpty());
    expect(cd.encodedFrameLen() == 11);

    let buf: [32]u8 = [0; 32];
    const n = cd.encodeFrame(buf[0..]) catch unreachable;
    expect(n == 11);
    expect(buf[0] == 'd' && buf[1] == 0 && buf[2] == 0 && buf[3] == 0 && buf[4] == 10);

    const decoded = PgCopyData.decodeFrame(buf[0..n]) catch unreachable;
    expect(decoded.eql(cd));
    expect(decoded.data.len == 6 && decoded.data[3] == 0x00 && decoded.data[4] == 0xFF);

    // Empty CopyData chunk (wire length == 4)
    const empty_cd = PgCopyData.init("");
    expect(empty_cd.isEmpty());
    const n_empty = empty_cd.encodeFrame(buf[0..]) catch unreachable;
    expect(n_empty == 5);
    const dec_empty = PgCopyData.decodeFrame(buf[0..n_empty]) catch unreachable;
    expect(dec_empty.isEmpty() && dec_empty.eql(empty_cd));
});

test("pgwire copyFrames: CopyDone ('c') 5-byte frame codec and guards", () => {
    let buf: [8]u8 = [0; 8];
    const n = PgCopyDone.encodeFrame(buf[0..]) catch unreachable;
    expect(n == 5);
    expect(buf[0] == 'c' && buf[1] == 0 && buf[2] == 0 && buf[3] == 0 && buf[4] == 4);
    expect(PgCopyDone.decodeFrame(buf[0..5]) catch unreachable);

    let tiny: [4]u8 = [0; 4];
    expectUsizeErr(PgCopyDone.encodeFrame(tiny[0..]), PgCopyError.BufferTooSmall);
    expectBoolErr(PgCopyDone.decodeFrame(buf[0..4]), PgCopyError.Truncated);

    buf[5] = 0x01;
    expectBoolErr(PgCopyDone.decodeFrame(buf[0..6]), PgCopyError.TrailingBytes);

    let bad_tag: [5]u8 = ['d', 0, 0, 0, 4];
    expectBoolErr(PgCopyDone.decodeFrame(bad_tag[0..]), PgCopyError.InvalidTag);

    let bad_len: [5]u8 = ['c', 0, 0, 0, 5];
    expectBoolErr(PgCopyDone.decodeFrame(bad_len[0..]), PgCopyError.InvalidLength);
});

test("pgwire copyFrames: CopyFail ('f') frame and payload round-trip including empty reason", () => {
    let buf: [64]u8 = [0; 64];

    const cf = PgCopyFail.init("client aborted: malformed CSV row 42");
    expect(cf.encodedPayloadLen() == 37);
    expect(cf.encodedFrameLen() == 42);

    const n = cf.encodeFrame(buf[0..]) catch unreachable;
    expect(n == 42);
    expect(buf[0] == 'f' && buf[4] == 41 && buf[41] == 0);

    const decoded = PgCopyFail.decodeFrame(buf[0..n]) catch unreachable;
    expect(decoded.eql(cf));
    expect(strEq(decoded.message, "client aborted: malformed CSV row 42"));

    // Empty error reason is valid in PostgreSQL wire protocol (payload = [0])
    const empty_cf = PgCopyFail.init("");
    const n0 = empty_cf.encodeFrame(buf[0..]) catch unreachable;
    expect(n0 == 6 && buf[0] == 'f' && buf[4] == 5 && buf[5] == 0);
    const dec0 = PgCopyFail.decodeFrame(buf[0..n0]) catch unreachable;
    expect(dec0.eql(empty_cf));
});

test("pgwire copyFrames: CopyFail encode guards reject embedded NUL and small buffers", () => {
    let buf: [32]u8 = [0; 32];
    let bad_msg: [5]u8 = ['e', 'r', 0, 'r', '!'];
    const nul_cf = PgCopyFail.init(bad_msg[0..]);
    expectUsizeErr(nul_cf.encodeFrame(buf[0..]), PgCopyError.EmbeddedNul);
    expectUsizeErr(nul_cf.encodePayload(buf[0..]), PgCopyError.EmbeddedNul);

    const ok = PgCopyFail.init("abort copy");
    let tiny: [8]u8 = [0; 8];
    expectUsizeErr(ok.encodeFrame(tiny[0..]), PgCopyError.BufferTooSmall);
    expectUsizeErr(ok.encodePayload(tiny[0..]), PgCopyError.BufferTooSmall);
});

test("pgwire copyFrames: CopyData decode guards (tag, length, truncation, trailing bytes)", () => {
    let buf: [16]u8 = [0; 16];
    const cd = PgCopyData.init("row1\n");
    const n = cd.encodeFrame(buf[0..]) catch unreachable;

    expectDataErr(PgCopyData.decodeFrame(buf[0..4]), PgCopyError.Truncated);
    expectDataErr(PgCopyData.decodeFrame(buf[0 .. n - 1]), PgCopyError.Truncated);

    buf[n] = 0xEE;
    expectDataErr(PgCopyData.decodeFrame(buf[0 .. n + 1]), PgCopyError.TrailingBytes);

    let wrong_tag: [6]u8 = ['c', 0, 0, 0, 5, 'a'];
    expectDataErr(PgCopyData.decodeFrame(wrong_tag[0..]), PgCopyError.InvalidTag);

    let bad_len: [5]u8 = ['d', 0, 0, 0, 3];
    expectDataErr(PgCopyData.decodeFrame(bad_len[0..]), PgCopyError.InvalidLength);

    let tiny: [6]u8 = [0; 6];
    expectUsizeErr(cd.encodeFrame(tiny[0..]), PgCopyError.BufferTooSmall);
});

test("pgwire copyFrames: CopyFail decode guards (tag, length, missing NUL, trailing bytes)", () => {
    let buf: [32]u8 = [0; 32];
    const cf = PgCopyFail.init("fail");
    const n = cf.encodeFrame(buf[0..]) catch unreachable;

    expectFailErr(PgCopyFail.decodeFrame(buf[0..4]), PgCopyError.Truncated);
    expectFailErr(PgCopyFail.decodeFrame(buf[0 .. n - 1]), PgCopyError.Truncated);

    buf[n] = 0xAA;
    expectFailErr(PgCopyFail.decodeFrame(buf[0 .. n + 1]), PgCopyError.TrailingBytes);

    let wrong_tag: [6]u8 = ['d', 0, 0, 0, 5, 0];
    expectFailErr(PgCopyFail.decodeFrame(wrong_tag[0..]), PgCopyError.InvalidTag);

    let bad_len: [5]u8 = ['f', 0, 0, 0, 4];
    expectFailErr(PgCopyFail.decodeFrame(bad_len[0..]), PgCopyError.InvalidLength);

    // Payload without NUL terminator or with trailing bytes
    let no_nul: [4]u8 = ['a', 'b', 'c', 'd'];
    expectFailErr(PgCopyFail.decodePayload(no_nul[0..]), PgCopyError.MissingNulTerminator);

    let trail_p: [4]u8 = ['a', 0, 'b', 0];
    expectFailErr(PgCopyFail.decodePayload(trail_p[0..]), PgCopyError.TrailingBytes);

    let empty_p: [0]u8 = [];
    expectFailErr(PgCopyFail.decodePayload(empty_p[0..]), PgCopyError.Truncated);
});
