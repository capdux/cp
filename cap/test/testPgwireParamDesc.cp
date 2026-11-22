import { test, expect } from "cap:test";
import {
    PG_TAG_PARAMETER_DESCRIPTION,
    PG_PARAM_DESC_HEADER_LEN,
    PG_PARAM_DESC_MAX_OIDS,
    PgParamDescError,
    PgParamDescView,
    PgParamDescEncoder,
} from "cap:pgwire";

test("empty ParameterDescription ('t') frame roundtrip", () => {
    const enc = PgParamDescEncoder.init();
    expect(enc.payloadLen()).toBe(2);
    expect(enc.frameLen()).toBe(PG_PARAM_DESC_HEADER_LEN);

    let buf: [16]u8 = [0; 16];
    const n = enc.encodeFrame(buf[0..])?;
    expect(n).toBe(7);
    expect(buf[0]).toBe(PG_TAG_PARAMETER_DESCRIPTION);
    // msg_len = 6 (4 len + 2 count)
    expect(buf[1]).toBe(0);
    expect(buf[2]).toBe(0);
    expect(buf[3]).toBe(0);
    expect(buf[4]).toBe(6);
    // count = 0
    expect(buf[5]).toBe(0);
    expect(buf[6]).toBe(0);

    const view = PgParamDescView.decodeFrame(buf[0..n])?;
    expect(view.isEmpty()).toBe(true);
    expect(view.count).toBe(0);
    expect(view.hasUnspecifiedOid()).toBe(false);
    expect(view.containsOid(23)).toBe(false);

    let it = view.iter();
    expect(it.remaining()).toBe(0);
    expect(it.next() == null).toBe(true);
});

test("multi-OID ParameterDescription big-endian roundtrip, iter & copyOids", () => {
    let enc = PgParamDescEncoder.init();
    * = enc.addOid(16)?;     // BOOL
    * = enc.addOid(23)?;     // INT4
    * = enc.addOid(25)?;     // TEXT
    * = enc.addOid(2950)?;   // UUID
    * = enc.addOid(3802)?;   // JSONB
    * = enc.addOid(0)?;      // UNSPECIFIED

    expect(enc.count).toBe(6);
    expect(enc.payloadLen()).toBe(26);
    expect(enc.frameLen()).toBe(31);

    let buf: [64]u8 = [0; 64];
    const n = enc.encodeFrame(buf[0..])?;
    expect(n).toBe(31);

    const view = PgParamDescView.decodeFrame(buf[0..n])?;
    expect(view.isEmpty()).toBe(false);
    expect(view.count).toBe(6);
    expect(view.oidAt(0)?).toBe(16);
    expect(view.oidAt(1)?).toBe(23);
    expect(view.oidAt(2)?).toBe(25);
    expect(view.oidAt(3)?).toBe(2950);
    expect(view.oidAt(4)?).toBe(3802);
    expect(view.oidAt(5)?).toBe(0);
    expect(view.containsOid(2950)).toBe(true);
    expect(view.containsOid(701)).toBe(false);
    expect(view.hasUnspecifiedOid()).toBe(true);

    // Iterator traversal + reset
    let it = view.iter();
    expect(it.remaining()).toBe(6);
    if (v0 := it.next()) {
        expect(v0).toBe(16);
    } else {
        expect(false).toBe(true);
    }
    if (v1 := it.next()) {
        expect(v1).toBe(23);
    } else {
        expect(false).toBe(true);
    }
    expect(it.remaining()).toBe(4);
    it.reset();
    expect(it.remaining()).toBe(6);

    // Bulk copyOids
    let dst: [8]u32 = [0; 8];
    const copied = view.copyOids(dst[0..])?;
    expect(copied).toBe(6);
    expect(dst[0]).toBe(16);
    expect(dst[3]).toBe(2950);
    expect(dst[4]).toBe(3802);
    expect(dst[5]).toBe(0);
});

test("encodeFrameFromSlice supports > 64 OIDs zero-alloc", () => {
    let big_oids: [80]u32 = [0; 80];
    let i: usize = 0;
    while (i < 80) {
        big_oids[i] = @as(u32, @intCast(1000 + i));
        i += 1;
    }

    let wire: [512]u8 = [0; 512];
    const n = PgParamDescEncoder.encodeFrameFromSlice(big_oids[0..], wire[0..])?;
    expect(n).toBe(PG_PARAM_DESC_HEADER_LEN + 80 * 4);

    const view = PgParamDescView.decodeFrame(wire[0..n])?;
    expect(view.count).toBe(80);
    expect(view.oidAt(0)?).toBe(1000);
    expect(view.oidAt(79)?).toBe(1079);
});

test("decodePayload direct roundtrip & high-bit OID big-endian encoding", () => {
    let enc = PgParamDescEncoder.init();
    * = enc.addOid(0x12345678)?;
    * = enc.addOid(0x80000001)?;

    let payload: [16]u8 = [0; 16];
    const pn = enc.encodePayload(payload[0..])?;
    expect(pn).toBe(10);
    expect(payload[0]).toBe(0);
    expect(payload[1]).toBe(2);
    expect(payload[2]).toBe(0x12);
    expect(payload[3]).toBe(0x34);
    expect(payload[4]).toBe(0x56);
    expect(payload[5]).toBe(0x78);

    const view = PgParamDescView.decodePayload(payload[0..pn])?;
    expect(view.oidAt(0)?).toBe(0x12345678);
    expect(view.oidAt(1)?).toBe(0x80000001);
});

test("encoder bounds: TooManyParams & BufferTooSmall", () => {
    let enc = PgParamDescEncoder.init();
    let i: usize = 0;
    while (i < PG_PARAM_DESC_MAX_OIDS) {
        * = enc.addOid(23)?;
        i += 1;
    }

    let got_too_many = false;
    * = enc.addOid(25) catch |e| {
        got_too_many = (e == PgParamDescError.TooManyParams);
        0
    };
    expect(got_too_many).toBe(true);

    let tiny: [10]u8 = [0; 10];
    let got_small = false;
    * = enc.encodeFrame(tiny[0..]) catch |e| {
        got_small = (e == PgParamDescError.BufferTooSmall);
        0
    };
    expect(got_small).toBe(true);
});

test("decoder error guards: InvalidTag, InvalidLength, NegativeCount, Truncated, TrailingBytes, IndexOutOfBounds", () => {
    // InvalidTag ('T' instead of 't')
    const bad_tag: [7]u8 = ['T', 0, 0, 0, 6, 0, 0];
    let got_tag = false;
    * = PgParamDescView.decodeFrame(bad_tag[0..]) catch |e| {
        got_tag = (e == PgParamDescError.InvalidTag);
        PgParamDescView{}
    };
    expect(got_tag).toBe(true);

    // InvalidLength (length = 4 < 6)
    const bad_len: [7]u8 = ['t', 0, 0, 0, 4, 0, 0];
    let got_len = false;
    * = PgParamDescView.decodeFrame(bad_len[0..]) catch |e| {
        got_len = (e == PgParamDescError.InvalidLength);
        PgParamDescView{}
    };
    expect(got_len).toBe(true);

    // NegativeCount (Int16 = -1 => 0xFF, 0xFF)
    const neg_cnt: [7]u8 = ['t', 0, 0, 0, 6, 0xFF, 0xFF];
    let got_neg = false;
    * = PgParamDescView.decodeFrame(neg_cnt[0..]) catch |e| {
        got_neg = (e == PgParamDescError.NegativeCount);
        PgParamDescView{}
    };
    expect(got_neg).toBe(true);

    // Truncated (count = 2 but only 1 OID present)
    const trunc: [11]u8 = ['t', 0, 0, 0, 10, 0, 2, 0, 0, 0, 23];
    let got_trunc = false;
    * = PgParamDescView.decodeFrame(trunc[0..]) catch |e| {
        got_trunc = (e == PgParamDescError.Truncated);
        PgParamDescView{}
    };
    expect(got_trunc).toBe(true);

    // TrailingBytes (frame has extra byte beyond declared length)
    const trailing: [8]u8 = ['t', 0, 0, 0, 6, 0, 0, 0xAA];
    let got_trail = false;
    * = PgParamDescView.decodeFrame(trailing[0..]) catch |e| {
        got_trail = (e == PgParamDescError.TrailingBytes);
        PgParamDescView{}
    };
    expect(got_trail).toBe(true);

    // IndexOutOfBounds & copyOids BufferTooSmall
    const one_oid: [11]u8 = ['t', 0, 0, 0, 10, 0, 1, 0, 0, 0, 23];
    const view = PgParamDescView.decodeFrame(one_oid[0..])?;
    let got_oob = false;
    * = view.oidAt(1) catch |e| {
        got_oob = (e == PgParamDescError.IndexOutOfBounds);
        0
    };
    expect(got_oob).toBe(true);

    let empty_dst: [0]u32 = [];
    let got_copy_small = false;
    * = view.copyOids(empty_dst[0..]) catch |e| {
        got_copy_small = (e == PgParamDescError.BufferTooSmall);
        0
    };
    expect(got_copy_small).toBe(true);
});
