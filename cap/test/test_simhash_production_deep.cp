// Source: cp/cap/test/test_simhash_production_deep.cp (Production deep tests for cap:simhash)

import { test, expect_eq } from "cap:test";
import {
    SIMHASH_BITS,
    SIMHASH_DEFAULT_THRESHOLD,
    fnv1a64,
    SimHasher,
    simhashText,
    simhashShingles,
    hammingDistance,
    simhashSimilarity,
    isNearDuplicate,
    simhashBands4,
    sharesAnyBand4,
} from "cap:crypto";

test("1. Identical documents, case/punctuation normalization & FNV-1a reference check", () => {
    expect_eq(SIMHASH_BITS, 64 as u8);
    expect_eq(SIMHASH_DEFAULT_THRESHOLD, 3 as u8);

    // Standard FNV-1a 64-bit test vectors: "" -> 0xCBF29CE484222325, "foobar" -> 0x85944171F73967E8
    expect_eq(fnv1a64(""), 0xCBF29CE484222325 as u64);
    expect_eq(fnv1a64("foobar"), 0x85944171F73967E8 as u64);

    const d1 = "The quick brown fox jumps over the lazy dog in the quiet forest";
    const d2 = "THE QUICK, BROWN FOX! Jumps over the lazy dog... in the quiet forest.";
    const f1 = simhashText(d1);
    const f2 = simhashText(d2);

    expect_eq(f1, f2);
    expect_eq(hammingDistance(f1, f2), 0 as u8);
    expect_eq(simhashSimilarity(f1, f2), 1.0);
    expect_eq(isNearDuplicate(f1, f2, SIMHASH_DEFAULT_THRESHOLD), true);
});

test("2. Minor paraphrase yields small Hamming distance vs unrelated document", () => {
    const base_doc = "CP language compiles to native Zig state machines for million scale concurrent AI agents with zero garbage collection pause and deterministic memory safety across distributed actor nodes";
    const near_doc = "CP language compiles to native Zig state machines for million scale concurrent AI agents with zero garbage collection latency and deterministic memory safety across distributed actor nodes";
    const diff_doc = "Bananas grow in tropical climates with heavy rainfall and warm sunshine while penguins swim rapidly through freezing Antarctic ocean currents hunting krill";

    const f_base = simhashText(base_doc);
    const f_near = simhashText(near_doc);
    const f_diff = simhashText(diff_doc);

    const d_near = hammingDistance(f_base, f_near);
    const d_diff = hammingDistance(f_base, f_diff);

    // Single-word swap in 26-word doc produces very small distance, much smaller than unrelated doc
    expect_eq(d_near <= 6, true);
    expect_eq(d_diff > d_near + 10, true);
    expect_eq(simhashSimilarity(f_base, f_near) > simhashSimilarity(f_base, f_diff), true);
});

test("3. Signed weight accumulation, cancellation & reset semantics", () => {
    let h = SimHasher.init();
    const h_a: u64 = 0xF0F0F0F00F0F0F0F;
    const h_b: u64 = 0xFFFFFFFFFFFFFFFF;

    // Dominant positive weight on h_a forces fingerprint == h_a
    h.addHash(h_a, 10);
    h.addHash(h_b, 3);
    expect_eq(h.feature_count, 2 as u64);
    expect_eq(h.finish(), h_a);

    // Adding -10 on h_a cancels it completely, leaving only h_b (weight +3 -> all 1s)
    h.addHash(h_a, -10);
    expect_eq(h.finish(), h_b);

    // Reset zeroes weights and feature_count
    h.reset();
    expect_eq(h.feature_count, 0 as u64);
    expect_eq(h.finish(), 0 as u64);

    // addToken consistency with fnv1a64
    h.addToken("agent", 5);
    expect_eq(h.finish(), fnv1a64("agent"));
});

test("4. addCharShingles for continuous / UTF-8 text & empty/short boundaries", () => {
    // Empty inputs yield 0
    expect_eq(simhashText(""), 0 as u64);
    expect_eq(simhashText("   !!! ---   "), 0 as u64);
    expect_eq(simhashShingles("", 4), 0 as u64);
    expect_eq(simhashShingles("hello", 0), 0 as u64);

    // Short text (< k) hashes entire slice once
    expect_eq(simhashShingles("ab", 5), fnv1a64("ab"));

    // Continuous / UTF-8 CJK text near-duplicate via 6-byte shingles
    const zh1 = "人工智能并发编程语言零垃圾回收确定性内存安全系统架构设计与高性能原生编译引擎";
    const zh2 = "人工智能并发编程语言零垃圾回收确定性内存安全系统架构设计与高性能原生执行引擎";
    const zh3 = "今天下午海边天气晴朗微风拂面适合散步拍照品尝新鲜水果与冰镇柠檬茶饮品";

    const s1 = simhashShingles(zh1, 6);
    const s2 = simhashShingles(zh2, 6);
    const s3 = simhashShingles(zh3, 6);

    const d12 = hammingDistance(s1, s2);
    const d13 = hammingDistance(s1, s3);
    expect_eq(d12 < d13, true);
    expect_eq(d12 <= 8, true);
});

test("5. Pigeonhole principle on 4x16-bit LSH bands (any dist <= 3 guarantees >= 1 matching band)", () => {
    const base_fp: u64 = 0x123456789ABCDEF0;
    const bands = simhashBands4(base_fp);
    expect_eq(bands[0], 0xDEF0 as u16);
    expect_eq(bands[1], 0x9ABC as u16);
    expect_eq(bands[2], 0x5678 as u16);
    expect_eq(bands[3], 0x1234 as u16);

    // Exhaustive check across representative 1-bit, 2-bit, and 3-bit perturbations spanning distinct bands
    let i: u32 = 0;
    while (i < 64) {
        const f1 = base_fp ^ ((1 as u64) << (i as u6));
        expect_eq(hammingDistance(base_fp, f1), 1 as u8);
        expect_eq(sharesAnyBand4(base_fp, f1), true);
        i += 1;
    }

    // 3-bit errors placed in 3 different 16-bit bands -> 4th band MUST still match
    const three_band_err = ((1 as u64) << 0) | ((1 as u64) << 17) | ((1 as u64) << 35);
    expect_eq(hammingDistance(base_fp, base_fp ^ three_band_err), 3 as u8);
    expect_eq(isNearDuplicate(base_fp, base_fp ^ three_band_err, 3), true);
    expect_eq(sharesAnyBand4(base_fp, base_fp ^ three_band_err), true);

    // 4-bit error placed across all 4 bands -> no band matches
    const four_band_err = ((1 as u64) << 0) | ((1 as u64) << 16) | ((1 as u64) << 32) | ((1 as u64) << 48);
    expect_eq(hammingDistance(base_fp, base_fp ^ four_band_err), 4 as u8);
    expect_eq(isNearDuplicate(base_fp, base_fp ^ four_band_err, 3), false);
    expect_eq(sharesAnyBand4(base_fp, base_fp ^ four_band_err), false);
});
