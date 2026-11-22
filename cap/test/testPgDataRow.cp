// Production deep test suite for cap:pgwire DataRow ('D') zero-copy decoder & streaming encoder.

import { test, expect } from "cap:test";
import {
    PG_TAG_DATA_ROW,
    PG_DATA_ROW_NULL_LEN,
    PG_DATA_ROW_HEADER_LEN,
    PgDataRowCell,
    PgDataRowView,
    PgDataRowEncoder,
} from "cap:pgwire";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("pgwire DataRow: mixed text, NULL, empty string, and binary columns roundtrip", fn() {
    let buf: [256]u8 = [0; 256];
    let enc = PgDataRowEncoder.init(buf[0..]) catch unreachable;

    const c0 = enc.writeBytes("1001") catch unreachable;
    const c1 = enc.writeBytes("alice") catch unreachable;
    const c2 = enc.writeNull() catch unreachable;
    const c3 = enc.writeBytes("") catch unreachable;
    const bin_val: [4]u8 = [0x00, 0xFF, 0x00, 0x7F];
    const c4 = enc.writeCell(bin_val[0..]) catch unreachable;
    const c5 = enc.writeCell(null) catch unreachable;

    expect(c0 == 0 && c1 == 1 && c2 == 2 && c3 == 3 && c4 == 4 && c5 == 5);
    expect(enc.columnCount() == 6);

    const frame_len = enc.finish() catch unreachable;
    expect(frame_len == enc.writtenLen());
    expect(buf[0] == PG_TAG_DATA_ROW);
    expect(PG_DATA_ROW_NULL_LEN == -1);
    expect(PG_DATA_ROW_HEADER_LEN == 7);

    const row = PgDataRowView.decodeFrame(buf[0..frame_len]) catch unreachable;
    expect(row.columnCount() == 6);
    expect(!row.isEmpty());
    expect(row.nullCount() == 2);
    expect(row.nonNullCount() == 4);

    // Random access checks
    if (cell0 := row.columnAt(0)) {
        expect(!cell0.isNull() && !cell0.isEmpty() && cell0.byteLen() == 4);
        if (v := cell0.value) { expect(strEql(v, "1001")); } else { expect(false); }
    } else {
        expect(false);
    }

    if (cell2 := row.columnAt(2)) {
        expect(cell2.isNull());
        expect(!cell2.isEmpty());
        expect(cell2.byteLen() == 0);
        expect(cell2.value == null);
    } else {
        expect(false);
    }

    if (cell3 := row.columnAt(3)) {
        expect(!cell3.isNull());
        expect(cell3.isEmpty());
        expect(cell3.byteLen() == 0);
        if (v := cell3.value) { expect(v.len == 0); } else { expect(false); }
    } else {
        expect(false);
    }

    if (cell4 := row.columnAt(4)) {
        expect(cell4.byteLen() == 4);
        if (v := cell4.value) { expect(strEql(v, bin_val[0..])); } else { expect(false); }
    } else {
        expect(false);
    }

    expect(row.columnAt(6) == null);
    expect(row.columnAt(99) == null);
});

test("pgwire DataRow: sequential iterator and cell equality", fn() {
    let buf: [128]u8 = [0; 128];
    let enc = PgDataRowEncoder.init(buf[0..]) catch unreachable;
    * = enc.writeBytes("k1") catch unreachable;
    * = enc.writeNull() catch unreachable;
    * = enc.writeBytes("v3") catch unreachable;
    const n = enc.finish() catch unreachable;

    const row = PgDataRowView.decodeFrame(buf[0..n]) catch unreachable;
    let it = row.iter();
    expect(it.remaining() == 3);

    if (a := it.next()) {
        expect(a.index == 0);
        expect(a.eql(PgDataRowCell{ index: 0, value: "k1" }));
        expect(!a.eql(PgDataRowCell{ index: 0, value: null }));
    } else {
        expect(false);
    }
    expect(it.remaining() == 2);

    if (b := it.next()) {
        expect(b.index == 1);
        expect(b.eql(PgDataRowCell{ index: 1, value: null }));
    } else {
        expect(false);
    }
    expect(it.remaining() == 1);

    if (c := it.next()) {
        expect(c.index == 2);
        expect(c.eql(PgDataRowCell{ index: 2, value: "v3" }));
    } else {
        expect(false);
    }
    expect(it.remaining() == 0);
    expect(it.next() == null);
});

test("pgwire DataRow: zero-column empty row and decodePayload", fn() {
    let buf: [32]u8 = [0; 32];
    let enc = PgDataRowEncoder.init(buf[0..]) catch unreachable;
    const n = enc.finish() catch unreachable;
    expect(n == 7);
    expect(buf[0] == 'D');
    expect(buf[1] == 0 && buf[2] == 0 && buf[3] == 0 && buf[4] == 6);
    expect(buf[5] == 0 && buf[6] == 0);

    const row = PgDataRowView.decodeFrame(buf[0..n]) catch unreachable;
    expect(row.isEmpty());
    expect(row.columnCount() == 0);
    expect(row.nullCount() == 0);
    expect(row.nonNullCount() == 0);
    let it = row.iter();
    expect(it.next() == null);

    // Direct decodePayload (without 5-byte frame header)
    const via_payload = PgDataRowView.decodePayload(buf[5..n]) catch unreachable;
    expect(via_payload.isEmpty());
});

test("pgwire DataRow: malformed frame header and framing boundary guards", fn() {
    // 1. Truncated header (< 7 bytes)
    const short_f: [6]u8 = ['D', 0, 0, 0, 6, 0];
    let e1 = false;
    * = PgDataRowView.decodeFrame(short_f[0..]) catch { e1 = true; };
    expect(e1);

    // 2. Wrong tag
    const bad_tag: [7]u8 = ['T', 0, 0, 0, 6, 0, 0];
    let e2 = false;
    * = PgDataRowView.decodeFrame(bad_tag[0..]) catch { e2 = true; };
    expect(e2);

    // 3. Invalid length (< 6)
    const bad_len: [7]u8 = ['D', 0, 0, 0, 5, 0, 0];
    let e3 = false;
    * = PgDataRowView.decodeFrame(bad_len[0..]) catch { e3 = true; };
    expect(e3);

    // 4. Truncated frame body
    const trunc_frame: [7]u8 = ['D', 0, 0, 0, 10, 0, 1];
    let e4 = false;
    * = PgDataRowView.decodeFrame(trunc_frame[0..]) catch { e4 = true; };
    expect(e4);

    // 5. Trailing bytes after frame
    const trail_frame: [8]u8 = ['D', 0, 0, 0, 6, 0, 0, 0xFF];
    let e5 = false;
    * = PgDataRowView.decodeFrame(trail_frame[0..]) catch { e5 = true; };
    expect(e5);
});

test("pgwire DataRow: malformed column lengths, truncation, and trailing cell bytes guards", fn() {
    // 1. Invalid negative column length (-2 = 0xFFFFFFFE)
    const neg2_col: [11]u8 = ['D', 0, 0, 0, 10, 0, 1, 0xFF, 0xFF, 0xFF, 0xFE];
    let e1 = false;
    * = PgDataRowView.decodeFrame(neg2_col[0..]) catch { e1 = true; };
    expect(e1);

    // 2. Column declared length exceeds remaining payload
    const trunc_col: [12]u8 = ['D', 0, 0, 0, 11, 0, 1, 0, 0, 0, 5, 'a'];
    let e2 = false;
    * = PgDataRowView.decodeFrame(trunc_col[0..]) catch { e2 = true; };
    expect(e2);

    // 3. Column count declares 2 columns but only 1 present
    const missing_col: [11]u8 = ['D', 0, 0, 0, 10, 0, 2, 0xFF, 0xFF, 0xFF, 0xFF];
    let e3 = false;
    * = PgDataRowView.decodeFrame(missing_col[0..]) catch { e3 = true; };
    expect(e3);

    // 4. Trailing bytes inside payload after all declared columns
    const extra_inside: [12]u8 = ['D', 0, 0, 0, 11, 0, 1, 0xFF, 0xFF, 0xFF, 0xFF, 0xAA];
    let e4 = false;
    * = PgDataRowView.decodeFrame(extra_inside[0..]) catch { e4 = true; };
    expect(e4);

    // 5. decodePayload with < 2 bytes
    const tiny_p: [1]u8 = [0];
    let e5 = false;
    * = PgDataRowView.decodePayload(tiny_p[0..]) catch { e5 = true; };
    expect(e5);
});

test("pgwire DataRowEncoder: buffer capacity guards", fn() {
    // 1. Buffer smaller than 7-byte header
    let tiny: [6]u8 = [0; 6];
    let e1 = false;
    * = PgDataRowEncoder.init(tiny[0..]) catch { e1 = true; };
    expect(e1);

    // 2. Buffer fits header (7B) + 1 NULL (4B) = 11B, fails on second NULL
    let exact: [11]u8 = [0; 11];
    let enc = PgDataRowEncoder.init(exact[0..]) catch unreachable;
    * = enc.writeNull() catch unreachable;
    let e2 = false;
    * = enc.writeNull() catch { e2 = true; };
    expect(e2);

    // 3. Buffer fails when writeBytes exceeds remaining space
    let e3 = false;
    * = enc.writeBytes("x") catch { e3 = true; };
    expect(e3);

    // Previous valid column can still be finalized cleanly
    const final_len = enc.finish() catch unreachable;
    expect(final_len == 11);
    const row = PgDataRowView.decodeFrame(exact[0..final_len]) catch unreachable;
    expect(row.columnCount() == 1 && row.nullCount() == 1);
});
