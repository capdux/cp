// Production deep regression tests for lib/crash_handler/cpuFeatures.cp (cap:crash_handler)

import { test, expect_eq } from "cap:test";
import {
    CPU_FEATURES_FORMAT_BUF_CAP,
    X86_BIT_SSE42,
    X86_BIT_POPCNT,
    X86_BIT_AVX,
    X86_BIT_AVX2,
    X86_BIT_AVX512,
    AARCH64_BIT_NEON,
    AARCH64_BIT_FP,
    AARCH64_BIT_AES,
    AARCH64_BIT_CRC32,
    AARCH64_BIT_ATOMICS,
    AARCH64_BIT_SVE,
    CpuFeaturesError,
    CpuArchKind,
    X86CpuFlags,
    Aarch64CpuFlags,
    CpuFeatures,
} from "cap:crash_handler";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("cpuFeatures: x86_64 bitmask parity, baseline & Haswell checks", () => {
    expect_eq(CPU_FEATURES_FORMAT_BUF_CAP, 64);

    // Empty mask (and ignored bit 0 / padding bits 6..7)
    const empty = X86CpuFlags.fromMask(0b1100_0001);
    expect_eq(empty.isEmpty(), true);
    expect_eq(empty.toMask(), 0);
    expect_eq(empty.hasAnyAvx(), false);
    expect_eq(empty.meetsBaseline(), false);
    expect_eq(empty.meetsHaswell(), false);

    // Baseline Nehalem: SSE4.2 + POPCNT
    const baseline = X86CpuFlags.fromMask(X86_BIT_SSE42 | X86_BIT_POPCNT);
    expect_eq(baseline.isEmpty(), false);
    expect_eq(baseline.meetsBaseline(), true);
    expect_eq(baseline.meetsHaswell(), false);
    expect_eq(baseline.hasAnyAvx(), false);

    // Haswell: SSE4.2 + POPCNT + AVX + AVX2
    const haswell = X86CpuFlags.fromMask(X86_BIT_SSE42 | X86_BIT_POPCNT | X86_BIT_AVX | X86_BIT_AVX2);
    expect_eq(haswell.meetsBaseline(), true);
    expect_eq(haswell.meetsHaswell(), true);
    expect_eq(haswell.hasAnyAvx(), true);
    expect_eq(haswell.toMask(), X86_BIT_SSE42 | X86_BIT_POPCNT | X86_BIT_AVX | X86_BIT_AVX2);

    // AVX-512 alone satisfies hasAnyAvx
    const avx512 = X86CpuFlags.fromMask(X86_BIT_AVX512);
    expect_eq(avx512.hasAnyAvx(), true);
});

test("cpuFeatures: x86_64 format & parse round-trip", () => {
    const f = X86CpuFlags{
        sse42: true,
        popcnt: true,
        avx: true,
        avx2: true,
        avx512: false,
    };
    let buf: [64]u8 = [0; 64];
    const n = f.formatToBuf(buf[0..]) catch 0;
    expect_eq(strEq(buf[0..n], "sse42 popcnt avx avx2"), true);

    const parsed = X86CpuFlags.parseFromStr(buf[0..n]) catch X86CpuFlags{};
    expect_eq(parsed.eql(f), true);

    // Empty formats to 0 bytes
    const empty = X86CpuFlags{};
    const n0 = empty.formatToBuf(buf[0..]) catch 99;
    expect_eq(n0, 0);
});

test("cpuFeatures: aarch64 bitmask, crypto check & format/parse round-trip", () => {
    const mask = AARCH64_BIT_NEON | AARCH64_BIT_FP | AARCH64_BIT_AES | AARCH64_BIT_CRC32 | AARCH64_BIT_ATOMICS;
    const arm = Aarch64CpuFlags.fromMask(mask);
    expect_eq(arm.isEmpty(), false);
    expect_eq(arm.hasCryptoAndCrc(), true);
    expect_eq(arm.sve, false);
    expect_eq(arm.toMask(), mask);

    let buf: [64]u8 = [0; 64];
    const n = arm.formatToBuf(buf[0..]) catch 0;
    expect_eq(strEq(buf[0..n], "neon fp aes crc32 atomics"), true);

    const parsed = Aarch64CpuFlags.parseFromStr("  neon   fp aes\tcrc32 atomics ") catch Aarch64CpuFlags{};
    expect_eq(parsed.eql(arm), true);

    const with_sve = Aarch64CpuFlags.fromMask(AARCH64_BIT_SVE);
    expect_eq(with_sve.sve, true);
    expect_eq(with_sve.hasCryptoAndCrc(), false);
});

test("cpuFeatures: unified CpuFeatures tagged dispatch", () => {
    const x_feat = CpuFeatures.fromRawMask(CpuArchKind.X86_64, X86_BIT_SSE42 | X86_BIT_AVX2);
    expect_eq(x_feat.isEmpty(), false);
    expect_eq(x_feat.hasAnyAvx(), true);
    expect_eq(x_feat.toRawMask(), X86_BIT_SSE42 | X86_BIT_AVX2);

    let buf: [64]u8 = [0; 64];
    const xn = x_feat.formatToBuf(buf[0..]) catch 0;
    expect_eq(strEq(buf[0..xn], "sse42 avx2"), true);

    const a_feat = CpuFeatures.fromRawMask(CpuArchKind.Aarch64, AARCH64_BIT_NEON | AARCH64_BIT_SVE);
    expect_eq(a_feat.isEmpty(), false);
    expect_eq(a_feat.hasAnyAvx(), false);
    const an = a_feat.formatToBuf(buf[0..]) catch 0;
    expect_eq(strEq(buf[0..an], "neon sve"), true);
});

test("cpuFeatures: error guards (BufferTooSmall & UnknownFeatureToken)", () => {
    const f = X86CpuFlags.fromMask(X86_BIT_SSE42 | X86_BIT_POPCNT | X86_BIT_AVX2);
    let tiny: [8]u8 = [0; 8];
    let buf_err = false;
    * = f.formatToBuf(tiny[0..]) catch |e| {
        buf_err = (e == CpuFeaturesError.BufferTooSmall);
        0
    };
    expect_eq(buf_err, true);

    let bad_x86 = false;
    * = X86CpuFlags.parseFromStr("sse42 neon") catch |e| {
        bad_x86 = (e == CpuFeaturesError.UnknownFeatureToken);
        X86CpuFlags{}
    };
    expect_eq(bad_x86, true);

    let bad_arm = false;
    * = Aarch64CpuFlags.parseFromStr("neon avx2") catch |e| {
        bad_arm = (e == CpuFeaturesError.UnknownFeatureToken);
        Aarch64CpuFlags{}
    };
    expect_eq(bad_arm, true);
});
