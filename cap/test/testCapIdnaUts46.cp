// cap/test/testCapIdnaUts46.cp
// Comprehensive unit tests for @cap/idna-uts46

import { test, expect_eq } from "cap:test";
import {
    IDNA_ERR_EMPTY,
    IDNA_ERR_EMPTY_LABEL,
    IDNA_ERR_LABEL_TOO_LONG,
    IDNA_ERR_INVALID_HYPHEN,
    IDNA_ERR_INVALID_PUNYCODE,
    domainToAscii,
    domainToUnicode,
    isValidIdnaAscii,
    isPunycodeLabel,
} from "../../packages/idna/src/index.cp";

fn idnaTestEqBytes(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/idna-uts46 — Pure ASCII domain lowercasing, fast path & validation", () => {
        let out: [128]u8 = [0; 128];
        const n = domainToAscii("API.Example-Host.ORG", out[0..128]);
        expect_eq(n, 20 as i64);
        expect_eq(idnaTestEqBytes(out[0..(n as usize)], "api.example-host.org"), true);
        expect_eq(isValidIdnaAscii("api.example-host.org"), true);
        expect_eq(isValidIdnaAscii("Example.COM."), true);
        expect_eq(isPunycodeLabel("xn--mnchen-3ya"), true);
        expect_eq(isPunycodeLabel("XN--FSQU00A"), true);
        expect_eq(isPunycodeLabel("example"), false);
    });

    test("2. @cap/idna-uts46 — Official RFC 3492 / UTS #46 vectors: münchen.de & 例子.测试", () => {
        // "münchen.de": 'm', 0xC3, 0xBC, 'n','c','h','e','n','.','d','e'
        let muenchen: [11]u8 = [
            'm' as u8, 0xC3 as u8, 0xBC as u8, 'n' as u8, 'c' as u8, 'h' as u8,
            'e' as u8, 'n' as u8, '.' as u8, 'd' as u8, 'e' as u8,
        ];
        let out: [128]u8 = [0; 128];
        const n1 = domainToAscii(muenchen[0..11], out[0..128]);
        expect_eq(n1 > (0 as i64), true);
        expect_eq(idnaTestEqBytes(out[0..(n1 as usize)], "xn--mnchen-3ya.de"), true);
        expect_eq(isValidIdnaAscii(out[0..(n1 as usize)]), true);

        // "例子.测试": 例(E4 BE 8B) 子(E5 AD 90) . 测(E6 B5 8B) 试(E8 AF 95)
        let zh_dom: [13]u8 = [
            0xE4 as u8, 0xBE as u8, 0x8B as u8,
            0xE5 as u8, 0xAD as u8, 0x90 as u8,
            '.' as u8,
            0xE6 as u8, 0xB5 as u8, 0x8B as u8,
            0xE8 as u8, 0xAF as u8, 0x95 as u8,
        ];
        const n2 = domainToAscii(zh_dom[0..13], out[0..128]);
        expect_eq(n2 > (0 as i64), true);
        expect_eq(idnaTestEqBytes(out[0..(n2 as usize)], "xn--fsqu00a.xn--0zwm56d"), true);
        expect_eq(isValidIdnaAscii(out[0..(n2 as usize)]), true);
    });

    test("3. @cap/idna-uts46 — Decomposed u + U+0308 normalized via @cap/icu-normalizer NFC", () => {
        // "mu\u{0308}nchen.de": 'm','u', 0xCC, 0x88, 'n','c','h','e','n','.','d','e'
        let nfd_muenchen: [12]u8 = [
            'm' as u8, 'u' as u8, 0xCC as u8, 0x88 as u8, 'n' as u8, 'c' as u8,
            'h' as u8, 'e' as u8, 'n' as u8, '.' as u8, 'd' as u8, 'e' as u8,
        ];
        let out: [128]u8 = [0; 128];
        const n = domainToAscii(nfd_muenchen[0..12], out[0..128]);
        expect_eq(n > (0 as i64), true);
        expect_eq(idnaTestEqBytes(out[0..(n as usize)], "xn--mnchen-3ya.de"), true);
    });

    test("4. @cap/idna-uts46 — domainToUnicode lossless round-trip restoration", () => {
        let muenchen: [11]u8 = [
            'm' as u8, 0xC3 as u8, 0xBC as u8, 'n' as u8, 'c' as u8, 'h' as u8,
            'e' as u8, 'n' as u8, '.' as u8, 'd' as u8, 'e' as u8,
        ];
        let zh_dom: [13]u8 = [
            0xE4 as u8, 0xBE as u8, 0x8B as u8,
            0xE5 as u8, 0xAD as u8, 0x90 as u8,
            '.' as u8,
            0xE6 as u8, 0xB5 as u8, 0x8B as u8,
            0xE8 as u8, 0xAF as u8, 0x95 as u8,
        ];

        let u_out: [128]u8 = [0; 128];
        const u1 = domainToUnicode("XN--MNCHEN-3YA.DE", u_out[0..128]);
        expect_eq(u1, 11 as i64);
        expect_eq(idnaTestEqBytes(u_out[0..(u1 as usize)], muenchen[0..11]), true);

        const u2 = domainToUnicode("xn--fsqu00a.xn--0zwm56d", u_out[0..128]);
        expect_eq(u2, 13 as i64);
        expect_eq(idnaTestEqBytes(u_out[0..(u2 as usize)], zh_dom[0..13]), true);
    });

    test("5. @cap/idna-uts46 — Fullwidth ideographic stop U+3002 (。) separator mapping", () => {
        // "例子。测试" where '。' is U+3002 (E3 80 82)
        let zh_fullstop: [15]u8 = [
            0xE4 as u8, 0xBE as u8, 0x8B as u8,
            0xE5 as u8, 0xAD as u8, 0x90 as u8,
            0xE3 as u8, 0x80 as u8, 0x82 as u8,
            0xE6 as u8, 0xB5 as u8, 0x8B as u8,
            0xE8 as u8, 0xAF as u8, 0x95 as u8,
        ];
        let out: [128]u8 = [0; 128];
        const n = domainToAscii(zh_fullstop[0..15], out[0..128]);
        expect_eq(n > (0 as i64), true);
        expect_eq(idnaTestEqBytes(out[0..(n as usize)], "xn--fsqu00a.xn--0zwm56d"), true);
    });

    test("6. @cap/idna-uts46 — Strict rejection of leading/trailing '-', 64-byte label & bad xn--", () => {
        let out: [128]u8 = [0; 128];

        // Empty domain & empty label
        expect_eq(domainToAscii("", out[0..128]), IDNA_ERR_EMPTY);
        expect_eq(domainToAscii("a..b", out[0..128]), IDNA_ERR_EMPTY_LABEL);
        expect_eq(isValidIdnaAscii(""), false);

        // Leading or trailing hyphen
        expect_eq(domainToAscii("-bad.example", out[0..128]), IDNA_ERR_INVALID_HYPHEN);
        expect_eq(domainToAscii("bad-.example", out[0..128]), IDNA_ERR_INVALID_HYPHEN);
        expect_eq(isValidIdnaAscii("-bad.example"), false);

        // 64-byte label (> 63 max)
        const label64 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.com";
        expect_eq(domainToAscii(label64, out[0..128]), IDNA_ERR_LABEL_TOO_LONG);
        expect_eq(isValidIdnaAscii(label64), false);

        // Invalid Punycode labels ("xn--" empty or overflow / truncated delta "xn--9999999999")
        expect_eq(domainToAscii("xn--.com", out[0..128]), IDNA_ERR_INVALID_PUNYCODE);
        expect_eq(domainToUnicode("xn--9999999999.com", out[0..128]), IDNA_ERR_INVALID_PUNYCODE);
        expect_eq(isValidIdnaAscii("xn--.com"), false);
        expect_eq(isValidIdnaAscii("xn--9999999999.com"), false);
    });
}
