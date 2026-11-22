// Origin: Pure-CP Stdlib Test Suite (lib/pack/cdc_chunker.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc FastCDC Chunker & Dedup Index:
//   1. Exact contiguous byte coverage & parity between nextChunk() and chunkAll()
//   2. Shift-resilience after prepending 73 bytes to a 16KB pseudo-random payload
//   3. Strict min_size / max_size bounds enforcement (including uniform zero-byte stream)
//   4. CdcDedupTable exact deduplication ratio, hit/unique accounting, contains() & reset()
//   5. Invalid FastCdcConfig & empty CdcDedupTable buffer rejection guards
//   6. Empty input, sub-min_size input, out_spans cap & full dedup table drop guards

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    FastCdcConfig,
    CdcChunkSpan,
    FastCdcChunker,
    CdcDedupSlot,
    CdcDedupStats,
    CdcDedupTable,
} from "cap:pack";

fn fillPseudoRandom(buf: []u8, seed_in: u64) {
    let s: u64 = seed_in;
    let i: usize = 0;
    while (i < buf.len) : (i += 1) {
        s = s *% 6364136223846793005 +% 1442695040888963407;
        buf[i] = ((s >> 33) & 0xFF) as u8;
    }
}

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const d = if (a >= b) (a - b) else (b - a);
    return d <= eps;
}

// ============================================================================
// Test 1: Exact Contiguous Coverage & nextChunk vs chunkAll Parity
// ============================================================================

fn testExactCoverageAndParity()! {
    let payload: [8192]u8 = [0; 8192];
    fillPseudoRandom(payload[0..], 0x123456789ABCDEF0);

    const cfg = cap.pack.FastCdcConfig.init(128, 512, 2048) ?? return error.InvalidConfig;
    let chunker = FastCdcChunker.init(cfg) ?? return error.InitFailed;

    let all_spans: [128]CdcChunkSpan = [CdcChunkSpan{}; 128];
    const n_all = chunker.chunkAll(payload[0..], all_spans[0..]);
    expect_eq(n_all > 1, true);

    // Stream via nextChunk() and verify 1-to-1 identity + contiguous coverage
    let total_bytes: usize = 0;
    let idx: usize = 0;
    while (span := chunker.nextChunk(payload[0..])) {
        expect_eq(idx < n_all, true);
        expect_eq(span.offset, total_bytes);
        expect_eq(span.offset, all_spans[idx].offset);
        expect_eq(span.len, all_spans[idx].len);
        expect_eq(span.fingerprint, all_spans[idx].fingerprint);
        total_bytes += span.len;
        idx += 1;
    }
    expect_eq(idx, n_all);
    expect_eq(total_bytes, payload.len);
    expect_eq(chunker.position(), payload.len);

    // Reset allows clean replay
    chunker.reset();
    expect_eq(chunker.position(), 0);
    const first = chunker.nextChunk(payload[0..]) ?? return error.Empty;
    expect_eq(first.fingerprint, all_spans[0].fingerprint);
}

// ============================================================================
// Test 2: Shift-Resilience After Head Insertion
// ============================================================================

fn testShiftResilienceAfterHeadInsert()! {
    // Original 16KB pseudo-random stream
    let orig: [16384]u8 = [0; 16384];
    fillPseudoRandom(orig[0..], 0xCAFEBABE11223344);

    // Shifted stream: 73 inserted prefix bytes followed by the exact original 16KB
    let shifted: [16457]u8 = [0; 16457];
    let p: usize = 0;
    while (p < 73) : (p += 1) {
        shifted[p] = ((p * 17 + 5) & 0xFF) as u8;
    }
    let q: usize = 0;
    while (q < 16384) : (q += 1) {
        shifted[73 + q] = orig[q];
    }

    const cfg = FastCdcConfig.init(128, 512, 2048) ?? return error.InvalidConfig;
    let chunker = FastCdcChunker.init(cfg) ?? return error.InitFailed;

    let spans_orig: [256]CdcChunkSpan = [CdcChunkSpan{}; 256];
    let spans_shift: [256]CdcChunkSpan = [CdcChunkSpan{}; 256];
    const n_orig = chunker.chunkAll(orig[0..], spans_orig[0..]);
    const n_shift = chunker.chunkAll(shifted[0..], spans_shift[0..]);
    expect_eq(n_orig >= 10, true);
    expect_eq(n_shift >= 10, true);

    // Insert all original chunks into dedup table, then observe shifted chunks
    let slots: [512]CdcDedupSlot = [CdcDedupSlot{}; 512];
    let dedup = CdcDedupTable.init(slots[0..]) ?? return error.InitFailed;
    let i: usize = 0;
    while (i < n_orig) : (i += 1) {
        * = dedup.observe(spans_orig[i]);
    }

    let reused_chunks: usize = 0;
    let reused_bytes: usize = 0;
    let j: usize = 0;
    while (j < n_shift) : (j += 1) {
        if (!dedup.observe(spans_shift[j])) {
            reused_chunks += 1;
            reused_bytes += spans_shift[j].len;
        }
    }

    // FastCDC must re-synchronize quickly and reuse the vast majority (> 70% of original bytes)
    expect_eq(reused_chunks > 0, true);
    expect_eq(reused_bytes > 11500, true);
}

// ============================================================================
// Test 3: Strict min_size & max_size Bounds Enforcement
// ============================================================================

fn testMinMaxSizeBoundsEnforcement()! {
    const cfg = FastCdcConfig.init(128, 512, 1024) ?? return error.InvalidConfig;
    let chunker = FastCdcChunker.init(cfg) ?? return error.InitFailed;

    // 3a. Pseudo-random stream: every non-final chunk in [min_size, max_size]
    let rand_buf: [6000]u8 = [0; 6000];
    fillPseudoRandom(rand_buf[0..], 0x9988776655443322);
    let spans: [128]CdcChunkSpan = [CdcChunkSpan{}; 128];
    const n = chunker.chunkAll(rand_buf[0..], spans[0..]);
    expect_eq(n > 1, true);
    let i: usize = 0;
    while (i < n) : (i += 1) {
        expect_eq(spans[i].len <= 1024, true);
        if (i + 1 < n) {
            expect_eq(spans[i].len >= 128, true);
        }
    }

    // 3b. Uniform all-zero stream: still respects max_size and covers all bytes
    let zeros: [4096]u8 = [0; 4096];
    const nz = chunker.chunkAll(zeros[0..], spans[0..]);
    expect_eq(nz >= 4, true);
    let total_z: usize = 0;
    let k: usize = 0;
    while (k < nz) : (k += 1) {
        expect_eq(spans[k].len <= 1024, true);
        total_z += spans[k].len;
    }
    expect_eq(total_z, 4096);
}

// ============================================================================
// Test 4: CdcDedupTable Exact Ratio, Accounting & Reset
// ============================================================================

fn testDedupTableExactRatioAndReset()! {
    let data: [4096]u8 = [0; 4096];
    fillPseudoRandom(data[0..], 0xABCD0123456789EF);

    const cfg = FastCdcConfig.init(128, 512, 2048) ?? return error.InvalidConfig;
    const chunker = FastCdcChunker.init(cfg) ?? return error.InitFailed;
    let spans: [64]CdcChunkSpan = [CdcChunkSpan{}; 64];
    const n = chunker.chunkAll(data[0..], spans[0..]);
    expect_eq(n > 0, true);

    let slots: [128]CdcDedupSlot = [CdcDedupSlot{}; 128];
    let dedup = CdcDedupTable.init(slots[0..]) ?? return error.InitFailed;

    // Pass 1: all chunks unique (assuming distinct content in random stream)
    let i: usize = 0;
    while (i < n) : (i += 1) {
        expect_eq(dedup.observe(spans[i]), true);
        expect_eq(dedup.contains(spans[i].fingerprint), true);
    }
    expect_eq(approxEq(dedup.dedupRatio(), 0.0, 0.0000001), true);

    // Pass 2: exact duplicate stream -> 50% byte deduplication savings
    let j: usize = 0;
    while (j < n) : (j += 1) {
        expect_eq(dedup.observe(spans[j]), false);
    }
    const st: CdcDedupStats = dedup.stats();
    expect_eq(st.total_chunks, (n * 2) as u64);
    expect_eq(st.unique_chunks, n as u64);
    expect_eq(st.duplicate_chunks, n as u64);
    expect_eq(st.total_bytes, 8192);
    expect_eq(st.unique_bytes, 4096);
    expect_eq(st.duplicate_bytes, 4096);
    expect_eq(approxEq(st.dedupRatio(), 0.5, 0.0000001), true);

    // Reset clears table cleanly
    dedup.reset();
    expect_eq(dedup.contains(spans[0].fingerprint), false);
    expect_eq(dedup.stats().total_chunks, 0);
    expect_eq(approxEq(dedup.dedupRatio(), 0.0, 0.0000001), true);
}

// ============================================================================
// Test 5: Invalid Configuration & Empty Buffer Guards
// ============================================================================

fn testInvalidConfigGuards()! {
    expect_eq(FastCdcConfig.init(32, 256, 1024) == null, true);   // min < 64
    expect_eq(FastCdcConfig.init(256, 256, 1024) == null, true);  // min == avg
    expect_eq(FastCdcConfig.init(512, 256, 1024) == null, true);  // min > avg
    expect_eq(FastCdcConfig.init(128, 1024, 1024) == null, true); // avg == max
    expect_eq(FastCdcConfig.init(128, 2048, 1024) == null, true); // avg > max

    const bad_cfg = FastCdcConfig{ min_size: 16, avg_size: 64, max_size: 128 };
    expect_eq(FastCdcChunker.init(bad_cfg) == null, true);

    let empty_slots: [0]CdcDedupSlot = [];
    expect_eq(CdcDedupTable.init(empty_slots[0..]) == null, true);
}

// ============================================================================
// Test 6: Empty Input, Short Input, Span Cap & Full Dedup Table Drops
// ============================================================================

fn testEmptyShortAndFullTableGuards()! {
    const cfg = FastCdcConfig.init(64, 256, 1024) ?? return error.InvalidConfig;
    let chunker = FastCdcChunker.init(cfg) ?? return error.InitFailed;

    let empty: [0]u8 = [];
    let spans: [4]CdcChunkSpan = [CdcChunkSpan{}; 4];
    expect_eq(chunker.chunkAll(empty[0..], spans[0..]), 0);
    expect_eq(chunker.nextChunk(empty[0..]) == null, true);

    // Input shorter than min_size -> single chunk covering entire input
    const short_msg: string = "hello fastcdc";
    expect_eq(chunker.chunkAll(short_msg, spans[0..]), 1);
    expect_eq(spans[0].offset, 0);
    expect_eq(spans[0].len, short_msg.len);

    // out_spans cap prevents OOB writes
    let big: [2048]u8 = [0; 2048];
    fillPseudoRandom(big[0..], 0x55555555);
    let tiny_spans: [2]CdcChunkSpan = [CdcChunkSpan{}; 2];
    expect_eq(chunker.chunkAll(big[0..], tiny_spans[0..]), 2);

    // Tiny dedup table (capacity 2) safely counts table_full_drops on 3rd distinct chunk
    let small_slots: [2]CdcDedupSlot = [CdcDedupSlot{}; 2];
    let dedup = CdcDedupTable.init(small_slots[0..]) ?? return error.InitFailed;
    expect_eq(dedup.observe(CdcChunkSpan{ offset: 0, len: 100, fingerprint: 111 }), true);
    expect_eq(dedup.observe(CdcChunkSpan{ offset: 100, len: 100, fingerprint: 222 }), true);
    expect_eq(dedup.observe(CdcChunkSpan{ offset: 200, len: 100, fingerprint: 333 }), false);
    expect_eq(dedup.stats().table_full_drops, 1);
}

test("pack.cdc_chunker: exact contiguous coverage & nextChunk/chunkAll parity", testExactCoverageAndParity);
test("pack.cdc_chunker: shift-resilience after 73-byte head insertion", testShiftResilienceAfterHeadInsert);
test("pack.cdc_chunker: strict min_size & max_size bounds enforcement", testMinMaxSizeBoundsEnforcement);
test("pack.cdc_chunker: CdcDedupTable exact ratio, accounting & reset", testDedupTableExactRatioAndReset);
test("pack.cdc_chunker: invalid FastCdcConfig & empty dedup table guards", testInvalidConfigGuards);
test("pack.cdc_chunker: empty/short input, span cap & full dedup table drops", testEmptyShortAndFullTableGuards);
