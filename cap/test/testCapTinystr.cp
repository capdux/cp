// cap/test/testCapTinystr.cp
// Comprehensive unit tests for @cap/tinystr

import { test, expect_eq } from "cap:test";
import {
    TINYSTR_OK,
    TINYSTR_ERR_TOO_LONG,
    TINYSTR_ERR_NON_ASCII,
    TINYSTR_ERR_CONTAINS_NUL,
    TinyAsciiStr4,
    TinyAsciiStr8,
    TinyAsciiStr16,
} from "../../packages/@cap/tinystr/src/index.cp";

fn tsTestEqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/tinystr — TinyAsciiStr4 construction, packed u32 round-trip & BCP-47 tags", () => {
        const en = TinyAsciiStr4.fromStr("en");
        expect_eq(en.ok, true);
        expect_eq(en.err, TINYSTR_OK);
        expect_eq(en.value.len(), 2 as usize);
        expect_eq(en.value.isEmpty(), false);
        expect_eq(en.value.byteAt(0), 'e' as u8);
        expect_eq(en.value.byteAt(1), 'n' as u8);
        expect_eq(en.value.byteAt(2), 0 as u8);

        const packed_en: u32 = en.value.toPackedU32();
        const unpacked_en = TinyAsciiStr4.fromPackedU32(packed_en);
        expect_eq(unpacked_en.ok, true);
        expect_eq(unpacked_en.value.eq(en.value), true);

        const us = TinyAsciiStr4.fromStr("US").value;
        expect_eq(us.len(), 2 as usize);
        expect_eq(us.isAsciiUppercase(), true);

        const latn: TinyAsciiStr4 = TinyAsciiStr4.fromStr("latn").value.toAsciiTitlecase();
        expect_eq(latn.len(), 4 as usize);
        let out: [8]u8 = [0; 8];
        const written = latn.asBytes(out[0..8]);
        expect_eq(written, 4 as usize);
        expect_eq(tsTestEqBytes(out[0..4], "Latn"), true);

        const packed_latn: u32 = latn.toPackedU32();
        const roundtrip = TinyAsciiStr4.fromPackedU32(packed_latn);
        expect_eq(roundtrip.ok, true);
        expect_eq(roundtrip.value.eq(latn), true);

        // Empty string
        const emp = TinyAsciiStr4.fromStr("").value;
        expect_eq(emp.isEmpty(), true);
        expect_eq(emp.len(), 0 as usize);
        expect_eq(emp.toPackedU32(), 0 as u32);
    });

    test("2. @cap/tinystr — TinyAsciiStr8 packed u64, lowercase, uppercase & titlecase", () => {
        const s = TinyAsciiStr8.fromStr("buddhist");
        expect_eq(s.ok, true);
        expect_eq(s.value.len(), 8 as usize);

        const packed_val: u64 = s.value.toPackedU64();
        const back = TinyAsciiStr8.fromPackedU64(packed_val);
        expect_eq(back.ok, true);
        expect_eq(back.value.eq(s.value), true);

        const upper: TinyAsciiStr8 = s.value.toAsciiUppercase();
        const title: TinyAsciiStr8 = upper.toAsciiTitlecase();
        const lower: TinyAsciiStr8 = upper.toAsciiLowercase();

        let buf: [8]u8 = [0; 8];
        expect_eq(upper.asBytes(buf[0..8]), 8 as usize);
        expect_eq(tsTestEqBytes(buf[0..8], "BUDDHIST"), true);
        expect_eq(title.asBytes(buf[0..8]), 8 as usize);
        expect_eq(tsTestEqBytes(buf[0..8], "Buddhist"), true);
        expect_eq(lower.eq(s.value), true);
    });

    test("3. @cap/tinystr — TinyAsciiStr16 long tags & lexicographical cmp", () => {
        const a = TinyAsciiStr16.fromStr("en-US-posix").value;
        const b = TinyAsciiStr16.fromStr("en-US-u-ca-iso86").value;
        const c = TinyAsciiStr16.fromStr("zh-Hans-CN").value;

        expect_eq(a.len(), 11 as usize);
        expect_eq(b.len(), 16 as usize);
        expect_eq(c.len(), 10 as usize);

        expect_eq(a.cmp(a), 0 as i32);
        expect_eq(a.cmp(b) < 0, true);
        expect_eq(b.cmp(a) > 0, true);
        expect_eq(b.cmp(c) < 0, true);
        expect_eq(c.cmp(a) > 0, true);

        // Prefix comparison ("abc" < "abcd")
        const p1 = TinyAsciiStr16.fromStr("abc").value;
        const p2 = TinyAsciiStr16.fromStr("abcd").value;
        expect_eq(p1.cmp(p2), -1 as i32);
        expect_eq(p2.cmp(p1), 1 as i32);
    });

    test("4. @cap/tinystr — All ASCII character class predicates", () => {
        const alpha = TinyAsciiStr8.fromStr("Hans").value;
        expect_eq(alpha.isAsciiAlphabetic(), true);
        expect_eq(alpha.isAsciiAlphanumeric(), true);
        expect_eq(alpha.isAsciiNumeric(), false);
        expect_eq(alpha.isAsciiLowercase(), false);
        expect_eq(alpha.isAsciiUppercase(), false);

        const lower = TinyAsciiStr4.fromStr("zh").value;
        expect_eq(lower.isAsciiAlphabetic(), true);
        expect_eq(lower.isAsciiLowercase(), true);
        expect_eq(lower.isAsciiUppercase(), false);

        const upper = TinyAsciiStr4.fromStr("USD").value;
        expect_eq(upper.isAsciiAlphabetic(), true);
        expect_eq(upper.isAsciiUppercase(), true);
        expect_eq(upper.isAsciiLowercase(), false);

        const digits = TinyAsciiStr4.fromStr("419").value;
        expect_eq(digits.isAsciiNumeric(), true);
        expect_eq(digits.isAsciiAlphanumeric(), true);
        expect_eq(digits.isAsciiAlphabetic(), false);

        const alnum = TinyAsciiStr8.fromStr("iso8601").value;
        expect_eq(alnum.isAsciiAlphanumeric(), true);
        expect_eq(alnum.isAsciiAlphabetic(), false);
        expect_eq(alnum.isAsciiNumeric(), false);

        const hyphen = TinyAsciiStr16.fromStr("zh-CN").value;
        expect_eq(hyphen.isAsciiAlphanumeric(), false);
    });

    test("5. @cap/tinystr — eqIgnoreAsciiCase across TinyAsciiStr4/8/16", () => {
        const l4 = TinyAsciiStr4.fromStr("Latn").value;
        const u4 = TinyAsciiStr4.fromStr("LATN").value;
        const d4 = TinyAsciiStr4.fromStr("Cyrl").value;
        expect_eq(l4.eqIgnoreAsciiCase(u4), true);
        expect_eq(l4.eq(u4), false);
        expect_eq(l4.eqIgnoreAsciiCase(d4), false);

        const l8 = TinyAsciiStr8.fromStr("Gregory").value;
        const u8 = TinyAsciiStr8.fromStr("gregory").value;
        expect_eq(l8.eqIgnoreAsciiCase(u8), true);

        const l16 = TinyAsciiStr16.fromStr("zh-Hans-CN").value;
        const u16 = TinyAsciiStr16.fromStr("ZH-HANS-CN").value;
        expect_eq(l16.eqIgnoreAsciiCase(u16), true);
    });

    test("6. @cap/tinystr — Strict rejection of too-long, NUL bytes & non-ASCII UTF-8", () => {
        // Too long
        const long4 = TinyAsciiStr4.fromStr("abcde");
        expect_eq(long4.ok, false);
        expect_eq(long4.err, TINYSTR_ERR_TOO_LONG);

        const long8 = TinyAsciiStr8.fromStr("123456789");
        expect_eq(long8.ok, false);
        expect_eq(long8.err, TINYSTR_ERR_TOO_LONG);

        const long16 = TinyAsciiStr16.fromStr("0123456789abcdefX");
        expect_eq(long16.ok, false);
        expect_eq(long16.err, TINYSTR_ERR_TOO_LONG);

        // Embedded NUL byte
        let nul_bytes: [3]u8 = ['a' as u8, 0 as u8, 'b' as u8];
        const nul4 = TinyAsciiStr4.fromBytes(nul_bytes[0..3]);
        expect_eq(nul4.ok, false);
        expect_eq(nul4.err, TINYSTR_ERR_CONTAINS_NUL);

        // Packed u32 with embedded NUL (0x00 in byte 1, 'b' in byte 2)
        const bad_pack4 = TinyAsciiStr4.fromPackedU32(0x00620061 as u32);
        expect_eq(bad_pack4.ok, false);
        expect_eq(bad_pack4.err, TINYSTR_ERR_CONTAINS_NUL);

        // Non-ASCII UTF-8 byte (>= 0x80)
        let utf8_bytes: [2]u8 = [0xC3 as u8, 0xA9 as u8]; // 'é'
        const non_ascii4 = TinyAsciiStr4.fromBytes(utf8_bytes[0..2]);
        expect_eq(non_ascii4.ok, false);
        expect_eq(non_ascii4.err, TINYSTR_ERR_NON_ASCII);

        const non_ascii8 = TinyAsciiStr8.fromBytes(utf8_bytes[0..2]);
        expect_eq(non_ascii8.ok, false);
        expect_eq(non_ascii8.err, TINYSTR_ERR_NON_ASCII);

        const bad_pack8 = TinyAsciiStr8.fromPackedU64(0x0000000000008061 as u64);
        expect_eq(bad_pack8.ok, false);
        expect_eq(bad_pack8.err, TINYSTR_ERR_NON_ASCII);
    });
}
