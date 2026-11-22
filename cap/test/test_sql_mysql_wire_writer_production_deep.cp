// Production Deep Tests for cap:sql MySQL Wire Writer (lib/sql/mysql_wire_writer.cp)
import { test, expect_eq } from "cap:test";
import {
    MYSQL_PACKET_HEADER_SIZE,
    MYSQL_MAX_PACKET_PAYLOAD,
    MySqlWireWriterError,
    MySqlPacketPatch,
    MySqlWireWriter,
    MySqlWireReader,
} from "cap:sql";

test("sql mysql_wire_writer: little-endian scalars & IEEE-754 floats roundtrip with MySqlWireReader", fn() {
    let buf: [64]u8 = [0; 64];
    let w = MySqlWireWriter.init(buf[0..]);

    expect_eq(w.writeByte(0xab) catch unreachable, @as(usize, 1));
    expect_eq(w.writeU16(0x1234) catch unreachable, @as(usize, 2));
    expect_eq(w.writeU24(0x030201) catch unreachable, @as(usize, 3));
    expect_eq(w.writeU32(0x89abcdef) catch unreachable, @as(usize, 4));
    expect_eq(w.writeI32(-42) catch unreachable, @as(usize, 4));
    expect_eq(w.writeU64(0x0102030405060708) catch unreachable, @as(usize, 8));
    expect_eq(w.writeI64(-1000000000000) catch unreachable, @as(usize, 8));
    expect_eq(w.writeF32(3.5) catch unreachable, @as(usize, 4));
    expect_eq(w.writeF64(-6.25) catch unreachable, @as(usize, 8));

    let r = MySqlWireReader.init(w.written());
    expect_eq(r.remaining(), @as(usize, 42));
    expect_eq(r.readByte() catch unreachable, @as(u8, 0xab));
    expect_eq(r.readU16() catch unreachable, @as(u16, 0x1234));
    expect_eq(r.readU24() catch unreachable, @as(u32, 0x030201));
    expect_eq(r.readU32() catch unreachable, @as(u32, 0x89abcdef));
    expect_eq(r.readI32() catch unreachable, @as(i32, -42));
    expect_eq(r.readU64() catch unreachable, @as(u64, 0x0102030405060708));
    expect_eq(r.readI64() catch unreachable, @as(i64, -1000000000000));
    expect_eq(r.readF32() catch unreachable, @as(f32, 3.5));
    expect_eq(r.readF64() catch unreachable, @as(f64, -6.25));
    expect_eq(r.remaining(), @as(usize, 0));
});

test("sql mysql_wire_writer: writeZ, lenenc-int, lenenc-str and lenenc-NULL roundtrip", fn() {
    let buf: [64]u8 = [0; 64];
    let w = MySqlWireWriter.init(buf[0..]);

    expect_eq(w.writeZ("") catch unreachable, @as(usize, 1));
    expect_eq(w.writeZ("root") catch unreachable, @as(usize, 5));
    const pre_z: [3]u8 = ['d', 'b', 0];
    expect_eq(w.writeZ(pre_z[0..]) catch unreachable, @as(usize, 3));
    expect_eq(w.writeLenEncInt(250) catch unreachable, @as(usize, 1));
    expect_eq(w.writeLenEncInt(70000) catch unreachable, @as(usize, 4));
    expect_eq(w.writeLenEncString("SELECT 1") catch unreachable, @as(usize, 9));
    expect_eq(w.writeLenEncNull() catch unreachable, @as(usize, 1));

    let r = MySqlWireReader.init(w.written());
    expect_eq(r.readZ() catch unreachable, "");
    expect_eq(r.readZ() catch unreachable, "root");
    expect_eq(r.readZ() catch unreachable, "db");
    expect_eq(r.readLenEncInt() catch unreachable, @as(u64, 250));
    expect_eq(r.readLenEncInt() catch unreachable, @as(u64, 70000));
    expect_eq(r.readLenEncString() catch unreachable, "SELECT 1");
    const opt = r.readLenEncStringOrNull() catch unreachable;
    expect_eq(opt == null, true);
    expect_eq(r.remaining(), @as(usize, 0));
});

test("sql mysql_wire_writer: beginPacket/finishPacket 4-byte header and multi-packet sequence", fn() {
    expect_eq(MYSQL_PACKET_HEADER_SIZE, @as(usize, 4));
    expect_eq(MYSQL_MAX_PACKET_PAYLOAD, @as(usize, 0xffffff));

    let buf: [64]u8 = [0; 64];
    let w = MySqlWireWriter.init(buf[0..]);

    // Packet 0: COM_QUERY "SELECT 1"
    const p0 = w.beginPacket(0) catch unreachable;
    * = w.writeByte(0x03) catch unreachable;
    * = w.writeBytes("SELECT 1") catch unreachable;
    const len0 = w.finishPacket(p0) catch unreachable;
    expect_eq(len0, @as(u32, 9));

    // Packet 1: COM_PING (0x0e), seq=1
    const p1 = w.beginPacket(1) catch unreachable;
    * = w.writeByte(0x0e) catch unreachable;
    const len1 = w.finishPacket(p1) catch unreachable;
    expect_eq(len1, @as(u32, 1));

    let r = MySqlWireReader.init(w.written());
    expect_eq(r.readU24() catch unreachable, @as(u32, 9));
    expect_eq(r.readByte() catch unreachable, @as(u8, 0));
    expect_eq(r.readByte() catch unreachable, @as(u8, 0x03));
    expect_eq(r.readBytes(8) catch unreachable, "SELECT 1");

    expect_eq(r.readU24() catch unreachable, @as(u32, 1));
    expect_eq(r.readByte() catch unreachable, @as(u8, 1));
    expect_eq(r.readByte() catch unreachable, @as(u8, 0x0e));
    expect_eq(r.remaining(), @as(usize, 0));
});

test("sql mysql_wire_writer: binary protocol NULL bitmap (offset 0 and offset 2)", fn() {
    let buf: [16]u8 = [0; 16];
    let w = MySqlWireWriter.init(buf[0..]);

    // COM_STMT_EXECUTE params (bit_offset=0): 9 params, indices 0, 2, 8 are NULL
    // byte 0 = (1<<0) | (1<<2) = 0x05; byte 1 = (1<<0) = 0x01
    const flags: [9]bool = [true, false, true, false, false, false, false, false, true];
    const n1 = w.writeNullBitmap(flags[0..], 0) catch unreachable;
    expect_eq(n1, @as(usize, 2));
    expect_eq(w.written()[0], @as(u8, 0x05));
    expect_eq(w.written()[1], @as(u8, 0x01));

    // Binary ResultSet row (bit_offset=2): 3 columns, col 0 & col 2 are NULL
    // bit 2 and bit 4 -> (1<<2) | (1<<4) = 0x14
    const row_flags: [3]bool = [true, false, true];
    const n2 = w.writeNullBitmap(row_flags[0..], 2) catch unreachable;
    expect_eq(n2, @as(usize, 1));
    expect_eq(w.written()[2], @as(u8, 0x14));
});

test("sql mysql_wire_writer: pwrite in-place overwrite and reset reuse", fn() {
    let buf: [16]u8 = [0; 16];
    let w = MySqlWireWriter.init(buf[0..]);

    * = w.writeBytes("hello!") catch unreachable;
    expect_eq(w.pwrite("HELLO", 0) catch unreachable, @as(usize, 5));
    expect_eq(w.offset(), @as(usize, 6));
    expect_eq(w.written(), "HELLO!");

    w.reset();
    expect_eq(w.offset(), @as(usize, 0));
    expect_eq(w.remaining(), @as(usize, 16));
    * = w.writeByte(0x42) catch unreachable;
    expect_eq(w.written().len, @as(usize, 1));
});

test("sql mysql_wire_writer: BufferTooSmall, PacketLengthOverflow and InvalidPatchOffset guards", fn() {
    let tiny: [3]u8 = [0; 3];
    let w = MySqlWireWriter.init(tiny[0..]);

    // beginPacket needs 4 bytes, buffer has 3 -> BufferTooSmall
    let buf_err = false;
    * = w.beginPacket(0) catch |e| {
        buf_err = (e == MySqlWireWriterError.BufferTooSmall);
        MySqlPacketPatch{}
    };
    expect_eq(buf_err, true);

    // writeU24 with > 0xffffff -> PacketLengthOverflow
    let ovf_err = false;
    * = w.writeU24(0x01000000) catch |e| {
        ovf_err = (e == MySqlWireWriterError.PacketLengthOverflow);
        @as(usize, 0)
    };
    expect_eq(ovf_err, true);

    // finishPacket with patch beyond pos -> InvalidPatchOffset
    let bad_patch = false;
    * = w.finishPacket(MySqlPacketPatch{ offset: 0, sequence_id: 0 }) catch |e| {
        bad_patch = (e == MySqlWireWriterError.InvalidPatchOffset);
        @as(u32, 0)
    };
    expect_eq(bad_patch, true);

    // pwrite out of bounds -> InvalidPatchOffset
    let bad_pwrite = false;
    * = w.pwrite("abcd", 1) catch |e| {
        bad_pwrite = (e == MySqlWireWriterError.InvalidPatchOffset);
        @as(usize, 0)
    };
    expect_eq(bad_pwrite, true);
});
