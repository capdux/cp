// Source: cp/cap/test/test_xdr_production_deep.cp (Production deep tests for cap:xdr)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    XDR_UNIT,
    xdrPadLen,
    xdrAlignedLen,
    XdrReader,
    XdrWriter,
} from "cap:binfmt";

fn expectBytesEq(actual: []const u8, expected: []const u8) {
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

test("1. All XDR scalar types (i32, u32, i64, u64, bool, f32, f64) round-trip", () => {
    expect_eq(XDR_UNIT, 4);
    expect_eq(xdrPadLen(0), 0);
    expect_eq(xdrPadLen(1), 3);
    expect_eq(xdrPadLen(2), 2);
    expect_eq(xdrPadLen(3), 1);
    expect_eq(xdrPadLen(4), 0);
    expect_eq(xdrAlignedLen(5), 8);

    let buf: [128]u8 = [0; 128];
    let w = XdrWriter.init(buf[0..]);
    expect_eq(w.writeI32(-42), true);
    expect_eq(w.writeU32(0xDEADBEEF), true);
    expect_eq(w.writeI64(-123456789012345), true);
    expect_eq(w.writeU64(0x0123456789ABCDEF), true);
    expect_eq(w.writeBool(true), true);
    expect_eq(w.writeBool(false), true);
    expect_eq(w.writeF32(3.5 as f32), true);
    expect_eq(w.writeF64(-123.75), true);

    // 4 + 4 + 8 + 8 + 4 + 4 + 4 + 8 = 44 bytes
    const wire = w.written();
    expect_eq(wire.len, 44);

    let r = XdrReader.init(wire);
    expect_eq(r.remaining(), 44);
    expect_eq(r.readI32() ?? 0, -42 as i32);
    expect_eq(r.readU32() ?? 0, 0xDEADBEEF as u32);
    expect_eq(r.readI64() ?? 0, -123456789012345 as i64);
    expect_eq(r.readU64() ?? 0, 0x0123456789ABCDEF as u64);
    expect_eq(r.readBool() ?? false, true);
    expect_eq(r.readBool() ?? true, false);
    expect_eq(r.readF32() ?? (0.0 as f32), 3.5 as f32);
    expect_eq(r.readF64() ?? 0.0, -123.75);
    expect_eq(r.isEof(), true);
});

test("2. Opaque and string 4-byte alignment padding for lengths 0, 1, 2, 3, 4, 5, 8", () => {
    const samples: [7][]const u8 = [
        "",
        "a",
        "ab",
        "abc",
        "abcd",
        "abcde",
        "abcdefgh",
    ];
    const expected_wire_sizes: [7]usize = [4, 8, 8, 8, 8, 12, 12];

    let buf: [128]u8 = [0xAA; 128]; // prefill with non-zero to verify writer zeroes padding!
    let idx: usize = 0;
    while (idx < 7) {
        let w = XdrWriter.init(buf[0..]);
        expect_eq(w.writeString(samples[idx]), true);
        const wire = w.written();
        expect_eq(wire.len, expected_wire_sizes[idx]);

        let r = XdrReader.init(wire);
        const decoded = r.readString() ?? unreachable;
        expectBytesEq(decoded, samples[idx]);
        expect_eq(r.isEof(), true);
        idx += 1;
    }

    // Fixed opaque 3 bytes -> 4 wire bytes with 1 zero pad byte
    let fbuf: [8]u8 = [0xFF; 8];
    let fw = XdrWriter.init(fbuf[0..]);
    expect_eq(fw.writeFixedOpaque("xyz"), true);
    expect_eq(fw.written().len, 4);
    expect_eq(fbuf[3], 0 as u8);

    let fr = XdrReader.init(fw.written());
    expectBytesEq(fr.readFixedOpaque(3) ?? unreachable, "xyz");
    expect_eq(fr.isEof(), true);
});

test("3. Nested ONC RPC Call message streaming encode/decode round-trip", () => {
    // ONC RPC v2 Call: xid, msg_type=0 (CALL), rpcvers=2, prog=100003 (NFS), vers=3, proc=1 (GETATTR),
    // cred(flavor=1 AUTH_SYS, opaque body), verf(flavor=0 AUTH_NONE, empty opaque), args array
    let buf: [256]u8 = [0; 256];
    let w = XdrWriter.init(buf[0..]);
    expect_eq(w.writeU32(0x778899AA), true); // xid
    expect_eq(w.writeU32(0), true);          // CALL
    expect_eq(w.writeU32(2), true);          // rpcvers
    expect_eq(w.writeU32(100003), true);     // NFS prog
    expect_eq(w.writeU32(3), true);          // vers 3
    expect_eq(w.writeU32(1), true);          // GETATTR
    expect_eq(w.writeU32(1), true);          // AUTH_SYS
    expect_eq(w.writeOpaque("cred-bytes"), true);
    expect_eq(w.writeU32(0), true);          // AUTH_NONE
    expect_eq(w.writeOpaque(""), true);
    expect_eq(w.writeArrayLen(2), true);
    expect_eq(w.writeString("/export/data"), true);
    expect_eq(w.writeString("file.bin"), true);

    let r = XdrReader.init(w.written());
    expect_eq(r.readU32() ?? 0, 0x778899AA as u32);
    expect_eq(r.readU32() ?? 9, 0 as u32);
    expect_eq(r.readU32() ?? 0, 2 as u32);
    expect_eq(r.readU32() ?? 0, 100003 as u32);
    expect_eq(r.readU32() ?? 0, 3 as u32);
    expect_eq(r.readU32() ?? 0, 1 as u32);
    expect_eq(r.readU32() ?? 0, 1 as u32);
    expectBytesEq(r.readOpaque() ?? unreachable, "cred-bytes");
    expect_eq(r.readU32() ?? 9, 0 as u32);
    expectBytesEq(r.readOpaque() ?? unreachable, "");
    expect_eq(r.readArrayLen() ?? 0, 2 as u32);
    expectBytesEq(r.readString() ?? unreachable, "/export/data");
    expectBytesEq(r.readString() ?? unreachable, "file.bin");
    expect_eq(r.isEof(), true);
});

test("4. XdrReader skip() and partial cursor navigation", () => {
    let buf: [16]u8 = [0; 16];
    let w = XdrWriter.init(buf[0..]);
    expect_eq(w.writeU32(10), true);
    expect_eq(w.writeU32(20), true);
    expect_eq(w.writeU32(30), true);
    expect_eq(w.writeU32(40), true);

    let r = XdrReader.init(w.written());
    expect_eq(r.readU32() ?? 0, 10 as u32);
    expect_eq(r.skip(8), true);
    expect_eq(r.remaining(), 4);
    expect_eq(r.readU32() ?? 0, 40 as u32);
    expect_eq(r.skip(1), false);
    expect_eq(r.readU32() == null, true);
});

test("5. Malformed XDR rejection (bool=2, non-zero padding, truncated length, buffer overflow)", () => {
    // 1. Invalid bool value (2)
    const bad_bool: [4]u8 = [0, 0, 0, 2];
    let r_bool = XdrReader.init(bad_bool[0..]);
    expect_eq(r_bool.readBool() == null, true);
    expect_eq(r_bool.pos, 0);

    // 2. Non-zero padding byte in opaque (len=1, byte='A', pad=[0, 1, 0])
    const bad_pad: [8]u8 = [0, 0, 0, 1, 'A', 0, 1, 0];
    let r_pad = XdrReader.init(bad_pad[0..]);
    expect_eq(r_pad.readOpaque() == null, true);
    expect_eq(r_pad.pos, 0); // cursor rolled back atomically

    // 3. Length prefix exceeding remaining buffer
    const over_len: [8]u8 = [0, 0, 0, 100, 1, 2, 3, 4];
    let r_over = XdrReader.init(over_len[0..]);
    expect_eq(r_over.readOpaque() == null, true);

    // 4. Truncated scalar
    const short3: [3]u8 = [1, 2, 3];
    let r_short = XdrReader.init(short3[0..]);
    expect_eq(r_short.readU32() == null, true);
    expect_eq(r_short.readU64() == null, true);

    // 5. Writer buffer overflow guard
    let small6: [6]u8 = [0; 6];
    let w = XdrWriter.init(small6[0..]);
    expect_eq(w.writeU64(1), false);
    expect_eq(w.writeOpaque("abc"), false); // needs 8 bytes
    expect_eq(w.pos, 0);
    expect_eq(w.writeU32(99), true);
    expect_eq(w.remaining(), 2);
    expect_eq(w.writeU32(100), false);
});
