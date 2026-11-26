// cap/test/testCapBumpalo.cp
// Comprehensive unit tests for @cap/bumpalo

import { test, expect_eq } from "cap:test";
import {
    BUMP_CAPACITY,
    BumpSlice,
    BumpArena,
} from "../../packages/@cap/bumpalo/src/index.cp";

fn main() {
    test("1. @cap/bumpalo — initial state, allocBytes, readByte & writeByte", () => {
        let bump = BumpArena.new();
        expect_eq(bump.capacity(), BUMP_CAPACITY);
        expect_eq(bump.capacity(), 4096 as usize);
        expect_eq(bump.allocatedBytes(), 0 as usize);
        expect_eq(bump.remaining(), 4096 as usize);
        expect_eq(bump.allocationCount(), 0 as usize);

        let h = bump.allocBytes(5 as usize) ?? BumpSlice.empty();
        expect_eq(h.offset, 0 as usize);
        expect_eq(h.len, 5 as usize);
        expect_eq(bump.allocatedBytes(), 5 as usize);
        expect_eq(bump.remaining(), 4091 as usize);
        expect_eq(bump.allocationCount(), 1 as usize);

        // Freshly allocated memory is zeroed.
        expect_eq(bump.readByte(h, 0 as usize) ?? (99 as u8), 0 as u8);
        expect_eq(bump.readByte(h, 4 as usize) ?? (99 as u8), 0 as u8);
        expect_eq(bump.readByte(h, 5 as usize) == null, true);

        expect_eq(bump.writeByte(h, 2 as usize, 0xAB as u8), true);
        expect_eq(bump.writeByte(h, 5 as usize, 0xFF as u8), false);
        expect_eq(bump.readByte(h, 2 as usize) ?? (0 as u8), 0xAB as u8);
    });

    test("2. @cap/bumpalo — power-of-two alignment (1..64) & invalid alignment rejection", () => {
        let bump = BumpArena.new();
        // Bump by 1 byte so cursor = 1
        let a1 = bump.allocAligned(1 as usize, 1 as usize) ?? BumpSlice.empty();
        expect_eq(a1.offset, 0 as usize);

        // Align to 4 -> offset 4, end 7
        let a4 = bump.allocAligned(3 as usize, 4 as usize) ?? BumpSlice.empty();
        expect_eq(a4.offset, 4 as usize);
        expect_eq(a4.end(), 7 as usize);

        // Align to 8 -> offset 8, end 9
        let a8 = bump.allocAligned(1 as usize, 8 as usize) ?? BumpSlice.empty();
        expect_eq(a8.offset, 8 as usize);

        // Align to 16 -> offset 16, end 17
        let a16 = bump.allocAligned(1 as usize, 16 as usize) ?? BumpSlice.empty();
        expect_eq(a16.offset, 16 as usize);

        // Align to 32 -> offset 32, end 33
        let a32 = bump.allocAligned(1 as usize, 32 as usize) ?? BumpSlice.empty();
        expect_eq(a32.offset, 32 as usize);

        // Align to 64 -> offset 64, end 72
        let a64 = bump.allocAligned(8 as usize, 64 as usize) ?? BumpSlice.empty();
        expect_eq(a64.offset, 64 as usize);
        expect_eq(bump.allocatedBytes(), 72 as usize);

        // Invalid alignments: 0, 3, 6, 128 must fail without moving cursor.
        expect_eq(bump.allocAligned(4 as usize, 0 as usize) == null, true);
        expect_eq(bump.allocAligned(4 as usize, 3 as usize) == null, true);
        expect_eq(bump.allocAligned(4 as usize, 6 as usize) == null, true);
        expect_eq(bump.allocAligned(4 as usize, 128 as usize) == null, true);
        expect_eq(bump.allocatedBytes(), 72 as usize);
    });

    test("3. @cap/bumpalo — allocStr, matchesStr & allocSliceCopy", () => {
        let bump = BumpArena.new();
        let s1 = bump.allocStr("hello") ?? BumpSlice.empty();
        let s2 = bump.allocStr("bumpalo") ?? BumpSlice.empty();
        expect_eq(bump.matchesStr(s1, "hello"), true);
        expect_eq(bump.matchesStr(s1, "hell"), false);
        expect_eq(bump.matchesStr(s1, "hellx"), false);
        expect_eq(bump.matchesStr(s2, "bumpalo"), true);

        let raw: [4]u8 = undefined;
        raw[0] = 10 as u8;
        raw[1] = 20 as u8;
        raw[2] = 30 as u8;
        raw[3] = 40 as u8;
        let cp = bump.allocSliceCopy(raw[0..4]) ?? BumpSlice.empty();
        expect_eq(cp.len, 4 as usize);
        expect_eq(bump.readByte(cp, 0 as usize) ?? (0 as u8), 10 as u8);
        expect_eq(bump.readByte(cp, 3 as usize) ?? (0 as u8), 40 as u8);
        expect_eq(bump.allocationCount(), 3 as usize);
    });

    test("4. @cap/bumpalo — checkpoint & rewind rollback", () => {
        let bump = BumpArena.new();
        let base = bump.allocStr("keep") ?? BumpSlice.empty();
        let mark = bump.checkpoint();
        expect_eq(mark, 4 as usize);

        let temp = bump.allocStr("transient-payload") ?? BumpSlice.empty();
        expect_eq(bump.matchesStr(temp, "transient-payload"), true);
        expect_eq(bump.allocatedBytes(), 21 as usize);

        // Cannot rewind forward past cursor.
        expect_eq(bump.rewind(100 as usize), false);
        expect_eq(bump.rewind(mark), true);
        expect_eq(bump.allocatedBytes(), 4 as usize);
        expect_eq(bump.remaining(), 4092 as usize);

        // Base slice remains intact; rolled-back slice is now invalid.
        expect_eq(bump.matchesStr(base, "keep"), true);
        expect_eq(bump.matchesStr(temp, "transient-payload"), false);
        expect_eq(bump.readByte(temp, 0 as usize) == null, true);

        // Next allocation reuses memory right at mark and zeroes it.
        let reused = bump.allocBytes(4 as usize) ?? BumpSlice.empty();
        expect_eq(reused.offset, 4 as usize);
        expect_eq(bump.readByte(reused, 0 as usize) ?? (99 as u8), 0 as u8);
    });

    test("5. @cap/bumpalo — capacity exhaustion & overflow protection", () => {
        let bump = BumpArena.new();
        let big = bump.allocBytes(4090 as usize) ?? BumpSlice.empty();
        expect_eq(big.len, 4090 as usize);
        expect_eq(bump.remaining(), 6 as usize);

        // Alignment padding would push past 4096 (4090 -> aligned 64 = 4096, size 1 fails).
        expect_eq(bump.allocAligned(1 as usize, 64 as usize) == null, true);
        // Oversized request fails safely without wraparound.
        expect_eq(bump.allocBytes(7 as usize) == null, true);
        expect_eq(bump.allocStr("1234567") == null, true);

        // Exact fit succeeds.
        let tail = bump.allocBytes(6 as usize) ?? BumpSlice.empty();
        expect_eq(tail.offset, 4090 as usize);
        expect_eq(bump.remaining(), 0 as usize);
        expect_eq(bump.allocBytes(1 as usize) == null, true);
    });

    test("6. @cap/bumpalo — reset clears cursor, count & invalidates old handles", () => {
        let bump = BumpArena.new();
        let h1 = bump.allocStr("frame-1") ?? BumpSlice.empty();
        expect_eq(bump.writeByte(h1, 0 as usize, 0x41 as u8), true);
        expect_eq(bump.allocationCount(), 1 as usize);

        bump.reset();
        expect_eq(bump.allocatedBytes(), 0 as usize);
        expect_eq(bump.remaining(), 4096 as usize);
        expect_eq(bump.allocationCount(), 0 as usize);
        expect_eq(bump.readByte(h1, 0 as usize) == null, true);

        // Fresh allocation starts at 0 and zeroes previous dirty bytes.
        let h2 = bump.allocBytes(4 as usize) ?? BumpSlice.empty();
        expect_eq(h2.offset, 0 as usize);
        expect_eq(bump.readByte(h2, 0 as usize) ?? (99 as u8), 0 as u8);
        expect_eq(bump.allocationCount(), 1 as usize);
    });
}
