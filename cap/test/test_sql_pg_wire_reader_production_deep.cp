// Production Deep Tests for cap:sql PostgreSQL Wire Reader (lib/sql/pg_wire_reader.cp)
import { test, expect_eq } from "cap:test";
import {
    PgWireReaderError,
    PgFrameView,
    PgWireReader,
    PgWireWriter,
} from "cap:sql";

test("sql pg_wire_reader: big-endian scalars, IEEE-754 floats and peek roundtrip with PgWireWriter", fn() {
    let buf: [64]u8 = [0; 64];
    let w = PgWireWriter.init(buf[0..]);

    * = w.writeU16(0x1234) catch unreachable;
    * = w.writeU16(0xfffe) catch unreachable; // i16 = -2
    * = w.writeU32(0x89abcdef) catch unreachable;
    * = w.writeI32(-123456) catch unreachable;
    * = w.writeI64(-9876543210123) catch unreachable;
    * = w.writeF32(2.5) catch unreachable;
    * = w.writeF64(-4.125) catch unreachable;

    let r = PgWireReader.init(w.written());
    expect_eq(r.remaining(), @as(usize, 32));
    expect_eq(r.readU16() catch unreachable, @as(u16, 0x1234));
    expect_eq(r.readI16() catch unreachable, @as(i16, -2));

    // Non-consuming peekU32 before readU32
    expect_eq(r.peekU32() ?? @as(u32, 0), @as(u32, 0x89abcdef));
    expect_eq(r.readU32() catch unreachable, @as(u32, 0x89abcdef));

    // Non-consuming peekI32 before readI32
    expect_eq(r.peekI32() ?? @as(i32, 0), @as(i32, -123456));
    expect_eq(r.readI32() catch unreachable, @as(i32, -123456));

    expect_eq(r.readI64() catch unreachable, @as(i64, -9876543210123));
    expect_eq(r.readF32() catch unreachable, @as(f32, 2.5));
    expect_eq(r.readF64() catch unreachable, @as(f64, -4.125));
    expect_eq(r.remaining(), @as(usize, 0));
    expect_eq(r.peekU32() == null, true);
});

test("sql pg_wire_reader: readZ, readLengthPrefixedOrNull and eatBytes", fn() {
    let buf: [64]u8 = [0; 64];
    let w = PgWireWriter.init(buf[0..]);

    * = w.writeBytes("PG") catch unreachable;
    * = w.writeCString("users") catch unreachable;
    * = w.writeCString("") catch unreachable;
    * = w.writeLengthPrefixed("alice") catch unreachable;
    * = w.writeLengthPrefixed("") catch unreachable;
    * = w.writeNull() catch unreachable;

    let r = PgWireReader.init(w.written());
    expect_eq(r.eatBytes("PG") catch unreachable, @as(usize, 2));
    expect_eq(r.readZ() catch unreachable, "users");
    expect_eq(r.readZ() catch unreachable, "");

    const c1 = r.readLengthPrefixedOrNull() catch unreachable;
    expect_eq(c1 ?? "", "alice");

    const c2 = r.readLengthPrefixedOrNull() catch unreachable;
    expect_eq(c2 != null, true);
    expect_eq((c2 ?? "x").len, @as(usize, 0));

    const c3 = r.readLengthPrefixedOrNull() catch unreachable;
    expect_eq(c3 == null, true);
    expect_eq(r.remaining(), @as(usize, 0));
});

test("sql pg_wire_reader: multi-frame pipeline decode (Parse + Bind + Sync) roundtrip", fn() {
    let buf: [128]u8 = [0; 128];
    let w = PgWireWriter.init(buf[0..]);

    const p = w.beginTaggedFrame('P') catch unreachable;
    * = w.writeCString("s1") catch unreachable;
    * = w.writeCString("SELECT $1") catch unreachable;
    * = w.writeShort(0) catch unreachable;
    * = w.finishLength(p) catch unreachable;

    const b = w.beginTaggedFrame('B') catch unreachable;
    * = w.writeCString("") catch unreachable;
    * = w.writeCString("s1") catch unreachable;
    * = w.writeShort(0) catch unreachable;
    * = w.writeShort(2) catch unreachable;
    * = w.writeLengthPrefixed("42") catch unreachable;
    * = w.writeNull() catch unreachable;
    * = w.writeShort(0) catch unreachable;
    * = w.finishLength(b) catch unreachable;

    const s = w.beginTaggedFrame('S') catch unreachable;
    * = w.finishLength(s) catch unreachable;

    let r = PgWireReader.init(w.written());

    const f1 = r.readTaggedFrame() catch unreachable;
    expect_eq(f1.tag, @as(u8, 'P'));
    let pr = PgWireReader.init(f1.payload);
    expect_eq(pr.readZ() catch unreachable, "s1");
    expect_eq(pr.readZ() catch unreachable, "SELECT $1");
    expect_eq(pr.readU16() catch unreachable, @as(u16, 0));

    const f2 = r.readTaggedFrame() catch unreachable;
    expect_eq(f2.tag, @as(u8, 'B'));
    let br = PgWireReader.init(f2.payload);
    expect_eq(br.readZ() catch unreachable, "");
    expect_eq(br.readZ() catch unreachable, "s1");
    expect_eq(br.readU16() catch unreachable, @as(u16, 0));
    expect_eq(br.readU16() catch unreachable, @as(u16, 2));
    expect_eq((br.readLengthPrefixedOrNull() catch unreachable) ?? "", "42");
    expect_eq((br.readLengthPrefixedOrNull() catch unreachable) == null, true);
    expect_eq(br.readU16() catch unreachable, @as(u16, 0));

    const f3 = r.readTaggedFrame() catch unreachable;
    expect_eq(f3.tag, @as(u8, 'S'));
    expect_eq(f3.payload.len, @as(usize, 0));
    expect_eq(r.remaining(), @as(usize, 0));
});

test("sql pg_wire_reader: message bookmark, rewind and skip clamp", fn() {
    const wire: [8]u8 = [0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80];
    let r = PgWireReader.init(wire[0..]);

    * = r.readBytes(2) catch unreachable;
    r.markMessageStart();
    expect_eq(r.offsetFromStart(), @as(usize, 0));

    * = r.readU32() catch unreachable;
    expect_eq(r.offsetFromStart(), @as(usize, 4));

    r.rewindToMessageStart();
    expect_eq(r.offset(), @as(usize, 2));
    expect_eq(r.readByte() catch unreachable, @as(u8, 0x30));
    expect_eq(r.skip(99), @as(usize, 5));
    expect_eq(r.remaining(), @as(usize, 0));
});

test("sql pg_wire_reader: readMessageLength normal and atomic ShortRead / InvalidMessageLength", fn() {
    // Valid message length = 7 (4 header + 3 payload)
    const ok_wire: [7]u8 = [0, 0, 0, 7, 'a', 'b', 'c'];
    let r1 = PgWireReader.init(ok_wire[0..]);
    expect_eq(r1.readMessageLength() catch unreachable, @as(u32, 7));
    expect_eq(r1.readBytes(3) catch unreachable, "abc");

    // Truncated payload: declares length 10 but only 2 payload bytes follow -> ShortRead, pos stays 0
    const trunc_wire: [6]u8 = [0, 0, 0, 10, 'a', 'b'];
    let r2 = PgWireReader.init(trunc_wire[0..]);
    let got_short = false;
    * = r2.readMessageLength() catch |e| {
        got_short = (e == PgWireReaderError.ShortRead);
        @as(u32, 0)
    };
    expect_eq(got_short, true);
    expect_eq(r2.offset(), @as(usize, 0));

    // Invalid length < 4 (length = 3) -> InvalidMessageLength, pos stays 0
    const bad_len: [4]u8 = [0, 0, 0, 3];
    let r3 = PgWireReader.init(bad_len[0..]);
    let got_bad = false;
    * = r3.readMessageLength() catch |e| {
        got_bad = (e == PgWireReaderError.InvalidMessageLength);
        @as(u32, 0)
    };
    expect_eq(got_bad, true);
    expect_eq(r3.offset(), @as(usize, 0));
});

test("sql pg_wire_reader: InvalidMessage, UnterminatedString, bad negative col length and partial frame guards", fn() {
    // 1. eatBytes mismatch -> InvalidMessage, pos stays 0
    const wire1: [4]u8 = ['A', 'B', 'C', 'D'];
    let r1 = PgWireReader.init(wire1[0..]);
    let got_inv = false;
    * = r1.eatBytes("ABX") catch |e| {
        got_inv = (e == PgWireReaderError.InvalidMessage);
        @as(usize, 0)
    };
    expect_eq(got_inv, true);
    expect_eq(r1.offset(), @as(usize, 0));

    // 2. readZ without NUL -> UnterminatedString
    let got_unterm = false;
    * = r1.readZ() catch |e| {
        got_unterm = (e == PgWireReaderError.UnterminatedString);
        ""
    };
    expect_eq(got_unterm, true);

    // 3. readLengthPrefixedOrNull with -2 (0xfffffffe) -> InvalidMessageLength, pos stays 0
    const neg2: [4]u8 = [0xff, 0xff, 0xff, 0xfe];
    let r2 = PgWireReader.init(neg2[0..]);
    let got_bad_col = false;
    * = r2.readLengthPrefixedOrNull() catch |e| {
        got_bad_col = (e == PgWireReaderError.InvalidMessageLength);
        null
    };
    expect_eq(got_bad_col, true);
    expect_eq(r2.offset(), @as(usize, 0));

    // 4. readTaggedFrame with partial payload -> ShortRead, pos stays 0
    const partial_frame: [7]u8 = ['Q', 0, 0, 0, 12, 'S', 'E'];
    let r3 = PgWireReader.init(partial_frame[0..]);
    let got_frame_short = false;
    * = r3.readTaggedFrame() catch |e| {
        got_frame_short = (e == PgWireReaderError.ShortRead);
        PgFrameView{}
    };
    expect_eq(got_frame_short, true);
    expect_eq(r3.offset(), @as(usize, 0));
});
