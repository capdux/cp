import { test, expect } from "cap:test";
import {
    PG_LENGTH_PLACEHOLDER_LEN,
    PG_DEFAULT_MAX_FRAME_BYTES,
    PgWireWriterError,
    PgLengthBookmark,
    PgWireWriter,
    PgStackReader,
} from "cap:pgwire";

test("PgWireWriter big-endian scalars, floats & roundtrip via PgStackReader", () => {
    let buf: [64]u8 = [0; 64];
    let w = PgWireWriter.init(buf[0..]);
    * = w.writeU8(0x5A)?;
    * = w.writeI16Be(-1234)?;
    * = w.writeU16Be(0xABCD)?;
    * = w.writeI32Be(-99999)?;
    * = w.writeU32Be(0x11223344)?;
    * = w.writeI64Be(-42)?;
    * = w.writeU64Be(0x0102030405060708)?;
    * = w.writeF32Be(1.5)?;
    * = w.writeF64Be(3.25)?;
    * = w.writeCString("postgres")?;

    expect(w.len()).toBe(1 + 2 + 2 + 4 + 4 + 8 + 8 + 4 + 8 + 9);

    let r = PgStackReader.init(w.written());
    expect(r.readU8()?).toBe(0x5A);
    expect(r.readI16Be()?).toBe(-1234);
    expect(r.readU16Be()?).toBe(0xABCD);
    expect(r.readI32Be()?).toBe(-99999);
    expect(r.readU32Be()?).toBe(0x11223344);
    expect(r.readI64Be()?).toBe(-42);
    expect(r.readU64Be()?).toBe(0x0102030405060708);
    // 1.5 in IEEE-754 f32 = 0x3FC00000
    expect(r.readU32Be()?).toBe(0x3FC00000);
    // 3.25 in IEEE-754 f64 = 0x400A000000000000
    expect(r.readU64Be()?).toBe(0x400A000000000000);
    expect(r.readZ()?).toBe("postgres");
    expect(r.isAtEnd()).toBe(true);
});

test("beginFrame + finishLength back-patching & nested excluding-self length", () => {
    let buf: [128]u8 = [0; 128];
    let w = PgWireWriter.init(buf[0..]);

    // Frame 1: Simple Query 'Q'
    const bm1 = w.beginFrame('Q')?;
    * = w.writeCString("SELECT 1")?;
    const len1 = w.finishLength(bm1)?;
    // 4 (len) + 8 ("SELECT 1") + 1 (NUL) = 13
    expect(len1).toBe(13);

    // Frame 2: DataRow-like frame with nested excluding-self field length
    const bm2 = w.beginFrame('D')?;
    * = w.writeU16Be(1)?;
    const field_bm = w.beginLength(false)?;
    * = w.writeBytes("abc")?;
    const field_len = w.finishLength(field_bm)?;
    expect(field_len).toBe(3);
    const len2 = w.finishLength(bm2)?;
    // 4 (len) + 2 (col_count) + 4 (field_len) + 3 ("abc") = 13
    expect(len2).toBe(13);

    let r = PgStackReader.init(w.written());
    const f1 = r.readFrame(PG_DEFAULT_MAX_FRAME_BYTES)?;
    expect(f1.tag).toBe('Q');
    expect(f1.msg_len).toBe(13);
    expect(f1.payloadLen()).toBe(9);

    const f2 = r.readFrame(PG_DEFAULT_MAX_FRAME_BYTES)?;
    expect(f2.tag).toBe('D');
    expect(f2.msg_len).toBe(13);
    expect(r.isAtEnd()).toBe(true);
});

test("writeNullableBytes & writeStringZ deduplication", () => {
    let buf: [64]u8 = [0; 64];
    let w = PgWireWriter.init(buf[0..]);

    * = w.writeNullableBytes(null)?;
    * = w.writeNullableBytes("val")?;

    const already_z: [3]u8 = ['o', 'k', 0];
    * = w.writeStringZ(already_z[0..])?;
    * = w.writeStringZ("hi")?;

    let r = PgStackReader.init(w.written());
    expect(r.readI32Be()?).toBe(-1);
    expect(r.readI32Be()?).toBe(3);
    expect(r.readBytes(3)?).toBe("val");
    expect(r.readZ()?).toBe("ok");
    expect(r.readZ()?).toBe("hi");
    expect(r.isAtEnd()).toBe(true);
});

test("reset & pwriteBytes in-place patching", () => {
    let buf: [16]u8 = [0; 16];
    let w = PgWireWriter.init(buf[0..]);
    * = w.writeBytes("hello world")?;
    * = w.pwriteBytes("CP!!", 6)?;
    expect(w.len()).toBe(11);
    expect(w.written()[6]).toBe('C');
    expect(w.written()[7]).toBe('P');

    w.reset();
    expect(w.len()).toBe(0);
    expect(w.remaining()).toBe(16);
    expect(PG_LENGTH_PLACEHOLDER_LEN).toBe(4);
});

test("error guards: BufferTooSmall, EmbeddedNul, InvalidBookmark", () => {
    let tiny: [3]u8 = [0; 3];
    let w = PgWireWriter.init(tiny[0..]);

    let got_small = false;
    * = w.writeU32Be(1) catch |e| {
        got_small = (e == PgWireWriterError.BufferTooSmall);
        0
    };
    expect(got_small).toBe(true);

    let got_frame_small = false;
    * = w.beginFrame('Q') catch |e| {
        got_frame_small = (e == PgWireWriterError.BufferTooSmall);
        PgLengthBookmark{}
    };
    expect(got_frame_small).toBe(true);

    let buf: [16]u8 = [0; 16];
    let w2 = PgWireWriter.init(buf[0..]);
    const bad_str: [3]u8 = ['a', 0, 'b'];
    let got_nul = false;
    * = w2.writeCString(bad_str[0..]) catch |e| {
        got_nul = (e == PgWireWriterError.EmbeddedNul);
        0
    };
    expect(got_nul).toBe(true);

    // InvalidBookmark (index beyond current offset)
    const bogus_bm = PgLengthBookmark{ index: 10, include_self: true };
    let got_bm = false;
    * = w2.finishLength(bogus_bm) catch |e| {
        got_bm = (e == PgWireWriterError.InvalidBookmark);
        0
    };
    expect(got_bm).toBe(true);
});
