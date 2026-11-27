// cap/test/testCapBitstream.cp
// Comprehensive unit tests for @cap/bitstream (Rust bitstream-io parity)

import { test, expect_eq } from "cap:test";
import { BitReader, BitWriter } from "../../packages/bitstream-io/src/index.cp";

fn main() {
    test("1. @cap/bitstream — MSB-first unaligned cross-byte read/write round-trip", () => {
        let buf: [16]u8 = undefined;
        let w = BitWriter.initMsb(buf[0..16]);

        expect_eq(w.writeBits(0b101 as u64, 3 as u8), true);
        expect_eq(w.writeBits(0x1AB as u64, 9 as u8), true);
        expect_eq(w.writeBit(1 as u8), true);
        expect_eq(w.writeBits(0x3F as u64, 6 as u8), true);
        expect_eq(w.bitsWritten(), 19 as usize);
        expect_eq(w.bytesUsed(), 3 as usize);
        expect_eq(w.alignToByte(), true);
        expect_eq(w.bitsWritten(), 24 as usize);

        let r = BitReader.initMsb(buf[0..3]);
        expect_eq(r.bitsRemaining(), 24 as usize);
        const f1 = r.readBits(3 as u8);
        expect_eq(f1.ok, true);
        expect_eq(f1.value, 0b101 as u64);

        const f2 = r.readBits(9 as u8);
        expect_eq(f2.ok, true);
        expect_eq(f2.value, 0x1AB as u64);

        const f3 = r.readBit();
        expect_eq(f3.ok, true);
        expect_eq(f3.value, 1 as u64);

        const f4 = r.readBits(6 as u8);
        expect_eq(f4.ok, true);
        expect_eq(f4.value, 0x3F as u64);
    });

    test("2. @cap/bitstream — LSB-first unaligned cross-byte read/write round-trip", () => {
        let buf: [16]u8 = undefined;
        let w = BitWriter.initLsb(buf[0..16]);

        expect_eq(w.writeBits(0b1101 as u64, 4 as u8), true);
        expect_eq(w.writeBits(0x345 as u64, 10 as u8), true);
        expect_eq(w.writeBits(0b10110 as u64, 5 as u8), true);
        expect_eq(w.alignToByte(), true);

        let r = BitReader.initLsb(buf[0..w.bytesUsed()]);
        expect_eq(r.readBits(4 as u8).value, 0b1101 as u64);
        expect_eq(r.readBits(10 as u8).value, 0x345 as u64);
        expect_eq(r.readBits(5 as u8).value, 0b10110 as u64);
    });

    test("3. @cap/bitstream — peekBits preserves cursor, skipBits & alignToByte", () => {
        let data: [3]u8 = [0xA5, 0x3C, 0xFF];
        let r = BitReader.initMsb(data[0..3]);

        // Peek top 4 bits (0xA) twice without moving cursor
        const p1 = r.peekBits(4 as u8);
        expect_eq(p1.ok, true);
        expect_eq(p1.value, 0xA as u64);
        expect_eq(r.bitPosition(), 0 as usize);

        expect_eq(r.skipBits(4 as usize), true);
        expect_eq(r.bitPosition(), 4 as usize);
        expect_eq(r.readBits(4 as u8).value, 0x5 as u64);

        // Read 3 bits then alignToByte -> jumps to bit 16 (byte 2 = 0xFF)
        * = r.readBits(3 as u8);
        expect_eq(r.bitPosition(), 11 as usize);
        r.alignToByte();
        expect_eq(r.bitPosition(), 16 as usize);
        expect_eq(r.readBits(8 as u8).value, 0xFF as u64);
    });

    test("4. @cap/bitstream — full 64-bit integer write/read across unaligned offset", () => {
        const val64: u64 = ((0xDEADBEEF as u64) << 32) | (0xCAFEBABE as u64);
        let buf: [16]u8 = undefined;

        let w = BitWriter.initMsb(buf[0..16]);
        expect_eq(w.writeBits(0b101 as u64, 3 as u8), true);
        expect_eq(w.writeBits(val64, 64 as u8), true);
        expect_eq(w.bitsWritten(), 67 as usize);

        let r = BitReader.initMsb(buf[0..w.bytesUsed()]);
        expect_eq(r.readBits(3 as u8).value, 0b101 as u64);
        const got = r.readBits(64 as u8);
        expect_eq(got.ok, true);
        expect_eq(got.value, val64);
    });

    test("5. @cap/bitstream — Exp-Golomb ue(v) round-trip across 0..1000", () => {
        let buf: [4096]u8 = undefined;
        let w = BitWriter.initMsb(buf[0..4096]);

        let v: u64 = 0;
        while (v <= 1000) {
            expect_eq(w.writeUe(v), true);
            v += 1;
        }
        expect_eq(w.alignToByte(), true);

        let r = BitReader.initMsb(buf[0..w.bytesUsed()]);
        v = 0;
        while (v <= 1000) {
            const dec_v = r.readUe();
            expect_eq(dec_v.ok, true);
            expect_eq(dec_v.value, v);
            v += 1;
        }
    });

    test("6. @cap/bitstream — buffer exhaustion & invalid bit-width (> 64) guards", () => {
        let tiny: [1]u8 = undefined;
        let w = BitWriter.initMsb(tiny[0..1]);
        expect_eq(w.writeBits(0 as u64, 65 as u8), false);
        expect_eq(w.writeBits(0xFF as u64, 8 as u8), true);
        expect_eq(w.writeBit(1 as u8), false); // buffer full

        let r = BitReader.initMsb(tiny[0..1]);
        expect_eq(r.readBits(65 as u8).ok, false);
        expect_eq(r.readBits(9 as u8).ok, false);
        expect_eq(r.skipBits(9 as usize), false);
        expect_eq(r.readBits(0 as u8).ok, true);
        expect_eq(r.readBits(8 as u8).value, 0xFF as u64);
        expect_eq(r.readBit().ok, false);
    });
}
