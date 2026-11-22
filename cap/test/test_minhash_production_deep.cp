// Source: cp/cap/test/test_minhash_production_deep.cp (Production deep tests for cap:minhash)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MINHASH_DEFAULT_K,
    MINHASH_EMPTY_VAL,
    MinHash,
    minhashJaccard,
    minhashBandHash,
    minhashCardinality,
} from "cap:crypto";

fn absDiff(a: f64, b: f64): f64 {
    if (a >= b) {
        return a - b;
    }
    return b - a;
}

test("1. Identical sets Jaccard == 1.0, disjoint sets near 0.0, and text word / shingle sketches", () => {
    expect_eq(MINHASH_DEFAULT_K, 128 as usize);
    expect_eq(MINHASH_EMPTY_VAL, 0xFFFFFFFFFFFFFFFF as u64);

    let buf_a: [128]u64 = std.mem.zeroes([128]u64);
    let buf_b: [128]u64 = std.mem.zeroes([128]u64);
    let buf_c: [128]u64 = std.mem.zeroes([128]u64);

    if (ma_init := MinHash.init(buf_a[0..])) {
        if (mb_init := MinHash.init(buf_b[0..])) {
            if (mc_init := MinHash.init(buf_c[0..])) {
                let ma: MinHash = ma_init;
                let mb: MinHash = mb_init;
                let mc: MinHash = mc_init;
                expect_eq(ma.len(), 128 as usize);

                // Identical words (different casing/punctuation) -> Jaccard == 1.0.
                ma.addTextWords("The quick brown fox jumps over the lazy dog!");
                mb.addTextWords("the QUICK, brown... fox jumps over the lazy dog?");
                if (j_same := ma.jaccard(mb)) {
                    expect_eq(j_same, 1.0);
                } else {
                    expect_eq(true, false);
                }

                // Completely disjoint word set -> Jaccard near 0.0 (< 0.05).
                mc.addTextWords("alpha beta gamma delta epsilon zeta eta theta");
                if (j_disj := ma.jaccard(mc)) {
                    expect_eq(j_disj < 0.05, true);
                } else {
                    expect_eq(true, false);
                }

                // Reset and verify char shingles similarity.
                ma.reset();
                mb.reset();
                ma.addCharShingles("minhash-similarity-sketch", 4);
                mb.addCharShingles("minhash-similarity-search", 4);
                if (j_sh := ma.jaccard(mb)) {
                    expect_eq(j_sh > 0.5 && j_sh < 1.0, true);
                } else {
                    expect_eq(true, false);
                }
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("2. Overlapping sets ({0..999} vs {500..1499} J=1/3, and {0..999} vs {0..499} J=0.5) with k=256 accuracy", () => {
    let buf_a: [256]u64 = std.mem.zeroes([256]u64);
    let buf_b: [256]u64 = std.mem.zeroes([256]u64);
    let buf_c: [256]u64 = std.mem.zeroes([256]u64);

    if (ma_init := MinHash.init(buf_a[0..])) {
        if (mb_init := MinHash.init(buf_b[0..])) {
            if (mc_init := MinHash.init(buf_c[0..])) {
                let ma: MinHash = ma_init;
                let mb: MinHash = mb_init;
                let mc: MinHash = mc_init;

                let i: u64 = 0;
                while (i < 1000) {
                    ma.addHash(i);
                    i += 1;
                }
                let j: u64 = 500;
                while (j < 1500) {
                    mb.addHash(j);
                    j += 1;
                }
                let k: u64 = 0;
                while (k < 500) {
                    mc.addHash(k);
                    k += 1;
                }

                // True Jaccard(A, B) = 500 / 1500 = 0.3333...
                if (j_ab := ma.jaccard(mb)) {
                    expect_eq(absDiff(j_ab, 0.3333333333) < 0.08, true);
                } else {
                    expect_eq(true, false);
                }

                // True Jaccard(A, C) = 500 / 1000 = 0.5
                if (j_ac := ma.jaccard(mc)) {
                    expect_eq(absDiff(j_ac, 0.5) < 0.08, true);
                } else {
                    expect_eq(true, false);
                }

                // Cardinality estimate for 1000 elements should be within reasonable order-statistic bounds.
                const card: f64 = minhashCardinality(ma.sig);
                expect_eq(card > 700.0 && card < 1400.0, true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("3. mergeFrom() produces slot-by-slot identical signature to direct union insertion", () => {
    let buf_a: [128]u64 = std.mem.zeroes([128]u64);
    let buf_b: [128]u64 = std.mem.zeroes([128]u64);
    let buf_u: [128]u64 = std.mem.zeroes([128]u64);

    if (ma_init := MinHash.init(buf_a[0..])) {
        if (mb_init := MinHash.init(buf_b[0..])) {
            if (mu_init := MinHash.init(buf_u[0..])) {
                let ma: MinHash = ma_init;
                let mb: MinHash = mb_init;
                let mu: MinHash = mu_init;

                let x: u64 = 0;
                while (x < 300) {
                    ma.addHash(x * 3 + 1);
                    mu.addHash(x * 3 + 1);
                    x += 1;
                }
                let y: u64 = 150;
                while (y < 450) {
                    mb.addHash(y * 3 + 1);
                    mu.addHash(y * 3 + 1);
                    y += 1;
                }

                expect_eq(ma.mergeFrom(mb), true);

                // Every single slot must be 100% bit-identical to the direct union sketch.
                let slot: usize = 0;
                while (slot < 128) {
                    expect_eq(ma.sig[slot], mu.sig[slot]);
                    slot += 1;
                }
                if (j_u := ma.jaccard(mu)) {
                    expect_eq(j_u, 1.0);
                } else {
                    expect_eq(true, false);
                }
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("4. LSH minhashBandHash shares >= 1 matching band for high-similarity sets and differs for disjoint sets", () => {
    // 128 rows = 32 bands x 4 rows per band (at J=0.85, P(>=1 match) = 1 - (1 - 0.85^4)^32 > 0.9999999).
    let buf_a: [128]u64 = std.mem.zeroes([128]u64);
    let buf_b: [128]u64 = std.mem.zeroes([128]u64);
    let buf_d: [128]u64 = std.mem.zeroes([128]u64);

    if (ma_init := MinHash.init(buf_a[0..])) {
        if (mb_init := MinHash.init(buf_b[0..])) {
            if (md_init := MinHash.init(buf_d[0..])) {
                let ma: MinHash = ma_init;
                let mb: MinHash = mb_init;
                let md: MinHash = md_init;

                let i: u64 = 0;
                while (i < 1000) {
                    ma.addHash(i);
                    if (i < 920) {
                        mb.addHash(i);
                    }
                    md.addHash(i + 50000);
                    i += 1;
                }

                let shared_high: usize = 0;
                let shared_disj: usize = 0;
                let band: usize = 0;
                while (band < 32) {
                    if (ha := minhashBandHash(ma.sig, band, 4)) {
                        if (hb := minhashBandHash(mb.sig, band, 4)) {
                            if (ha == hb) {
                                shared_high += 1;
                            }
                        } else {
                            expect_eq(true, false);
                        }
                        if (hd := minhashBandHash(md.sig, band, 4)) {
                            if (ha == hd) {
                                shared_disj += 1;
                            }
                        } else {
                            expect_eq(true, false);
                        }
                    } else {
                        expect_eq(true, false);
                    }
                    band += 1;
                }

                expect_eq(shared_high >= 1, true);
                expect_eq(shared_disj, 0 as usize);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("5. Malformed & edge-case guards: empty slice, length mismatch, out-of-bounds band, empty cardinality", () => {
    let empty_arr: [0]u64 = undefined;
    if (_ := MinHash.init(empty_arr[0..0])) {
        expect_eq(true, false);
    }
    if (_ := minhashJaccard(empty_arr[0..0], empty_arr[0..0])) {
        expect_eq(true, false);
    }
    expect_eq(minhashCardinality(empty_arr[0..0]), 0.0);

    let buf64: [64]u64 = std.mem.zeroes([64]u64);
    let buf32: [32]u64 = std.mem.zeroes([32]u64);

    if (m64_init := MinHash.init(buf64[0..])) {
        if (m32_init := MinHash.init(buf32[0..])) {
            let m64: MinHash = m64_init;
            const m32: MinHash = m32_init;

            // Unpopulated signature cardinality must be 0.0.
            expect_eq(minhashCardinality(m64.sig), 0.0);

            // Length mismatch rejected by mergeFrom and jaccard.
            expect_eq(m64.mergeFrom(m32), false);
            if (_ := m64.jaccard(m32)) {
                expect_eq(true, false);
            }
            if (_ := minhashJaccard(m64.sig, m32.sig)) {
                expect_eq(true, false);
            }

            // Invalid LSH band parameters: rows_per_band == 0, band_idx out of range.
            if (_ := minhashBandHash(m64.sig, 0, 0)) {
                expect_eq(true, false);
            }
            if (_ := minhashBandHash(m64.sig, 16, 4)) {
                expect_eq(true, false);
            }
            if (_ := minhashBandHash(m64.sig, 0, 65)) {
                expect_eq(true, false);
            }
            // Valid boundary band (15 * 4 + 4 == 64).
            if (_ := minhashBandHash(m64.sig, 15, 4)) {
                // ok
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});
