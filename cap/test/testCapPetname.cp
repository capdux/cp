// cap/test/testCapPetname.cp
// Comprehensive unit tests for @cap/petname

import { test, expect_eq } from "cap:test";
import { Petname, petnameFromSeed, subdomainFromSeed } from "../../packages/@cap/petname/src/index.cp";

fn countChar(s: string, ch: u8) -> usize {
    let n: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) {
            n += 1;
        }
        i += 1;
    }
    return n;
}

fn isDnsSafe(s: string) -> bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        const ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
        if (!ok) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/petname — Deterministic reproducibility & seed sensitivity", () => {
        let b1: [64]u8 = undefined;
        let b2: [64]u8 = undefined;
        let b3: [64]u8 = undefined;
        const s1 = petnameFromSeed(12345 as u64, 2 as u8, "-", b1[0..64]);
        const s2 = petnameFromSeed(12345 as u64, 2 as u8, "-", b2[0..64]);
        const s3 = petnameFromSeed(99999 as u64, 2 as u8, "-", b3[0..64]);
        expect_eq(s1, s2);
        expect_eq(s1.len > 4, true);
        expect_eq(countChar(s1, '-'), 1 as usize);
        expect_eq(s1.len == s3.len && s1[0] == s3[0] && s1[s1.len - 1] == s3[s3.len - 1], false);
    });

    test("2. @cap/petname — 1, 2, 3 & 4 word variants with custom separator", () => {
        let buf: [128]u8 = undefined;
        const w1 = petnameFromSeed(7 as u64, 1 as u8, "-", buf[0..128]);
        expect_eq(countChar(w1, '-'), 0 as usize);

        const w3 = petnameFromSeed(7 as u64, 3 as u8, "_", buf[0..128]);
        expect_eq(countChar(w3, '_'), 2 as usize);

        const w4 = petnameFromSeed(7 as u64, 4 as u8, ".", buf[0..128]);
        expect_eq(countChar(w4, '.'), 3 as usize);
    });

    test("3. @cap/petname — DNS-safe subdomain generation with 4-hex suffix", () => {
        let buf: [64]u8 = undefined;
        const sub = subdomainFromSeed(2026 as u64, buf[0..64]);
        expect_eq(isDnsSafe(sub), true);
        expect_eq(countChar(sub, '-'), 2 as usize);
        expect_eq(sub.len >= 10, true);
    });

    test("4. @cap/petname — Stateful sequence produces distinct names", () => {
        let gen = Petname.init(42 as u64);
        let b1: [64]u8 = undefined;
        let b2: [64]u8 = undefined;
        const n1 = gen.generateSlug(b1[0..64]);
        const n2 = gen.generateSlug(b2[0..64]);
        expect_eq(isDnsSafe(n1), true);
        expect_eq(isDnsSafe(n2), true);
        expect_eq(n1.len == n2.len && n1[0] == n2[0] && n1[n1.len - 1] == n2[n2.len - 1], false);
    });
}
