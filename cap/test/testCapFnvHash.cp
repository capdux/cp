import { test, expect_eq } from "cap:test";
import {
    FNV32_OFFSET_BASIS,
    fnv64OffsetBasis,
    fnv1a32,
    fnv1a32Update,
    fnv132,
    fnv1a64,
    fnv1a64Update,
    fnv164,
    formatHex64,
} from "../../packages/@cap/fnv-hash/src/index.cp";

fn eqStr(a: []const u8, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/fnv-hash — IETF official FNV-1a 32-bit test vectors", () => {
        expect_eq(fnv1a32(""), 0x811c9dc5);
        expect_eq(fnv1a32("a"), 0xe40c292c);
        expect_eq(fnv1a32("foobar"), 0xbf9cf968);
    });

    test("2. @cap/fnv-hash — IETF official FNV-1a 64-bit test vectors", () => {
        expect_eq(fnv1a64(""), fnv64OffsetBasis());
        let expected_a: u64 = ((0xaf63dc4c as u64) << 32) | (0x8601ec8c as u64);
        let expected_foobar: u64 = ((0x85944171 as u64) << 32) | (0xf73967e8 as u64);
        expect_eq(fnv1a64("a"), expected_a);
        expect_eq(fnv1a64("foobar"), expected_foobar);
    });

    test("3. @cap/fnv-hash — IETF official FNV-1 32-bit & 64-bit test vectors", () => {
        expect_eq(fnv132("foobar"), 0x31f0b262);
        let expected_f1_64: u64 = ((0x340d8765 as u64) << 32) | (0xa4dda9c2 as u64);
        expect_eq(fnv164("foobar"), expected_f1_64);
    });

    test("4. @cap/fnv-hash — streaming multi-chunk update equals one-shot", () => {
        let s32: u32 = fnv1a32Update(FNV32_OFFSET_BASIS, "foo");
        s32 = fnv1a32Update(s32, "bar");
        expect_eq(s32, fnv1a32("foobar"));

        let s64: u64 = fnv1a64Update(fnv64OffsetBasis(), "foo");
        s64 = fnv1a64Update(s64, "bar");
        expect_eq(s64, fnv1a64("foobar"));
    });

    test("5. @cap/fnv-hash — formatHex64 output", () => {
        let buf: [16]u8 = undefined;
        let n: usize = formatHex64(fnv1a64("foobar"), buf[0..]);
        expect_eq(n, 16);
        expect_eq(eqStr(buf[0..16], "85944171f73967e8"), true);
    });

    test("6. @cap/fnv-hash — small buffer protection in formatHex64", () => {
        let tiny: [8]u8 = undefined;
        expect_eq(formatHex64(fnv1a64("a"), tiny[0..]), 0);
    });
}
