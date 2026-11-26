import { test, expect_eq } from "cap:test";
import {
    PhfHashes,
    PhfBuildResult,
    hashKey,
    displace,
    buildPhf,
    buildPhfWithSeed,
    lookupIndex,
} from "../../packages/@cap/phf-hash/src/index.cp";

fn main() {
    test("1. @cap/phf-hash — 12 HTTP methods & keywords build & O(1) exact lookup", () => {
        let keys: [12]string = [
            "GET", "POST", "PUT", "DELETE", "PATCH", "HEAD",
            "OPTIONS", "CONNECT", "TRACE", "PROPFIND", "MKCOL", "LOCK",
        ];
        let d1: [6]u32 = [0; 6];
        let d2: [6]u32 = [0; 6];
        let slots: [16]i32 = [-1; 16];

        let res: PhfBuildResult = buildPhf(keys[0..12], d1[0..6], d2[0..6], slots[0..16]);
        expect_eq(res.ok, true);
        expect_eq(res.table_len, 16 as usize);
        expect_eq(res.disp_len, 6 as usize);

        let i: usize = 0;
        while (i < 12) {
            let idx: i64 = lookupIndex(keys[i], res.seed, keys[0..12], d1[0..6], d2[0..6], slots[0..16]);
            expect_eq(idx, i as i64);
            i = i + 1;
        }
    });

    test("2. @cap/phf-hash — missing keys return -1", () => {
        let keys: [6]string = ["fn", "let", "const", "struct", "return", "while"];
        let d1: [4]u32 = [0; 4];
        let d2: [4]u32 = [0; 4];
        let slots: [8]i32 = [-1; 8];

        let res: PhfBuildResult = buildPhf(keys[0..6], d1[0..4], d2[0..4], slots[0..8]);
        expect_eq(res.ok, true);

        expect_eq(lookupIndex("class", res.seed, keys[0..6], d1[0..4], d2[0..4], slots[0..8]), -1 as i64);
        expect_eq(lookupIndex("", res.seed, keys[0..6], d1[0..4], d2[0..4], slots[0..8]), -1 as i64);
        expect_eq(lookupIndex("FN", res.seed, keys[0..6], d1[0..4], d2[0..4], slots[0..8]), -1 as i64);
        expect_eq(lookupIndex("return2", res.seed, keys[0..6], d1[0..4], d2[0..4], slots[0..8]), -1 as i64);
    });

    test("3. @cap/phf-hash — different seeds yield different hashes & collision-free tables", () => {
        let h_a: PhfHashes = hashKey("content-type", 101);
        let h_b: PhfHashes = hashKey("content-type", 202);
        expect_eq(h_a.g != h_b.g || h_a.f1 != h_b.f1, true);

        let keys: [8]string = [
            "host", "accept", "cookie", "origin",
            "referer", "upgrade", "connection", "user-agent",
        ];
        let d1_a: [4]u32 = [0; 4];
        let d2_a: [4]u32 = [0; 4];
        let slots_a: [10]i32 = [-1; 10];
        let res_a: PhfBuildResult = buildPhfWithSeed(keys[0..8], 101, d1_a[0..4], d2_a[0..4], slots_a[0..10]);
        expect_eq(res_a.ok, true);

        let d1_b: [4]u32 = [0; 4];
        let d2_b: [4]u32 = [0; 4];
        let slots_b: [10]i32 = [-1; 10];
        let res_b: PhfBuildResult = buildPhfWithSeed(keys[0..8], 99999, d1_b[0..4], d2_b[0..4], slots_b[0..10]);
        expect_eq(res_b.ok, true);

        let i: usize = 0;
        while (i < 8) {
            expect_eq(lookupIndex(keys[i], res_a.seed, keys[0..8], d1_a[0..4], d2_a[0..4], slots_a[0..10]), i as i64);
            expect_eq(lookupIndex(keys[i], res_b.seed, keys[0..8], d1_b[0..4], d2_b[0..4], slots_b[0..10]), i as i64);
            i = i + 1;
        }
    });

    test("4. @cap/phf-hash — single key & empty key set boundaries", () => {
        let single: [1]string = ["only"];
        let d1: [1]u32 = [0; 1];
        let d2: [1]u32 = [0; 1];
        let slots: [1]i32 = [-1; 1];

        let r1: PhfBuildResult = buildPhf(single[0..1], d1[0..1], d2[0..1], slots[0..1]);
        expect_eq(r1.ok, true);
        expect_eq(lookupIndex("only", r1.seed, single[0..1], d1[0..1], d2[0..1], slots[0..1]), 0 as i64);
        expect_eq(lookupIndex("other", r1.seed, single[0..1], d1[0..1], d2[0..1], slots[0..1]), -1 as i64);

        let r0: PhfBuildResult = buildPhf(single[0..0], d1[0..1], d2[0..1], slots[0..1]);
        expect_eq(r0.ok, true);
        expect_eq(lookupIndex("only", r0.seed, single[0..0], d1[0..1], d2[0..1], slots[0..1]), -1 as i64);
    });

    test("5. @cap/phf-hash — similar short-prefix keys a1..a12 collision-free", () => {
        let keys: [12]string = [
            "a1", "a2", "a3", "a4", "a5", "a6",
            "a7", "a8", "a9", "a10", "a11", "a12",
        ];
        let d1: [6]u32 = [0; 6];
        let d2: [6]u32 = [0; 6];
        let slots: [12]i32 = [-1; 12]; // Exact minimal 12-slot perfect hash!

        let res: PhfBuildResult = buildPhf(keys[0..12], d1[0..6], d2[0..6], slots[0..12]);
        expect_eq(res.ok, true);

        let i: usize = 0;
        while (i < 12) {
            expect_eq(lookupIndex(keys[i], res.seed, keys[0..12], d1[0..6], d2[0..6], slots[0..12]), i as i64);
            i = i + 1;
        }
        expect_eq(lookupIndex("a13", res.seed, keys[0..12], d1[0..6], d2[0..6], slots[0..12]), -1 as i64);
    });

    test("6. @cap/phf-hash — displace determinism & zero-length safety", () => {
        let s1: usize = displace(10, 3, 5, 7, 13);
        // (7 + 10 * 5 + 3) % 13 = 60 % 13 = 8
        expect_eq(s1, 8 as usize);
        expect_eq(displace(10, 3, 5, 7, 13), s1);

        // Large u32 values do not overflow
        let big: usize = displace(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 17);
        expect_eq(big < 17, true);
        expect_eq(displace(1, 2, 3, 4, 0), 0 as usize);
    });
}
