import { test, expect_eq } from "cap:test";
import {
    SMOL_INLINE_CAP,
    SmolStr,
    smolStartsWith,
    smolEndsWith
} from "../../packages/@cap/smol-str/src/index.cp";

fn main() {
    test("1. @cap/smol-str — empty and 24-byte inline boundary SSO", () => {
        expect_eq(SMOL_INLINE_CAP, 24);

        let e = SmolStr.empty();
        expect_eq(e.len(), 0);
        expect_eq(e.isEmpty(), true);
        expect_eq(e.isInline(), true);
        expect_eq(e.byteAt(0) == null, true);

        // Exactly 24 bytes stays inline
        let s24 = SmolStr.new("0123456789abcdef01234567");
        expect_eq(s24.len(), 24);
        expect_eq(s24.isInline(), true);
        expect_eq(s24.byteAt(0) ?? 0, 48);
        expect_eq(s24.byteAt(23) ?? 0, 55);
        expect_eq(s24.byteAt(24) == null, true);

        // 25 bytes falls back to zero-copy external slice view
        let s25 = SmolStr.new("0123456789abcdef012345678");
        expect_eq(s25.len(), 25);
        expect_eq(s25.isInline(), false);
        expect_eq(s25.byteAt(24) ?? 0, 56);
        expect_eq(s25.byteAt(25) == null, true);
    });

    test("2. @cap/smol-str — eq, eqStr, and cmp across inline and external strings", () => {
        let a = SmolStr.new("alpha");
        let b = SmolStr.new("alpha");
        let c = SmolStr.new("alphabet");
        let long1 = SmolStr.new("this_string_is_longer_than_24_bytes!");
        let long2 = SmolStr.new("this_string_is_longer_than_24_bytes!");
        let long3 = SmolStr.new("this_string_is_longer_than_24_bytes?");

        expect_eq(a.eq(b), true);
        expect_eq(a.eqStr("alpha"), true);
        expect_eq(a.eq(c), false);
        expect_eq(a.cmp(b), 0);
        expect_eq(a.cmp(c), -1);
        expect_eq(c.cmp(a), 1);

        expect_eq(long1.eq(long2), true);
        expect_eq(long1.eqStr("this_string_is_longer_than_24_bytes!"), true);
        expect_eq(long1.cmp(long3), -1); // '!' (33) < '?' (63)
    });

    test("3. @cap/smol-str — hasPrefix/hasSuffix and smolStartsWith/smolEndsWith", () => {
        let s = SmolStr.new("cap_smol_str");
        expect_eq(s.hasPrefix("cap_"), true);
        expect_eq(smolStartsWith(s, "cap_"), true);
        expect_eq(s.hasPrefix("smol"), false);
        expect_eq(s.hasSuffix("_str"), true);
        expect_eq(smolEndsWith(s, "_str"), true);
        expect_eq(s.hasSuffix("cap_smol_str_extra"), false);

        let long_s = SmolStr.new("prefix_with_more_than_twenty_four_bytes_suffix");
        expect_eq(long_s.isInline(), false);
        expect_eq(long_s.hasPrefix("prefix_"), true);
        expect_eq(long_s.hasSuffix("_suffix"), true);
        expect_eq(long_s.hasSuffix("_wrong"), false);
    });

    test("4. @cap/smol-str — writeTo copies inline and external bytes with truncation safety", () => {
        let s = SmolStr.new("hello");
        let out: [8]u8 = [0; 8];
        let written = s.writeTo(out[0..8]);
        expect_eq(written, 5);
        expect_eq(out[0], 104); // 'h'
        expect_eq(out[4], 111); // 'o'

        // Truncated destination buffer
        let small: [3]u8 = [0; 3];
        let w2 = s.writeTo(small[0..3]);
        expect_eq(w2, 3);
        expect_eq(small[2], 108); // 'l'

        // External string writeTo
        let long_s = SmolStr.new("abcdefghijklmnopqrstuvwxyz012345");
        let out2: [32]u8 = [0; 32];
        let w3 = long_s.writeTo(out2[0..32]);
        expect_eq(w3, 32);
        expect_eq(out2[0], 97);  // 'a'
        expect_eq(out2[31], 53); // '5'
    });

    test("5. @cap/smol-str — fromInt formats zero, positive, negative, and i64::MIN inline", () => {
        let z = SmolStr.fromInt(0);
        expect_eq(z.isInline(), true);
        expect_eq(z.eqStr("0"), true);

        let pos = SmolStr.fromInt(1234567890);
        expect_eq(pos.isInline(), true);
        expect_eq(pos.eqStr("1234567890"), true);

        let neg = SmolStr.fromInt(-42);
        expect_eq(neg.isInline(), true);
        expect_eq(neg.eqStr("-42"), true);

        // i64::MIN = -9223372036854775808 (constructed safely via -9223372036854775807 - 1)
        let min_i64: i64 = -9223372036854775807 - 1;
        let min_s = SmolStr.fromInt(min_i64);
        expect_eq(min_s.isInline(), true);
        expect_eq(min_s.len(), 20);
        expect_eq(min_s.eqStr("-9223372036854775808"), true);
    });

    test("6. @cap/smol-str — O(1) value copy independence", () => {
        let orig = SmolStr.new("independent_copy");
        let copy = orig;
        expect_eq(copy.eq(orig), true);
        expect_eq(copy.isInline(), true);
        expect_eq(copy.len(), 16);
        expect_eq(copy.eqStr("independent_copy"), true);
    });
}
