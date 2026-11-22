// Production deep test suite for cap:pgwire RowDescription ('T') zero-copy decoder & streaming encoder.

import { test, expect } from "cap:test";
import {
    PG_TAG_ROW_DESCRIPTION,
    PG_ROW_DESC_HEADER_LEN,
    PG_ROW_DESC_FIELD_META_LEN,
    PgFormatCode,
    parseFormatCode,
    PgRowFieldDesc,
    PgRowDescView,
    PgRowDescEncoder,
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

test("pgwire RowDescription: full field metadata roundtrip and mixed text/binary formats", fn() {
    let buf: [256]u8 = [0; 256];
    let enc = PgRowDescEncoder.init(buf[0..]) catch unreachable;

    const f0 = PgRowFieldDesc{
        index: 0,
        name: "id",
        table_oid: 16384,
        column_attr: 1,
        type_oid: 23, // INT4
        type_size: 4,
        type_modifier: -1,
        format: PgFormatCode.Binary,
    };
    const f1 = PgRowFieldDesc{
        index: 1,
        name: "username",
        table_oid: 16384,
        column_attr: 2,
        type_oid: 1043, // VARCHAR
        type_size: -1,
        type_modifier: 68, // varchar(64)
        format: PgFormatCode.Text,
    };

    const idx0 = enc.writeField(f0) catch unreachable;
    const idx1 = enc.writeField(f1) catch unreachable;
    const idx2 = enc.writeSimpleText("created_at", 1184) catch unreachable; // TIMESTAMPTZ
    expect(idx0 == 0 && idx1 == 1 && idx2 == 2);
    expect(enc.fieldCount() == 3);

    const frame_len = enc.finish() catch unreachable;
    expect(frame_len == enc.writtenLen());
    expect(buf[0] == PG_TAG_ROW_DESCRIPTION);
    expect(PG_ROW_DESC_HEADER_LEN == 7);
    expect(PG_ROW_DESC_FIELD_META_LEN == 18);

    const view = PgRowDescView.decodeFrame(buf[0..frame_len]) catch unreachable;
    expect(view.fieldCount() == 3);
    expect(!view.isEmpty());
    expect(!view.allText());
    expect(!view.allBinary());

    if (d0 := view.fieldAt(0)) {
        expect(d0.eql(f0));
        expect(d0.isBinary() && !d0.isText());
        expect(!d0.isVariableLength());
    } else {
        expect(false);
    }

    if (d1 := view.fieldAt(1)) {
        expect(d1.eql(f1));
        expect(d1.isText() && !d1.isBinary());
        expect(d1.isVariableLength());
    } else {
        expect(false);
    }

    if (d2 := view.fieldAt(2)) {
        expect(strEql(d2.name, "created_at"));
        expect(d2.type_oid == 1184);
        expect(d2.isText());
        expect(d2.isVariableLength());
    } else {
        expect(false);
    }

    expect(view.fieldAt(3) == null);
});

test("pgwire RowDescription: findByName, indexOfName, iterator, and allText/allBinary", fn() {
    expect(parseFormatCode(0) == PgFormatCode.Text);
    expect(parseFormatCode(1) == PgFormatCode.Binary);
    expect(parseFormatCode(2) == null);
    expect(parseFormatCode(-1) == null);

    let buf: [256]u8 = [0; 256];
    let enc = PgRowDescEncoder.init(buf[0..]) catch unreachable;
    * = enc.writeSimpleText("a", 23) catch unreachable;
    * = enc.writeSimpleText("b", 25) catch unreachable;
    const n = enc.finish() catch unreachable;

    const view = PgRowDescView.decodeFrame(buf[0..n]) catch unreachable;
    expect(view.allText());
    expect(!view.allBinary());

    if (fb := view.findByName("b")) {
        expect(fb.index == 1 && fb.type_oid == 25);
    } else {
        expect(false);
    }
    if (ib := view.indexOfName("b")) {
        expect(ib == 1);
    } else {
        expect(false);
    }
    expect(view.findByName("missing") == null);
    expect(view.indexOfName("missing") == null);

    let it = view.iter();
    expect(it.remaining() == 2);
    expect(it.next() != null);
    expect(it.remaining() == 1);
    expect(it.next() != null);
    expect(it.remaining() == 0);
    expect(it.next() == null);

    // All binary check
    let bin_enc = PgRowDescEncoder.init(buf[0..]) catch unreachable;
    * = bin_enc.writeField(PgRowFieldDesc{ name: "x", type_oid: 20, type_size: 8, format: PgFormatCode.Binary }) catch unreachable;
    const n_bin = bin_enc.finish() catch unreachable;
    const bin_view = PgRowDescView.decodeFrame(buf[0..n_bin]) catch unreachable;
    expect(bin_view.allBinary());
    expect(!bin_view.allText());
});

test("pgwire RowDescription: zero-field empty description and decodePayload", fn() {
    let buf: [32]u8 = [0; 32];
    let enc = PgRowDescEncoder.init(buf[0..]) catch unreachable;
    const n = enc.finish() catch unreachable;
    expect(n == 7);
    expect(buf[0] == 'T');
    expect(buf[1] == 0 && buf[2] == 0 && buf[3] == 0 && buf[4] == 6);

    const view = PgRowDescView.decodeFrame(buf[0..n]) catch unreachable;
    expect(view.isEmpty());
    expect(view.fieldCount() == 0);
    expect(view.allText());
    expect(view.allBinary());

    const via_payload = PgRowDescView.decodePayload(buf[5..n]) catch unreachable;
    expect(via_payload.isEmpty());
});

test("pgwire RowDescription: malformed frame header and framing guards", fn() {
    // 1. Truncated (< 7B)
    const short_f: [6]u8 = ['T', 0, 0, 0, 6, 0];
    let e1 = false;
    * = PgRowDescView.decodeFrame(short_f[0..]) catch { e1 = true; };
    expect(e1);

    // 2. Wrong tag
    const bad_tag: [7]u8 = ['D', 0, 0, 0, 6, 0, 0];
    let e2 = false;
    * = PgRowDescView.decodeFrame(bad_tag[0..]) catch { e2 = true; };
    expect(e2);

    // 3. Invalid length (< 6)
    const bad_len: [7]u8 = ['T', 0, 0, 0, 5, 0, 0];
    let e3 = false;
    * = PgRowDescView.decodeFrame(bad_len[0..]) catch { e3 = true; };
    expect(e3);

    // 4. Trailing bytes
    const trail: [8]u8 = ['T', 0, 0, 0, 6, 0, 0, 0xFF];
    let e4 = false;
    * = PgRowDescView.decodeFrame(trail[0..]) catch { e4 = true; };
    expect(e4);
});

test("pgwire RowDescription: invalid format code, missing NUL, truncated meta, trailing payload guards", fn() {
    // Valid single-field frame with empty name "" (1B NUL + 18B meta = 19B; total = 7 + 19 = 26B; len = 25)
    let frame: [26]u8 = [0; 26];
    frame[0] = 'T';
    frame[4] = 25;
    frame[6] = 1; // 1 field
    // frame[7] = 0 (empty name NUL)
    // format_code at offset 7 + 1 + 16 = 24..26

    // 1. Invalid format_code == 2
    frame[25] = 2;
    let e1 = false;
    * = PgRowDescView.decodeFrame(frame[0..]) catch { e1 = true; };
    expect(e1);

    // 2. Missing NUL terminator in field name (all non-zero)
    let no_nul: [12]u8 = ['T', 0, 0, 0, 11, 0, 1, 'a', 'b', 'c', 'd', 'e'];
    let e2 = false;
    * = PgRowDescView.decodeFrame(no_nul[0..]) catch { e2 = true; };
    expect(e2);

    // 3. Truncated 18-byte metadata after name NUL
    let trunc_meta: [12]u8 = ['T', 0, 0, 0, 11, 0, 1, 'a', 0, 0, 0, 0];
    let e3 = false;
    * = PgRowDescView.decodeFrame(trunc_meta[0..]) catch { e3 = true; };
    expect(e3);

    // 4. Trailing garbage inside payload after declared fields
    let extra: [27]u8 = [0; 27];
    extra[0] = 'T';
    extra[4] = 26;
    extra[6] = 1;
    extra[26] = 0xAA;
    let e4 = false;
    * = PgRowDescView.decodeFrame(extra[0..]) catch { e4 = true; };
    expect(e4);
});

test("pgwire RowDescEncoder: embedded NUL and buffer capacity guards", fn() {
    // 1. Buffer smaller than header
    let tiny: [6]u8 = [0; 6];
    let e1 = false;
    * = PgRowDescEncoder.init(tiny[0..]) catch { e1 = true; };
    expect(e1);

    // 2. Embedded NUL in column name rejected
    let buf: [64]u8 = [0; 64];
    let enc = PgRowDescEncoder.init(buf[0..]) catch unreachable;
    const bad_name: [3]u8 = ['a', 0, 'b'];
    let e2 = false;
    * = enc.writeSimpleText(bad_name[0..], 25) catch { e2 = true; };
    expect(e2);

    // 3. Buffer too small for field (7B header + 2B name\0 + 18B meta = 27B > 20B)
    let small: [20]u8 = [0; 20];
    let enc2 = PgRowDescEncoder.init(small[0..]) catch unreachable;
    let e3 = false;
    * = enc2.writeSimpleText("id", 23) catch { e3 = true; };
    expect(e3);
});
