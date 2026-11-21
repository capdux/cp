import std from "std";
import { test, expect } from "cap:test";
import {
    PACKET_HEADER_SIZE,
    PacketHeader,
    MySqlPacketWriter,
    MySqlPacketReader,
} from "cap:mysql";

test("mysql_packet: framed packet write/backfill and zero-copy read roundtrip", () => {
    let w = MySqlPacketWriter.init(std.testing.allocator);
    const h = w.startPacket(7) catch unreachable;
    w.writeU8(0x03) catch unreachable; // COM_QUERY
    w.writeU16Le(0x1234) catch unreachable;
    w.writeU32Le(0xDEADBEEF) catch unreachable;
    w.writeU64Le(0x0102030405060708) catch unreachable;
    w.writeLenInt(42) catch unreachable;
    w.writeLenInt(1000) catch unreachable;
    w.writeLenInt(100000) catch unreachable;
    w.writeLenInt(5000000000) catch unreachable;
    w.writeLenString("SELECT 1") catch unreachable;
    w.writeCString("utf8mb4") catch unreachable;
    w.endPacket(h) catch unreachable;

    let r = MySqlPacketReader.init(w.bytes());
    const hdr = r.readHeader() catch unreachable;
    expect(hdr.sequence_id).toBe(7);
    expect(@as(usize, hdr.length)).toBe(w.offset() - PACKET_HEADER_SIZE);

    expect(r.readU8() catch unreachable).toBe(0x03);
    expect(r.readU16Le() catch unreachable).toBe(0x1234);
    expect(r.readU32Le() catch unreachable).toBe(0xDEADBEEF);
    expect(r.readU64Le() catch unreachable).toBe(0x0102030405060708);
    expect(r.readLenInt() catch unreachable).toBe(42);
    expect(r.readLenInt() catch unreachable).toBe(1000);
    expect(r.readLenInt() catch unreachable).toBe(100000);
    expect(r.readLenInt() catch unreachable).toBe(5000000000);
    expect(r.readLenString() catch unreachable).toBe("SELECT 1");
    expect(r.readCString() catch unreachable).toBe("utf8mb4");
    expect(r.remaining()).toBe(0);
});

test("mysql_packet: truncated packets and invalid length prefixes rejected", () => {
    const short3: [3]u8 = [1, 2, 3];
    const h_bad = PacketHeader.decode(short3[0..]) catch null;
    expect(h_bad == null).toBeTrue();

    const invalid_len: [1]u8 = [0xFF];
    let r1 = MySqlPacketReader.init(invalid_len[0..]);
    const li_bad = r1.readLenInt() catch null;
    expect(li_bad == null).toBeTrue();

    let r2 = MySqlPacketReader.init("no-null-term");
    const cs_bad = r2.readCString() catch null;
    expect(cs_bad == null).toBeTrue();
});
