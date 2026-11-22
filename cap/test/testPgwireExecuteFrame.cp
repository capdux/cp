import { test, expect } from "cap:test";
import {
    PG_TAG_EXECUTE,
    PG_TAG_SYNC,
    PG_TAG_FLUSH,
    PG_EXECUTE_MIN_FRAME_LEN,
    PG_EXECUTE_UNLIMITED_ROWS,
    PgExecuteFrameError,
    PgExecuteFrame,
} from "cap:pgwire";

test("unnamed Execute ('E') frame roundtrip with unlimited rows", () => {
    const req = PgExecuteFrame.unnamedAll();
    expect(req.isUnnamed()).toBe(true);
    expect(req.isUnlimited()).toBe(true);
    expect(req.payloadLen()).toBe(5);
    expect(req.frameLen()).toBe(PG_EXECUTE_MIN_FRAME_LEN);

    let buf: [32]u8 = [0; 32];
    const n = req.encodeFrame(buf[0..])?;
    expect(n).toBe(10);
    expect(buf[0]).toBe(PG_TAG_EXECUTE);
    // msg_len = 9 (4 len + 1 NUL + 4 max_rows)
    expect(buf[1]).toBe(0);
    expect(buf[2]).toBe(0);
    expect(buf[3]).toBe(0);
    expect(buf[4]).toBe(9);
    // empty portal NUL
    expect(buf[5]).toBe(0);
    // max_rows = 0
    expect(buf[6]).toBe(0);
    expect(buf[7]).toBe(0);
    expect(buf[8]).toBe(0);
    expect(buf[9]).toBe(0);

    const decoded = PgExecuteFrame.decodeFrame(buf[0..n])?;
    expect(decoded.eql(req)).toBe(true);
    expect(decoded.isUnnamed()).toBe(true);
    expect(decoded.max_rows).toBe(PG_EXECUTE_UNLIMITED_ROWS);
});

test("named portal Execute with cursor pagination max_rows", () => {
    const req = PgExecuteFrame.named("portal_42", 0x01020304);
    expect(req.isUnnamed()).toBe(false);
    expect(req.isUnlimited()).toBe(false);
    expect(req.payloadLen()).toBe(14);
    expect(req.frameLen()).toBe(19);

    let buf: [64]u8 = [0; 64];
    const n = req.encodeFrame(buf[0..])?;
    expect(n).toBe(19);
    expect(buf[14]).toBe(0); // NUL after "portal_42"
    expect(buf[15]).toBe(0x01);
    expect(buf[16]).toBe(0x02);
    expect(buf[17]).toBe(0x03);
    expect(buf[18]).toBe(0x04);

    const decoded = PgExecuteFrame.decodeFrame(buf[0..n])?;
    expect(decoded.eql(req)).toBe(true);
    expect(decoded.portal).toBe("portal_42");
    expect(decoded.max_rows).toBe(0x01020304);

    const req2 = PgExecuteFrame.namedAll("portal_42");
    expect(req2.eql(req)).toBe(false);
    expect(req2.isUnlimited()).toBe(true);
});

test("single-buffer Execute+Sync and Execute+Flush pipelining", () => {
    const req = PgExecuteFrame.unnamed(250);
    let buf: [32]u8 = [0; 32];

    const n_sync = req.encodeExecuteSync(buf[0..])?;
    expect(n_sync).toBe(15);
    expect(buf[10]).toBe(PG_TAG_SYNC);
    expect(buf[14]).toBe(4);
    const dec1 = PgExecuteFrame.decodeFrame(buf[0..10])?;
    expect(dec1.max_rows).toBe(250);

    const n_flush = req.encodeExecuteFlush(buf[0..])?;
    expect(n_flush).toBe(15);
    expect(buf[10]).toBe(PG_TAG_FLUSH);
    expect(buf[14]).toBe(4);
    const dec2 = PgExecuteFrame.decodeFrame(buf[0..10])?;
    expect(dec2.eql(req)).toBe(true);
});

test("encode validation guards: NegativeMaxRows, EmbeddedNul, BufferTooSmall", () => {
    let buf: [32]u8 = [0; 32];

    // NegativeMaxRows
    const neg = PgExecuteFrame.unnamed(-1);
    let got_neg = false;
    * = neg.encodeFrame(buf[0..]) catch |e| {
        got_neg = (e == PgExecuteFrameError.NegativeMaxRows);
        0
    };
    expect(got_neg).toBe(true);

    // EmbeddedNul in portal name
    const bad_portal: [4]u8 = ['a', 0, 'b', 'c'];
    const emb = PgExecuteFrame.named(bad_portal[0..], 10);
    let got_nul = false;
    * = emb.encodeFrame(buf[0..]) catch |e| {
        got_nul = (e == PgExecuteFrameError.EmbeddedNul);
        0
    };
    expect(got_nul).toBe(true);

    // BufferTooSmall
    let tiny: [6]u8 = [0; 6];
    let got_small = false;
    * = PgExecuteFrame.unnamedAll().encodeFrame(tiny[0..]) catch |e| {
        got_small = (e == PgExecuteFrameError.BufferTooSmall);
        0
    };
    expect(got_small).toBe(true);

    let sync_tiny: [12]u8 = [0; 12];
    let got_sync_small = false;
    * = PgExecuteFrame.unnamedAll().encodeExecuteSync(sync_tiny[0..]) catch |e| {
        got_sync_small = (e == PgExecuteFrameError.BufferTooSmall);
        0
    };
    expect(got_sync_small).toBe(true);
});

test("decode error guards: InvalidTag, InvalidLength, Truncated, MissingNul, TrailingBytes, NegativeMaxRows", () => {
    // InvalidTag
    const bad_tag: [10]u8 = ['P', 0, 0, 0, 9, 0, 0, 0, 0, 0];
    let got_tag = false;
    * = PgExecuteFrame.decodeFrame(bad_tag[0..]) catch |e| {
        got_tag = (e == PgExecuteFrameError.InvalidTag);
        PgExecuteFrame{}
    };
    expect(got_tag).toBe(true);

    // InvalidLength (< 9)
    const bad_len: [10]u8 = ['E', 0, 0, 0, 8, 0, 0, 0, 0, 0];
    let got_len = false;
    * = PgExecuteFrame.decodeFrame(bad_len[0..]) catch |e| {
        got_len = (e == PgExecuteFrameError.InvalidLength);
        PgExecuteFrame{}
    };
    expect(got_len).toBe(true);

    // Truncated (< 10 bytes)
    const trunc: [6]u8 = ['E', 0, 0, 0, 9, 0];
    let got_trunc = false;
    * = PgExecuteFrame.decodeFrame(trunc[0..]) catch |e| {
        got_trunc = (e == PgExecuteFrameError.Truncated);
        PgExecuteFrame{}
    };
    expect(got_trunc).toBe(true);

    // MissingNulTerminator
    const no_nul: [10]u8 = ['E', 0, 0, 0, 9, 'a', 'b', 'c', 'd', 'e'];
    let got_no_nul = false;
    * = PgExecuteFrame.decodeFrame(no_nul[0..]) catch |e| {
        got_no_nul = (e == PgExecuteFrameError.MissingNulTerminator);
        PgExecuteFrame{}
    };
    expect(got_no_nul).toBe(true);

    // TrailingBytes
    const trailing: [11]u8 = ['E', 0, 0, 0, 9, 0, 0, 0, 0, 1, 0xFF];
    let got_trail = false;
    * = PgExecuteFrame.decodeFrame(trailing[0..]) catch |e| {
        got_trail = (e == PgExecuteFrameError.TrailingBytes);
        PgExecuteFrame{}
    };
    expect(got_trail).toBe(true);

    // NegativeMaxRows on wire (0xFFFFFFFF = -1)
    const wire_neg: [10]u8 = ['E', 0, 0, 0, 9, 0, 0xFF, 0xFF, 0xFF, 0xFF];
    let got_wire_neg = false;
    * = PgExecuteFrame.decodeFrame(wire_neg[0..]) catch |e| {
        got_wire_neg = (e == PgExecuteFrameError.NegativeMaxRows);
        PgExecuteFrame{}
    };
    expect(got_wire_neg).toBe(true);
});
