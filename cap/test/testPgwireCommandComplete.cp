import { test, expect } from "cap:test";
import {
    PG_TAG_COMMAND_COMPLETE,
    PG_COMMAND_COMPLETE_MIN_FRAME_LEN,
    PgCommandKind,
    PgCommandTag,
    PgCommandCompleteError,
    PgCommandCompleteFrame,
} from "cap:pgwire";

test("CommandComplete ('C') INSERT / SELECT frame roundtrip & rowCount extraction", () => {
    const f = PgCommandCompleteFrame.init("INSERT 0 42");
    expect(f.payloadLen()).toBe(12);
    expect(f.frameLen()).toBe(17);

    let buf: [64]u8 = [0; 64];
    const n = f.encodeFrame(buf[0..])?;
    expect(n).toBe(17);
    expect(buf[0]).toBe(PG_TAG_COMMAND_COMPLETE);
    // msg_len = 4 + 12 = 16
    expect(buf[4]).toBe(16);
    expect(buf[16]).toBe(0);

    const decoded = PgCommandCompleteFrame.decodeFrame(buf[0..n])?;
    expect(decoded.eql(f)).toBe(true);
    expect(decoded.raw_tag).toBe("INSERT 0 42");
    expect(decoded.kind() == PgCommandKind.Insert).toBe(true);
    expect(decoded.isDml()).toBe(true);
    expect(decoded.isQueryOrCursor()).toBe(false);
    expect(decoded.isTransactionControl()).toBe(false);
    if (rows := decoded.rowCount()) {
        expect(rows).toBe(42);
    } else {
        expect(false).toBe(true);
    }

    const sel = PgCommandCompleteFrame.init("SELECT 128");
    const n2 = sel.encodeFrame(buf[0..])?;
    const d2 = PgCommandCompleteFrame.decodeFrame(buf[0..n2])?;
    expect(d2.kind() == PgCommandKind.Select).toBe(true);
    expect(d2.isDml()).toBe(false);
    expect(d2.isQueryOrCursor()).toBe(true);
    if (r2 := d2.rowCount()) {
        expect(r2).toBe(128);
    } else {
        expect(false).toBe(true);
    }
});

test("encodeFromTag direct structured serialization & transaction control predicates", () => {
    let buf: [64]u8 = [0; 64];
    const tag = PgCommandTag.parse("UPDATE 999");
    const n = PgCommandCompleteFrame.encodeFromTag(tag, buf[0..])?;
    const decoded = PgCommandCompleteFrame.decodeFrame(buf[0..n])?;
    expect(decoded.raw_tag).toBe("UPDATE 999");
    expect(decoded.isDml()).toBe(true);
    if (r := decoded.rowCount()) {
        expect(r).toBe(999);
    } else {
        expect(false).toBe(true);
    }

    expect(PgCommandCompleteFrame.init("BEGIN").isTransactionControl()).toBe(true);
    expect(PgCommandCompleteFrame.init("COMMIT").isTransactionControl()).toBe(true);
    expect(PgCommandCompleteFrame.init("ROLLBACK").isTransactionControl()).toBe(true);
    expect(PgCommandCompleteFrame.init("SAVEPOINT").isTransactionControl()).toBe(true);
    expect(PgCommandCompleteFrame.init("RELEASE").isTransactionControl()).toBe(true);
    expect(PgCommandCompleteFrame.init("PREPARE TRANSACTION").isTransactionControl()).toBe(true);
    expect(PgCommandCompleteFrame.init("CREATE TABLE").isTransactionControl()).toBe(false);
    expect(PgCommandCompleteFrame.init("CREATE TABLE").rowCount() == null).toBe(true);
});

test("empty command tag minimal frame (6 bytes)", () => {
    const empty = PgCommandCompleteFrame.init("");
    expect(empty.frameLen()).toBe(PG_COMMAND_COMPLETE_MIN_FRAME_LEN);

    let buf: [16]u8 = [0; 16];
    const n = empty.encodeFrame(buf[0..])?;
    expect(n).toBe(6);
    expect(buf[0]).toBe(PG_TAG_COMMAND_COMPLETE);
    expect(buf[4]).toBe(5);
    expect(buf[5]).toBe(0);

    const decoded = PgCommandCompleteFrame.decodeFrame(buf[0..n])?;
    expect(decoded.raw_tag.len).toBe(0);
    expect(decoded.kind() == PgCommandKind.Other).toBe(true);
});

test("encode guards: EmbeddedNul & BufferTooSmall", () => {
    let buf: [32]u8 = [0; 32];
    const bad: [5]u8 = ['B', 'E', 0, 'G', 'I'];
    let got_nul = false;
    * = PgCommandCompleteFrame.init(bad[0..]).encodeFrame(buf[0..]) catch |e| {
        got_nul = (e == PgCommandCompleteError.EmbeddedNul);
        0
    };
    expect(got_nul).toBe(true);

    let tiny: [5]u8 = [0; 5];
    let got_small = false;
    * = PgCommandCompleteFrame.init("COMMIT").encodeFrame(tiny[0..]) catch |e| {
        got_small = (e == PgCommandCompleteError.BufferTooSmall);
        0
    };
    expect(got_small).toBe(true);

    let got_tag_small = false;
    * = PgCommandCompleteFrame.encodeFromTag(PgCommandTag.parse("SELECT 1"), tiny[0..]) catch |e| {
        got_tag_small = (e == PgCommandCompleteError.BufferTooSmall);
        0
    };
    expect(got_tag_small).toBe(true);
});

test("decode guards: InvalidTag, InvalidLength, Truncated, MissingNulTerminator, TrailingBytes", () => {
    // InvalidTag
    const bad_tag: [6]u8 = ['S', 0, 0, 0, 5, 0];
    let got_tag = false;
    * = PgCommandCompleteFrame.decodeFrame(bad_tag[0..]) catch |e| {
        got_tag = (e == PgCommandCompleteError.InvalidTag);
        PgCommandCompleteFrame{}
    };
    expect(got_tag).toBe(true);

    // InvalidLength (< 5)
    const bad_len: [6]u8 = ['C', 0, 0, 0, 4, 0];
    let got_len = false;
    * = PgCommandCompleteFrame.decodeFrame(bad_len[0..]) catch |e| {
        got_len = (e == PgCommandCompleteError.InvalidLength);
        PgCommandCompleteFrame{}
    };
    expect(got_len).toBe(true);

    // Truncated (< 6 bytes)
    const trunc: [5]u8 = ['C', 0, 0, 0, 5];
    let got_trunc = false;
    * = PgCommandCompleteFrame.decodeFrame(trunc[0..]) catch |e| {
        got_trunc = (e == PgCommandCompleteError.Truncated);
        PgCommandCompleteFrame{}
    };
    expect(got_trunc).toBe(true);

    // MissingNulTerminator
    const no_nul: [8]u8 = ['C', 0, 0, 0, 7, 'O', 'K', '!'];
    let got_no_nul = false;
    * = PgCommandCompleteFrame.decodeFrame(no_nul[0..]) catch |e| {
        got_no_nul = (e == PgCommandCompleteError.MissingNulTerminator);
        PgCommandCompleteFrame{}
    };
    expect(got_no_nul).toBe(true);

    // TrailingBytes inside payload after NUL
    const inner_trail: [8]u8 = ['C', 0, 0, 0, 7, 'O', 0, 'X'];
    let got_trail1 = false;
    * = PgCommandCompleteFrame.decodeFrame(inner_trail[0..]) catch |e| {
        got_trail1 = (e == PgCommandCompleteError.TrailingBytes);
        PgCommandCompleteFrame{}
    };
    expect(got_trail1).toBe(true);

    // TrailingBytes beyond frame length
    const outer_trail: [7]u8 = ['C', 0, 0, 0, 5, 0, 0xFF];
    let got_trail2 = false;
    * = PgCommandCompleteFrame.decodeFrame(outer_trail[0..]) catch |e| {
        got_trail2 = (e == PgCommandCompleteError.TrailingBytes);
        PgCommandCompleteFrame{}
    };
    expect(got_trail2).toBe(true);
});
