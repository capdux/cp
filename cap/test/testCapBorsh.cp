import { test, expect_eq } from "cap:test";
import {
    BytesSpan,
    BorshWriter,
    BorshReader,
} from "../../packages/borsh/src/index.cp";

fn sliceEq(buf: []const u8, start: usize, len: usize, expected: string): bool {
    if (len != expected.len || start + len > buf.len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        if (buf[start + i] != expected[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/borsh — all LE integer types round-trip including negatives", () => {
        let buf: [64]u8 = [0; 64];
        let w: BorshWriter = BorshWriter.new();
        expect_eq(w.writeU8(buf[0..64], 0xAB), true);
        expect_eq(w.writeU16Le(buf[0..64], 0x1234), true);
        expect_eq(w.writeU32Le(buf[0..64], 0xDEADBEEF), true);
        let big_u64: u64 = ((0x01020304 as u64) << 32) | (0x05060708 as u64);
        expect_eq(w.writeU64Le(buf[0..64], big_u64), true);
        expect_eq(w.writeI32Le(buf[0..64], -123456), true);
        expect_eq(w.writeI64Le(buf[0..64], -9876543210), true);

        let r: BorshReader = BorshReader.new();
        expect_eq(r.readU8(buf[0..w.pos]), 0xAB as u8);
        expect_eq(r.readU16Le(buf[0..w.pos]), 0x1234 as u16);
        expect_eq(r.readU32Le(buf[0..w.pos]), 0xDEADBEEF as u32);
        expect_eq(r.readU64Le(buf[0..w.pos]), big_u64);
        expect_eq(r.readI32Le(buf[0..w.pos]), -123456 as i32);
        expect_eq(r.readI64Le(buf[0..w.pos]), -9876543210 as i64);
        expect_eq(r.ok, true);
        expect_eq(r.remaining(buf[0..w.pos]), 0 as usize);
    });

    test("2. @cap/borsh — writeString & readBytesSpan 4-byte LE length prefix", () => {
        let buf: [64]u8 = [0; 64];
        let w: BorshWriter = BorshWriter.new();
        expect_eq(w.writeString(buf[0..64], "hello"), true);
        // Canonical Borsh bytes: [5, 0, 0, 0, 'h', 'e', 'l', 'l', 'o']
        expect_eq(w.pos, 9 as usize);
        expect_eq(buf[0], 5 as u8);
        expect_eq(buf[1], 0 as u8);
        expect_eq(buf[2], 0 as u8);
        expect_eq(buf[3], 0 as u8);

        let r: BorshReader = BorshReader.new();
        let sp: BytesSpan = r.readBytesSpan(buf[0..w.pos]);
        expect_eq(sp.ok, true);
        expect_eq(sliceEq(buf[0..w.pos], sp.start, sp.len, "hello"), true);
        expect_eq(r.ok, true);
    });

    test("3. @cap/borsh — multi-field composite record round-trip", () => {
        let buf: [128]u8 = [0; 128];
        let w: BorshWriter = BorshWriter.new();
        expect_eq(w.writeU64Le(buf[0..128], 42), true);
        expect_eq(w.writeString(buf[0..128], "alice.near"), true);
        expect_eq(w.writeBool(buf[0..128], true), true);
        expect_eq(w.writeI32Le(buf[0..128], -7), true);

        let r: BorshReader = BorshReader.new();
        expect_eq(r.readU64Le(buf[0..w.pos]), 42 as u64);
        let name: BytesSpan = r.readBytesSpan(buf[0..w.pos]);
        expect_eq(name.ok, true);
        expect_eq(sliceEq(buf[0..w.pos], name.start, name.len, "alice.near"), true);
        expect_eq(r.readBool(buf[0..w.pos]), true);
        expect_eq(r.readI32Le(buf[0..w.pos]), -7 as i32);
        expect_eq(r.ok, true);
        expect_eq(r.remaining(buf[0..w.pos]), 0 as usize);
    });

    test("4. @cap/borsh — writeOptionTag & readOptionTag Some/None", () => {
        let buf: [32]u8 = [0; 32];
        let w: BorshWriter = BorshWriter.new();
        expect_eq(w.writeOptionTag(buf[0..32], false), true);
        expect_eq(w.writeOptionTag(buf[0..32], true), true);
        expect_eq(w.writeU32Le(buf[0..32], 999), true);

        let r: BorshReader = BorshReader.new();
        expect_eq(r.readOptionTag(buf[0..w.pos]), false);
        expect_eq(r.readOptionTag(buf[0..w.pos]), true);
        expect_eq(r.readU32Le(buf[0..w.pos]), 999 as u32);
        expect_eq(r.ok, true);
    });

    test("5. @cap/borsh — readBool strictly rejects invalid byte value 2", () => {
        let bad: [2]u8 = [2, 1];
        let r: BorshReader = BorshReader.new();
        let b: bool = r.readBool(bad[0..2]);
        expect_eq(b, false);
        expect_eq(r.ok, false);
    });

    test("6. @cap/borsh — truncated buffer & write overflow boundary safety", () => {
        // Length prefix claims 100 bytes but only 2 bytes follow
        let trunc: [6]u8 = [100, 0, 0, 0, 65, 66];
        let r: BorshReader = BorshReader.new();
        let sp: BytesSpan = r.readBytesSpan(trunc[0..6]);
        expect_eq(sp.ok, false);
        expect_eq(r.ok, false);

        // Writer rejects write when buffer too small without advancing pos
        let tiny: [5]u8 = [0; 5];
        let w: BorshWriter = BorshWriter.new();
        expect_eq(w.writeU64Le(tiny[0..5], 1), false);
        expect_eq(w.pos, 0 as usize);
        expect_eq(w.writeString(tiny[0..5], "ab"), false); // needs 4 + 2 = 6 bytes
        expect_eq(w.pos, 0 as usize);
        expect_eq(w.writeString(tiny[0..5], "a"), true);   // needs 4 + 1 = 5 bytes
        expect_eq(w.pos, 5 as usize);
    });
}
