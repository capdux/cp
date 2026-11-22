import { test, expect } from "cap:test";
import {
    PG_TAG_COPY_IN_RESPONSE,
    PG_TAG_COPY_OUT_RESPONSE,
    PG_TAG_COPY_BOTH_RESPONSE,
    PG_COPY_RESP_HEADER_LEN,
    PG_COPY_FORMAT_TEXT,
    PG_COPY_FORMAT_BINARY,
    PG_COPY_RESP_MAX_COLS,
    PgCopyResponseError,
    PgCopyDirection,
    PgCopyFormat,
    PgCopyResponseView,
    PgCopyResponseEncoder,
} from "cap:pgwire";

test("CopyInResponse ('G') text-mode roundtrip with 0 and N columns", () => {
    const empty_enc = PgCopyResponseEncoder.init(PgCopyDirection.CopyIn, PgCopyFormat.Text);
    expect(empty_enc.payloadLen()).toBe(3);
    expect(empty_enc.frameLen()).toBe(PG_COPY_RESP_HEADER_LEN);

    let buf: [64]u8 = [0; 64];
    const n0 = empty_enc.encodeFrame(buf[0..])?;
    expect(n0).toBe(8);
    expect(buf[0]).toBe(PG_TAG_COPY_IN_RESPONSE);
    // msg_len = 7 (4 len + 1 format + 2 cols)
    expect(buf[4]).toBe(7);
    expect(buf[5]).toBe(PG_COPY_FORMAT_TEXT);
    expect(buf[6]).toBe(0);
    expect(buf[7]).toBe(0);

    const v0 = PgCopyResponseView.decodeFrame(buf[0..n0])?;
    expect(v0.direction == PgCopyDirection.CopyIn).toBe(true);
    expect(v0.isText()).toBe(true);
    expect(v0.isBinary()).toBe(false);
    expect(v0.column_count).toBe(0);
    expect(v0.allColumnsMatchOverall()).toBe(true);

    let enc = PgCopyResponseEncoder.init(PgCopyDirection.CopyOut, PgCopyFormat.Text);
    * = enc.addColumn(PgCopyFormat.Text)?;
    * = enc.addColumn(PgCopyFormat.Text)?;
    * = enc.addColumn(PgCopyFormat.Text)?;
    const n3 = enc.encodeFrame(buf[0..])?;
    expect(n3).toBe(14);
    expect(buf[0]).toBe(PG_TAG_COPY_OUT_RESPONSE);

    const v3 = PgCopyResponseView.decodeFrame(buf[0..n3])?;
    expect(v3.direction == PgCopyDirection.CopyOut).toBe(true);
    expect(v3.column_count).toBe(3);
    expect(v3.allColumnsMatchOverall()).toBe(true);
});

test("CopyBothResponse ('W') mixed binary/text columns & iterator", () => {
    let enc = PgCopyResponseEncoder.init(PgCopyDirection.CopyBoth, PgCopyFormat.Binary);
    * = enc.addColumn(PgCopyFormat.Binary)?;
    * = enc.addColumn(PgCopyFormat.Text)?;
    * = enc.addColumn(PgCopyFormat.Binary)?;

    let buf: [64]u8 = [0; 64];
    const n = enc.encodeFrame(buf[0..])?;
    expect(n).toBe(14);
    expect(buf[0]).toBe(PG_TAG_COPY_BOTH_RESPONSE);
    expect(buf[5]).toBe(PG_COPY_FORMAT_BINARY);

    const view = PgCopyResponseView.decodeFrame(buf[0..n])?;
    expect(view.direction == PgCopyDirection.CopyBoth).toBe(true);
    expect(view.isBinary()).toBe(true);
    expect(view.allColumnsMatchOverall()).toBe(false);
    expect(view.columnFormatAt(0)? == PgCopyFormat.Binary).toBe(true);
    expect(view.columnFormatAt(1)? == PgCopyFormat.Text).toBe(true);
    expect(view.columnFormatAt(2)? == PgCopyFormat.Binary).toBe(true);

    let it = view.iter();
    expect(it.remaining()).toBe(3);
    if (c0 := it.next()) {
        expect(c0 == PgCopyFormat.Binary).toBe(true);
    } else {
        expect(false).toBe(true);
    }
    if (c1 := it.next()) {
        expect(c1 == PgCopyFormat.Text).toBe(true);
    } else {
        expect(false).toBe(true);
    }
    expect(it.remaining()).toBe(1);
    it.reset();
    expect(it.remaining()).toBe(3);
});

test("encodeFrameFromSlice supports > 64 columns zero-alloc", () => {
    let cols: [72]u8 = [0; 72];
    let i: usize = 0;
    while (i < 72) {
        cols[i] = if ((i & 1) == 0) PG_COPY_FORMAT_BINARY else PG_COPY_FORMAT_TEXT;
        i += 1;
    }

    let wire: [256]u8 = [0; 256];
    const n = PgCopyResponseEncoder.encodeFrameFromSlice(
        PgCopyDirection.CopyIn,
        PgCopyFormat.Binary,
        cols[0..],
        wire[0..],
    )?;
    expect(n).toBe(PG_COPY_RESP_HEADER_LEN + 72 * 2);

    const view = PgCopyResponseView.decodeFrame(wire[0..n])?;
    expect(view.column_count).toBe(72);
    expect(view.columnFormatAt(0)? == PgCopyFormat.Binary).toBe(true);
    expect(view.columnFormatAt(71)? == PgCopyFormat.Text).toBe(true);
});

test("encoder bounds: TooManyColumns, InvalidFormatCode, BufferTooSmall", () => {
    let enc = PgCopyResponseEncoder.init(PgCopyDirection.CopyIn, PgCopyFormat.Text);
    let i: usize = 0;
    while (i < PG_COPY_RESP_MAX_COLS) {
        * = enc.addColumn(PgCopyFormat.Text)?;
        i += 1;
    }
    let got_many = false;
    * = enc.addColumn(PgCopyFormat.Text) catch |e| {
        got_many = (e == PgCopyResponseError.TooManyColumns);
        0
    };
    expect(got_many).toBe(true);

    let wire: [64]u8 = [0; 64];
    const bad_cols: [2]u8 = [0, 2];
    let got_bad_fmt = false;
    * = PgCopyResponseEncoder.encodeFrameFromSlice(
        PgCopyDirection.CopyOut,
        PgCopyFormat.Text,
        bad_cols[0..],
        wire[0..],
    ) catch |e| {
        got_bad_fmt = (e == PgCopyResponseError.InvalidFormatCode);
        0
    };
    expect(got_bad_fmt).toBe(true);

    let tiny: [4]u8 = [0; 4];
    let got_small = false;
    * = enc.encodeFrame(tiny[0..]) catch |e| {
        got_small = (e == PgCopyResponseError.BufferTooSmall);
        0
    };
    expect(got_small).toBe(true);
});

test("decoder error guards: InvalidTag, InvalidLength, InvalidFormatCode, NegativeColumnCount, Truncated, TrailingBytes, IndexOutOfBounds", () => {
    // InvalidTag
    const bad_tag: [8]u8 = ['X', 0, 0, 0, 7, 0, 0, 0];
    let got_tag = false;
    * = PgCopyResponseView.decodeFrame(bad_tag[0..]) catch |e| {
        got_tag = (e == PgCopyResponseError.InvalidTag);
        PgCopyResponseView{}
    };
    expect(got_tag).toBe(true);

    // InvalidLength (< 7)
    const bad_len: [8]u8 = ['G', 0, 0, 0, 6, 0, 0, 0];
    let got_len = false;
    * = PgCopyResponseView.decodeFrame(bad_len[0..]) catch |e| {
        got_len = (e == PgCopyResponseError.InvalidLength);
        PgCopyResponseView{}
    };
    expect(got_len).toBe(true);

    // Invalid overall format code (2)
    const bad_overall: [8]u8 = ['G', 0, 0, 0, 7, 2, 0, 0];
    let got_overall = false;
    * = PgCopyResponseView.decodeFrame(bad_overall[0..]) catch |e| {
        got_overall = (e == PgCopyResponseError.InvalidFormatCode);
        PgCopyResponseView{}
    };
    expect(got_overall).toBe(true);

    // NegativeColumnCount (-1 => 0xFF, 0xFF)
    const neg_cols: [8]u8 = ['H', 0, 0, 0, 7, 0, 0xFF, 0xFF];
    let got_neg = false;
    * = PgCopyResponseView.decodeFrame(neg_cols[0..]) catch |e| {
        got_neg = (e == PgCopyResponseError.NegativeColumnCount);
        PgCopyResponseView{}
    };
    expect(got_neg).toBe(true);

    // Invalid column format code (0x00, 0x05)
    const bad_col_fmt: [10]u8 = ['W', 0, 0, 0, 9, 1, 0, 1, 0, 5];
    let got_col_fmt = false;
    * = PgCopyResponseView.decodeFrame(bad_col_fmt[0..]) catch |e| {
        got_col_fmt = (e == PgCopyResponseError.InvalidFormatCode);
        PgCopyResponseView{}
    };
    expect(got_col_fmt).toBe(true);

    // Truncated
    const trunc: [6]u8 = ['G', 0, 0, 0, 7, 0];
    let got_trunc = false;
    * = PgCopyResponseView.decodeFrame(trunc[0..]) catch |e| {
        got_trunc = (e == PgCopyResponseError.Truncated);
        PgCopyResponseView{}
    };
    expect(got_trunc).toBe(true);

    // TrailingBytes
    const trailing: [9]u8 = ['G', 0, 0, 0, 7, 0, 0, 0, 0xAA];
    let got_trail = false;
    * = PgCopyResponseView.decodeFrame(trailing[0..]) catch |e| {
        got_trail = (e == PgCopyResponseError.TrailingBytes);
        PgCopyResponseView{}
    };
    expect(got_trail).toBe(true);

    // IndexOutOfBounds
    const valid: [10]u8 = ['G', 0, 0, 0, 9, 0, 0, 1, 0, 0];
    const view = PgCopyResponseView.decodeFrame(valid[0..])?;
    let got_oob = false;
    * = view.columnFormatAt(1) catch |e| {
        got_oob = (e == PgCopyResponseError.IndexOutOfBounds);
        PgCopyFormat.Text
    };
    expect(got_oob).toBe(true);
});
