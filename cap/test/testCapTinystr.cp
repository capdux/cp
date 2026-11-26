// cap/test/testCapTinystr.cp
// Comprehensive unit tests for @cap/tinystr

import { test, expect_eq } from "cap:test";
import {
    TinyStr4,
    TinyStr8,
} from "../../packages/@cap/tinystr/src/index.cp";

fn main() {
    test("1. @cap/tinystr — TinyStr4 packing, round-trip & single-instruction equals", () => {
        const usdOpt = TinyStr4.fromStr("USD");
        expect_eq(usdOpt != null, true);
        const usd = usdOpt ?? TinyStr4{ raw: 0 as u32 };
        expect_eq(usd.len(), 3 as usize);

        const usd2 = TinyStr4.fromStr("USD") ?? TinyStr4{ raw: 0 as u32 };
        const eur = TinyStr4.fromStr("EUR") ?? TinyStr4{ raw: 0 as u32 };
        expect_eq(usd.equals(usd2), true);
        expect_eq(usd.equals(eur), false);

        let buf: [8]u8 = undefined;
        expect_eq(usd.writeTo(buf[0..8]), "USD");

        const av01 = TinyStr4.fromStr("av01") ?? TinyStr4{ raw: 0 as u32 };
        expect_eq(av01.len(), 4 as usize);
        let buf2: [8]u8 = undefined;
        expect_eq(av01.writeTo(buf2[0..8]), "av01");
    });

    test("2. @cap/tinystr — TinyStr4 case conversion (lower, upper, title)", () => {
        const s = TinyStr4.fromStr("hAnS") ?? TinyStr4{ raw: 0 as u32 };
        let b1: [8]u8 = undefined;
        let b2: [8]u8 = undefined;
        let b3: [8]u8 = undefined;
        expect_eq(s.toAsciiLower().writeTo(b1[0..8]), "hans");
        expect_eq(s.toAsciiUpper().writeTo(b2[0..8]), "HANS");
        expect_eq(s.toAsciiTitle().writeTo(b3[0..8]), "Hans");
    });

    test("3. @cap/tinystr — TinyStr4 character class predicates", () => {
        const alpha = TinyStr4.fromStr("zh") ?? TinyStr4{ raw: 0 as u32 };
        expect_eq(alpha.isAsciiAlpha(), true);
        expect_eq(alpha.isAsciiNumeric(), false);
        expect_eq(alpha.isAsciiAlphanumeric(), true);

        const num = TinyStr4.fromStr("2026") ?? TinyStr4{ raw: 0 as u32 };
        expect_eq(num.isAsciiAlpha(), false);
        expect_eq(num.isAsciiNumeric(), true);
        expect_eq(num.isAsciiAlphanumeric(), true);

        const alnum = TinyStr4.fromStr("mp4a") ?? TinyStr4{ raw: 0 as u32 };
        expect_eq(alnum.isAsciiAlpha(), false);
        expect_eq(alnum.isAsciiNumeric(), false);
        expect_eq(alnum.isAsciiAlphanumeric(), true);

        const sym = TinyStr4.fromStr("a-b") ?? TinyStr4{ raw: 0 as u32 };
        expect_eq(sym.isAsciiAlphanumeric(), false);
    });

    test("4. @cap/tinystr — TinyStr8 packing, equals & case conversion", () => {
        const opt = TinyStr8.fromStr("OPTIONS") ?? TinyStr8{ raw: 0 as u64 };
        expect_eq(opt.len(), 7 as usize);
        let b1: [16]u8 = undefined;
        expect_eq(opt.writeTo(b1[0..16]), "OPTIONS");

        let b2: [16]u8 = undefined;
        expect_eq(opt.toAsciiLower().writeTo(b2[0..16]), "options");

        let b3: [16]u8 = undefined;
        expect_eq(opt.toAsciiTitle().writeTo(b3[0..16]), "Options");

        const full8 = TinyStr8.fromStr("http2026") ?? TinyStr8{ raw: 0 as u64 };
        expect_eq(full8.len(), 8 as usize);
        expect_eq(full8.isAsciiAlphanumeric(), true);
        expect_eq(full8.isAsciiAlpha(), false);
    });

    test("5. @cap/tinystr — TinyStr8 character predicates & upper conversion", () => {
        const proto = TinyStr8.fromStr("https") ?? TinyStr8{ raw: 0 as u64 };
        expect_eq(proto.isAsciiAlpha(), true);
        expect_eq(proto.isAsciiNumeric(), false);
        expect_eq(proto.isAsciiAlphanumeric(), true);

        let b1: [16]u8 = undefined;
        expect_eq(proto.toAsciiUpper().writeTo(b1[0..16]), "HTTPS");

        const code = TinyStr8.fromStr("12345678") ?? TinyStr8{ raw: 0 as u64 };
        expect_eq(code.isAsciiNumeric(), true);
        expect_eq(code.isAsciiAlphanumeric(), true);
    });

    test("6. @cap/tinystr — Reject empty, over-length, non-ASCII & NUL bytes", () => {
        expect_eq(TinyStr4.fromStr("") == null, true);
        expect_eq(TinyStr4.fromStr("12345") == null, true);
        expect_eq(TinyStr4.fromStr("中文") == null, true);

        let nul4: [3]u8 = undefined;
        nul4[0] = 'a';
        nul4[1] = 0 as u8;
        nul4[2] = 'b';
        expect_eq(TinyStr4.fromStr(nul4[0..3]) == null, true);

        expect_eq(TinyStr8.fromStr("") == null, true);
        expect_eq(TinyStr8.fromStr("123456789") == null, true);
        expect_eq(TinyStr8.fromStr("café") == null, true);

        let nul8: [4]u8 = undefined;
        nul8[0] = 'h';
        nul8[1] = 'i';
        nul8[2] = 0 as u8;
        nul8[3] = '!';
        expect_eq(TinyStr8.fromStr(nul8[0..4]) == null, true);
    });
}
