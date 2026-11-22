// Production Deep Tests for cap:sql PostgreSQL Wire Writer (lib/sql/pg_wire_writer.cp)
import { test, expect_eq } from "cap:test";
import {
    PG_NULL_LENGTH_U32,
    PG_NULL_LENGTH_I32,
    PgWireWriterError,
    PgLengthPatch,
    PgWireWriter,
} from "cap:sql";

test("sql pg_wire_writer: big-endian integer and IEEE-754 float encoding", fn() {
    let buf: [64]u8 = [0; 64];
    let w = PgWireWriter.init(buf[0..]);

    expect_eq(w.writeU16(0x1234) catch unreachable, @as(usize, 2));
    expect_eq(w.writeShort(513) catch unreachable, @as(usize, 2));
    expect_eq(w.writeU32(0x01020304) catch unreachable, @as(usize, 4));
    expect_eq(w.writeI32(-2) catch unreachable, @as(usize, 4));
    expect_eq(w.writeI64(-1) catch unreachable, @as(usize, 8));
    expect_eq(w.writeF32(1.5) catch unreachable, @as(usize, 4));
    expect_eq(w.writeF64(-2.25) catch unreachable, @as(usize, 8));

    const out = w.written();
    expect_eq(out.len, @as(usize, 32));
    expect_eq(w.remaining(), @as(usize, 32));

    // u16 0x1234
    expect_eq(out[0], @as(u8, 0x12));
    expect_eq(out[1], @as(u8, 0x34));
    // short 513 = 0x0201
    expect_eq(out[2], @as(u8, 0x02));
    expect_eq(out[3], @as(u8, 0x01));
    // u32 0x01020304
    expect_eq(out[4], @as(u8, 0x01));
    expect_eq(out[7], @as(u8, 0x04));
    // i32 -2 = 0xfffffffe
    expect_eq(out[8], @as(u8, 0xff));
    expect_eq(out[11], @as(u8, 0xfe));
    // i64 -1 = 0xffffffffffffffff
    expect_eq(out[12], @as(u8, 0xff));
    expect_eq(out[19], @as(u8, 0xff));
    // f32 1.5 = 0x3fc00000
    expect_eq(out[20], @as(u8, 0x3f));
    expect_eq(out[21], @as(u8, 0xc0));
    expect_eq(out[22], @as(u8, 0x00));
    expect_eq(out[23], @as(u8, 0x00));
    // f64 -2.25 = 0xc002000000000000
    expect_eq(out[24], @as(u8, 0xc0));
    expect_eq(out[25], @as(u8, 0x02));
    expect_eq(out[31], @as(u8, 0x00));
});

test("sql pg_wire_writer: CString NUL-termination, bool text, NULL sentinel and length-prefixed payload", fn() {
    let buf: [64]u8 = [0; 64];
    let w = PgWireWriter.init(buf[0..]);

    // Empty CString writes a single NUL byte
    expect_eq(w.writeCString("") catch unreachable, @as(usize, 1));
    // Non-terminated CString appends NUL
    expect_eq(w.writeCString("pg") catch unreachable, @as(usize, 3));
    // Already NUL-terminated CString does not append a duplicate NUL
    const already_z: [3]u8 = ['o', 'k', 0];
    expect_eq(w.writeCString(already_z[0..]) catch unreachable, @as(usize, 3));
    // Text bools
    expect_eq(w.writeBoolText(true) catch unreachable, @as(usize, 1));
    expect_eq(w.writeBoolText(false) catch unreachable, @as(usize, 1));
    // NULL sentinel (-1 = 0xffffffff)
    expect_eq(PG_NULL_LENGTH_U32, @as(u32, 0xffffffff));
    expect_eq(PG_NULL_LENGTH_I32, @as(i32, -1));
    expect_eq(w.writeNull() catch unreachable, @as(usize, 4));
    // Length-prefixed payload
    expect_eq(w.writeLengthPrefixed("abc") catch unreachable, @as(usize, 7));

    const out = w.written();
    expect_eq(out.len, @as(usize, 20));
    expect_eq(out[0], @as(u8, 0));
    expect_eq(out[1], @as(u8, 'p'));
    expect_eq(out[2], @as(u8, 'g'));
    expect_eq(out[3], @as(u8, 0));
    expect_eq(out[4], @as(u8, 'o'));
    expect_eq(out[5], @as(u8, 'k'));
    expect_eq(out[6], @as(u8, 0));
    expect_eq(out[7], @as(u8, 't'));
    expect_eq(out[8], @as(u8, 'f'));
    expect_eq(out[9], @as(u8, 0xff));
    expect_eq(out[12], @as(u8, 0xff));
    expect_eq(out[16], @as(u8, 3));
    expect_eq(out[17], @as(u8, 'a'));
    expect_eq(out[19], @as(u8, 'c'));
});

test("sql pg_wire_writer: tagged Simple Query ('Q') frame with inclusive length backpatch", fn() {
    let buf: [64]u8 = [0; 64];
    let w = PgWireWriter.init(buf[0..]);

    const patch = w.beginTaggedFrame('Q') catch unreachable;
    * = w.writeCString("SELECT 1") catch unreachable;
    const frame_len = w.finishLength(patch) catch unreachable;

    // 4-byte length field + 9-byte "SELECT 1\0" = 13
    expect_eq(frame_len, @as(u32, 13));
    const out = w.written();
    expect_eq(out.len, @as(usize, 14));
    expect_eq(out[0], @as(u8, 'Q'));
    expect_eq(out[1], @as(u8, 0));
    expect_eq(out[2], @as(u8, 0));
    expect_eq(out[3], @as(u8, 0));
    expect_eq(out[4], @as(u8, 13));
    expect_eq(out[5], @as(u8, 'S'));
    expect_eq(out[13], @as(u8, 0));
});

test("sql pg_wire_writer: finishLengthExcludingSelf, pwrite and reset buffer reuse", fn() {
    let buf: [32]u8 = [0; 32];
    let w = PgWireWriter.init(buf[0..]);

    const patch = w.beginLength() catch unreachable;
    * = w.writeBytes("hello") catch unreachable;
    const payload_len = w.finishLengthExcludingSelf(patch) catch unreachable;
    expect_eq(payload_len, @as(u32, 5));

    const out = w.written();
    expect_eq(out.len, @as(usize, 9));
    expect_eq(out[3], @as(u8, 5));
    expect_eq(out[4], @as(u8, 'h'));

    // pwrite overwrites in place without moving pos
    expect_eq(w.pwrite("WORLD", 4) catch unreachable, @as(usize, 5));
    expect_eq(w.offset(), @as(usize, 9));
    expect_eq(w.written()[4], @as(u8, 'W'));
    expect_eq(w.written()[8], @as(u8, 'D'));

    // reset reuses the same stack buffer
    w.reset();
    expect_eq(w.offset(), @as(usize, 0));
    expect_eq(w.remaining(), @as(usize, 32));
    * = w.writeByte('S') catch unreachable;
    expect_eq(w.written().len, @as(usize, 1));
});

test("sql pg_wire_writer: multi-frame pipeline (Parse + Bind + Execute + Sync)", fn() {
    let buf: [128]u8 = [0; 128];
    let w = PgWireWriter.init(buf[0..]);

    // Parse ('P'): name="", query="SELECT $1", param_count=0
    const p_patch = w.beginTaggedFrame('P') catch unreachable;
    * = w.writeCString("") catch unreachable;
    * = w.writeCString("SELECT $1") catch unreachable;
    * = w.writeShort(0) catch unreachable;
    const p_len = w.finishLength(p_patch) catch unreachable;
    expect_eq(p_len, @as(u32, 4 + 1 + 10 + 2));

    // Bind ('B'): portal="", stmt="", fmt=0, params=2 ("42", NULL), res_fmt=0
    const b_patch = w.beginTaggedFrame('B') catch unreachable;
    * = w.writeCString("") catch unreachable;
    * = w.writeCString("") catch unreachable;
    * = w.writeShort(0) catch unreachable;
    * = w.writeShort(2) catch unreachable;
    * = w.writeLengthPrefixed("42") catch unreachable;
    * = w.writeNull() catch unreachable;
    * = w.writeShort(0) catch unreachable;
    const b_len = w.finishLength(b_patch) catch unreachable;
    expect_eq(b_len, @as(u32, 4 + 1 + 1 + 2 + 2 + 6 + 4 + 2));

    // Sync ('S'): length=4
    const s_patch = w.beginTaggedFrame('S') catch unreachable;
    const s_len = w.finishLength(s_patch) catch unreachable;
    expect_eq(s_len, @as(u32, 4));

    expect_eq(w.offset(), @as(usize, (1 + 17) + (1 + 22) + (1 + 4)));
});

test("sql pg_wire_writer: BufferTooSmall, TooManyParameters and InvalidPatchOffset guards", fn() {
    let tiny: [4]u8 = [0; 4];
    let w = PgWireWriter.init(tiny[0..]);

    // TooManyParameters (> 65535)
    let too_many = false;
    * = w.writeShort(65536) catch |e| {
        too_many = (e == PgWireWriterError.TooManyParameters);
        @as(usize, 0)
    };
    expect_eq(too_many, true);

    // BufferTooSmall for beginTaggedFrame (needs 5 bytes, buffer has 4)
    let buf_small = false;
    * = w.beginTaggedFrame('Q') catch |e| {
        buf_small = (e == PgWireWriterError.BufferTooSmall);
        PgLengthPatch{}
    };
    expect_eq(buf_small, true);

    // InvalidPatchOffset when patch exceeds current written pos
    let bad_patch = false;
    * = w.finishLength(PgLengthPatch{ offset: 0 }) catch |e| {
        bad_patch = (e == PgWireWriterError.InvalidPatchOffset);
        @as(u32, 0)
    };
    expect_eq(bad_patch, true);

    // InvalidPatchOffset on out-of-bounds pwrite
    let bad_pwrite = false;
    * = w.pwrite("abcd", 2) catch |e| {
        bad_pwrite = (e == PgWireWriterError.InvalidPatchOffset);
        @as(usize, 0)
    };
    expect_eq(bad_pwrite, true);
});
