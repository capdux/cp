import { test, expect_eq } from "cap:test";
import {
    NIBBLE_MAX_BYTES,
    NibbleVec,
    Nibblet,
} from "../../packages/nibble_vec/src/index.cp";

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

    test("7. @cap/nibble-vec — fromByteSlice, Nibblet alias, asBytes, copyBytes & byteLen", () => {
        const raw: [3]u8 = [0xDE, 0xAD, 0xBE];
        let v: Nibblet = NibbleVec.fromByteSlice(raw[0..3]);
        expect_eq(v.len(), 6);
        expect_eq(v.byteLen(), 3);
        const view: []const u8 = v.asBytes();
        expect_eq(view.len, 3);
        expect_eq(view[0], 0xDE);
        expect_eq(view[2], 0xBE);

        let out: [4]u8 = [0, 0, 0, 0];
        expect_eq(v.copyBytes(out[0..4]), 3);
        expect_eq(out[1], 0xAD);
    });

    test("8. @cap/nibble-vec — split, join, extend, slice, hasPrefix, equals, truncate & clear", () => {
        const raw: [3]u8 = [0x12, 0x34, 0x56];
        let v = NibbleVec.fromByteSlice(raw[0..3]);
        const orig = NibbleVec.fromByteSlice(raw[0..3]);

        // Split at odd nibble index 3: [1,2,3] and [4,5,6]
        const tail = v.split(3);
        expect_eq(v.len(), 3);
        expect_eq(tail.len(), 3);
        expect_eq(v.get(0).?, 0x1);
        expect_eq(v.get(2).?, 0x3);
        expect_eq(tail.get(0).?, 0x4);
        expect_eq(tail.get(2).?, 0x6);

        // Join restores original vector
        const rejoined = v.join(tail);
        expect_eq(rejoined.equals(orig), true);
        expect_eq(orig.hasPrefix(v), true);

        // slice [1..5) -> [2,3,4,5]
        const sub = orig.slice(1, 5);
        expect_eq(sub.len(), 4);
        expect_eq(sub.get(0).?, 0x2);
        expect_eq(sub.get(3).?, 0x5);

        // extend, truncate & clear
        expect_eq(v.extend(tail), true);
        expect_eq(v.equals(orig), true);
        v.truncate(2);
        expect_eq(v.len(), 2);
        v.clear();
        expect_eq(v.isEmpty(), true);
    });
}
