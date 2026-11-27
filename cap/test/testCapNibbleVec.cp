import { test, expect_eq } from "cap:test";
import {
    NIBBLE_MAX_BYTES,
    NibbleVec,
} from "../../packages/@cap/nibble-vec/src/index.cp";

fn main() {
    test("1. @cap/nibble-vec — new vector is empty and push/get/len work", () => {
        let v = NibbleVec.new();
        expect_eq(NIBBLE_MAX_BYTES, 64);
        expect_eq(v.len(), 0);
        expect_eq(v.isEmpty(), true);
        expect_eq(v.get(0) == null, true);

        expect_eq(v.push(0xA), true);
        expect_eq(v.push(0x5), true);
        expect_eq(v.push(0x1F), true);
        expect_eq(v.len(), 3);
        expect_eq(v.isEmpty(), false);
        expect_eq(v.get(0).?, 0xA);
        expect_eq(v.get(1).?, 0x5);
        expect_eq(v.get(2).?, 0xF);
        expect_eq(v.get(3) == null, true);
    });

    test("2. @cap/nibble-vec — fromBytes unpacks high and low nibbles", () => {
        const raw: [3]u8 = [0xAB, 0xCD, 0x05];
        const v = NibbleVec.fromBytes(raw[0..]);
        expect_eq(v.len(), 6);
        expect_eq(v.get(0).?, 0xA);
        expect_eq(v.get(1).?, 0xB);
        expect_eq(v.get(2).?, 0xC);
        expect_eq(v.get(3).?, 0xD);
        expect_eq(v.get(4).?, 0x0);
        expect_eq(v.get(5).?, 0x5);
    });

    test("3. @cap/nibble-vec — set updates in-place and rejects out-of-bounds", () => {
        const raw: [2]u8 = [0x12, 0x34];
        let v = NibbleVec.fromBytes(raw[0..]);
        expect_eq(v.set(0, 0x9), true);
        expect_eq(v.set(3, 0xE), true);
        expect_eq(v.set(4, 0x1), false);
        expect_eq(v.get(0).?, 0x9);
        expect_eq(v.get(1).?, 0x2);
        expect_eq(v.get(2).?, 0x3);
        expect_eq(v.get(3).?, 0xE);
    });

    test("4. @cap/nibble-vec — pop removes nibbles in LIFO order", () => {
        let v = NibbleVec.new();
        expect_eq(v.pop() == null, true);
        expect_eq(v.push(0x4), true);
        expect_eq(v.push(0xC), true);
        expect_eq(v.push(0x7), true);
        expect_eq(v.pop().?, 0x7);
        expect_eq(v.pop().?, 0xC);
        expect_eq(v.len(), 1);
        expect_eq(v.pop().?, 0x4);
        expect_eq(v.isEmpty(), true);
        expect_eq(v.pop() == null, true);
    });

    test("5. @cap/nibble-vec — commonPrefixLen computes shared nibble prefix", () => {
        const a_bytes: [2]u8 = [0xAB, 0xC1];
        const b_bytes: [2]u8 = [0xAB, 0xC9];
        const c_bytes: [2]u8 = [0x1B, 0xC1];
        const a = NibbleVec.fromBytes(a_bytes[0..]);
        const b = NibbleVec.fromBytes(b_bytes[0..]);
        const c = NibbleVec.fromBytes(c_bytes[0..]);
        expect_eq(a.commonPrefixLen(b), 3);
        expect_eq(a.commonPrefixLen(a), 4);
        expect_eq(a.commonPrefixLen(c), 0);
    });

    test("6. @cap/nibble-vec — capacity boundary enforces 128-nibble limit", () => {
        let v = NibbleVec.new();
        let i: usize = 0;
        while (i < 128) : (i += 1) {
            expect_eq(v.push(u8(i & 0x0F)), true);
        }
        expect_eq(v.len(), 128);
        expect_eq(v.push(0x1), false);
        expect_eq(v.get(127).?, 0xF);
    });
}
