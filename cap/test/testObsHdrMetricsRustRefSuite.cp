import { test, expect_eq } from "cap:test";
import {
    HDR_TOTAL_BUCKETS,
    HDR_MAX_SUPPORTED_VALUE,
    scalarHdrMergeBuckets,
    simdHdrMergeBuckets,
    scalarHdrSumBuckets,
    simdHdrSumBuckets,
    HdrHistogram,
} from "../../lib/obs/hdrHist.cp";
import {
    isValidMetricName,
    isValidLabelName,
    scalarOmFindNeedEscape,
    simdOmFindNeedEscape,
    MetricType,
    MetricsDocument,
    parse,
} from "../../lib/obs/openmetrics.cp";

const std = @import("std");

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

fn expect_false(cond: bool): void! {
    expect_eq(cond, false);
}

fn tryParseOm(alloc: std.mem.Allocator, text: []const u8): MetricsDocument? {
    return parse(alloc, text) catch return null;
}

struct XorShift64 {
    state: u64 = 0x1E3779B97F4A7C15,

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }
}

// ---------------------------------------------------------------------------
// Test 1: HdrHistogram Precision, Clamping, Monotonicity & Saturating Merge
// ---------------------------------------------------------------------------

fn runTestHdrHistogramStrict(): void! {
    // 1. Empty histogram quantiles safely return 0
    let h = HdrHistogram.init(1_000_000);
    expect_eq(h.value_at_quantile(0.0), 0 as u64);
    expect_eq(h.value_at_quantile(0.50), 0 as u64);
    expect_eq(h.value_at_quantile(0.99), 0 as u64);
    expect_eq(h.value_at_quantile(1.0), 0 as u64);
    expect_eq(h.value_at_quantile(std.math.nan(f64)), 0 as u64);
    expect_eq(h.value_at_quantile(-0.1), 0 as u64);
    expect_eq(h.value_at_quantile(1.1), 0 as u64);

    // 2. Exact linear band [0, 127] and log-linear accuracy (<= 1.5625% relative error)
    let v: u64 = 1;
    while (v <= 1000) : (v += 1) {
        expect_true(h.recordValue(v))?;
    }
    const sum = h.summarize();
    expect_eq(sum.total_count, 1000 as u64);
    expect_eq(sum.min_val, 1 as u64);
    expect_eq(sum.max_val, 1000 as u64);
    expect_true(sum.min_val <= sum.p50)?;
    expect_true(sum.p50 <= sum.p90)?;
    expect_true(sum.p90 <= sum.p95)?;
    expect_true(sum.p95 <= sum.p99)?;
    expect_true(sum.p99 <= sum.p999)?;
    expect_true(sum.p999 <= sum.max_val)?;
    expect_true(sum.p50 >= 500 and sum.p50 <= 520)?;
    expect_true(sum.p99 >= 990 and sum.p99 <= 1000)?;

    // 3. Out-of-bounds clamping
    expect_false(h.recordValue(2_000_000))?;
    expect_eq(h.clamped_count, 1 as u64);
    expect_eq(h.max_val, 1_000_000 as u64);

    // 4. Saturating u64 merge (no wraparound overflow)
    let ha = HdrHistogram.init(10_000);
    let hb = HdrHistogram.init(10_000);
    const big: u64 = std.math.maxInt(u64) - 15;
    expect_true(ha.record_n(42, big))?;
    expect_true(hb.record_n(42, 100))?;
    expect_true(ha.merge(hb))?;
    expect_eq(ha.buckets[42], std.math.maxInt(u64));
    expect_eq(ha.total_count, std.math.maxInt(u64));

    // Mismatched max_trackable_val merge rejected
    const hc = HdrHistogram.init(20_000);
    expect_false(ha.merge(hc))?;
}

// ---------------------------------------------------------------------------
// Test 2: OpenMetrics 1.0 Strict Parsing, Escaping, Special Floats & Rejections
// ---------------------------------------------------------------------------

fn runTestOpenMetricsStrict(): void! {
    const alloc = std.heap.page_allocator;

    // 1. Metric name & label name validation
    expect_true(isValidMetricName("http_requests_total"))?;
    expect_true(isValidMetricName(":custom:metric_01"))?;
    expect_false(isValidMetricName(""))?;
    expect_false(isValidMetricName("9invalid_start"))?;
    expect_false(isValidMetricName("bad-dash"))?;
    expect_false(isValidMetricName("bad.dot"))?;

    expect_true(isValidLabelName("method"))?;
    expect_true(isValidLabelName("_private_1"))?;
    expect_false(isValidLabelName(""))?;
    expect_false(isValidLabelName("0digit"))?;
    expect_false(isValidLabelName("has:colon"))?;
    expect_false(isValidLabelName("has-dash"))?;

    // 2. Full OpenMetrics 1.0 document with escapes, +Inf, -Inf, NaN, timestamp, # EOF
    const doc_text =
        "# HELP http_latency_seconds Request latency\\nwith newline and \\\\ slash\n" ++
        "# TYPE http_latency_seconds histogram\n" ++
        "# UNIT http_latency_seconds seconds\n" ++
        "http_latency_seconds_bucket{path=\"/api/v1/\\\"quoted\\\"\\nline\\\\end\",le=\"0.5\"} 12 1700000000000\n" ++
        "http_latency_seconds_bucket{path=\"/ok\",le=\"+Inf\"} +Inf\n" ++
        "http_latency_seconds_sum -Inf\n" ++
        "http_latency_seconds_count NaN\n" ++
        "# EOF\n";

    let doc = parse(alloc, doc_text) catch unreachable;
    defer doc.deinit(alloc);
    expect_true(doc.has_eof)?;
    const fam = doc.findFamily("http_latency_seconds").?;
    expect_eq(fam.metric_type, MetricType.Histogram);
    expect_eq(fam.samples.len, 4 as usize);
    expect_true(std.mem.eql(u8, fam.samples[0].labels[0].value, "/api/v1/\"quoted\"\nline\\end"))?;
    expect_true(std.math.isPositiveInf(fam.samples[1].value))?;
    expect_true(std.math.isNegativeInf(fam.samples[2].value))?;
    expect_true(std.math.isNan(fam.samples[3].value))?;

    // Serialize roundtrip preserves +Inf / -Inf / NaN and escaped label values
    const serialized = doc.serialize(alloc) catch unreachable;
    defer alloc.free(serialized);
    expect_true(std.mem.indexOf(u8, serialized, "+Inf") != null)?;
    expect_true(std.mem.indexOf(u8, serialized, "-Inf") != null)?;
    expect_true(std.mem.indexOf(u8, serialized, "NaN") != null)?;
    expect_true(std.mem.indexOf(u8, serialized, "\\\"quoted\\\"\\nline\\\\end") != null)?;

    let doc2 = parse(alloc, serialized) catch unreachable;
    defer doc2.deinit(alloc);
    const fam2 = doc2.findFamily("http_latency_seconds").?;
    expect_eq(fam2.samples.len, 4 as usize);
    expect_true(std.mem.eql(u8, fam2.samples[0].labels[0].value, "/api/v1/\"quoted\"\nline\\end"))?;

    // 3. Reject invalid metric names, invalid label names, illegal escapes, embedded CR/NUL/control chars
    const bad_docs: [6][]const u8 = [
        "123bad_metric 1.0\n",
        "good_metric{1bad=\"v\"} 1.0\n",
        "good_metric{k=\"bad\\xescape\"} 1.0\n",
        "good_metric{k=\"embedded\rvalue\"} 1.0\n",
        "good_metric{k=\"ctrl\x07bell\"} 1.0\n",
        "good_metric{k=\"nul\x00byte\"} 1.0\n",
    ];
    let bi: usize = 0;
    while (bi < bad_docs.len) : (bi += 1) {
        if (d := tryParseOm(alloc, bad_docs[bi])) {
            let dm = d;
            dm.deinit(alloc);
            expect_true(false)?;
        }
    }
}

// ---------------------------------------------------------------------------
// Test 3: 4-Lane & 16-Lane SIMD vs Scalar 0-Diff Verification
// ---------------------------------------------------------------------------

fn runTestSimdZeroDiff(): void! {
    // Part A: 4-lane u64 saturating merge & sum parity (lengths 0..=37 including overflow lanes)
    let src: [37]u64 = [0; 37];
    let dst_simd: [37]u64 = [0; 37];
    let dst_scalar: [37]u64 = [0; 37];

    let len: usize = 0;
    while (len <= 37) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) {
            if ((i % 5) == 0) {
                dst_simd[i] = std.math.maxInt(u64) - 15;
                dst_scalar[i] = std.math.maxInt(u64) - 15;
                src[i] = 100;
            } else {
                dst_simd[i] = (i as u64) * 17 + 3;
                dst_scalar[i] = (i as u64) * 17 + 3;
                src[i] = (i as u64) * 31 + 7;
            }
        }
        simdHdrMergeBuckets(dst_simd[0..len], src[0..len]);
        scalarHdrMergeBuckets(dst_scalar[0..len], src[0..len]);
        let j: usize = 0;
        while (j < len) : (j += 1) {
            expect_eq(dst_simd[j], dst_scalar[j]);
        }
        expect_eq(simdHdrSumBuckets(dst_simd[0..len]), scalarHdrSumBuckets(dst_scalar[0..len]));
        expect_eq(simdHdrSumBuckets(src[0..len]), scalarHdrSumBuckets(src[0..len]));
    }

    // Part B: 16-lane u8 OpenMetrics escape/control scanner parity (lengths 0..=80)
    let sbuf: [80]u8 = [0; 80];
    let slen: usize = 0;
    while (slen <= 80) : (slen += 1) {
        let k: usize = 0;
        while (k < slen) : (k += 1) {
            sbuf[k] = (97 as u8) + ((k % 26) as u8);
        }
        expect_eq(simdOmFindNeedEscape(sbuf[0..slen]), scalarOmFindNeedEscape(sbuf[0..slen]));
        expect_true(simdOmFindNeedEscape(sbuf[0..slen]) == null)?;

        if (slen > 0 and (slen <= 34 or slen == 48 or slen == 64 or slen == 80)) {
            const specials: [5]u8 = ['"', '\\', '\n', '\r', 0x7F];
            let sp: usize = 0;
            while (sp < specials.len) : (sp += 1) {
                let pos: usize = 0;
                while (pos < slen) : (pos += 1) {
                    const saved = sbuf[pos];
                    sbuf[pos] = specials[sp];
                    expect_eq(simdOmFindNeedEscape(sbuf[0..slen]), scalarOmFindNeedEscape(sbuf[0..slen]));
                    expect_eq(simdOmFindNeedEscape(sbuf[0..slen]).?, pos);
                    sbuf[pos] = saved;
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Test 4: 1,600-Case Deterministic PRNG Fuzz (800 HDR + 800 OpenMetrics)
// ---------------------------------------------------------------------------

fn runTestObsFuzz1600(): void! {
    const alloc = std.heap.page_allocator;
    let rng = XorShift64 { state: 0x37030B520261124 };

    // Part A: 800 HDR Histogram fuzz ops (recording, clamping, monotonic quantiles, merging)
    let h1 = HdrHistogram.init(HDR_MAX_SUPPORTED_VALUE);
    let h2 = HdrHistogram.init(HDR_MAX_SUPPORTED_VALUE);
    let i: usize = 0;
    while (i < 800) : (i += 1) {
        const raw_val = rng.next() & 0x0000000FFFFFFFFF; // up to ~68B (triggers both in-range and clamped)
        if ((i & 1) == 0) {
            * = h1.recordValue(raw_val);
        } else {
            * = h2.recordValue(raw_val);
        }
        if ((i % 40) == 39) {
            expect_true(h1.merge(h2))?;
            h2.reset();
            const s = h1.summarize();
            expect_eq(s.total_count, simdHdrSumBuckets(h1.buckets[0..]));
            expect_true(s.min_val <= s.p50)?;
            expect_true(s.p50 <= s.p90)?;
            expect_true(s.p90 <= s.p95)?;
            expect_true(s.p95 <= s.p99)?;
            expect_true(s.p99 <= s.p999)?;
            expect_true(s.p999 <= s.max_val)?;
        }
    }

    // Part B: 800 OpenMetrics fuzz cases (random malformed text & control bytes; 0 panic / 0 leak)
    let buf: [160]u8 = [0; 160];
    let oi: usize = 0;
    while (oi < 800) : (oi += 1) {
        const flen = (rng.next() as usize) % 160;
        let j: usize = 0;
        while (j < flen) : (j += 1) {
            const r = rng.next() % 10;
            if (r == 0) {
                buf[j] = '\n';
            } else if (r == 1) {
                buf[j] = '{';
            } else if (r == 2) {
                buf[j] = '}';
            } else if (r == 3) {
                buf[j] = '"';
            } else if (r == 4) {
                buf[j] = '\\';
            } else if (r == 5) {
                buf[j] = '#';
            } else {
                buf[j] = (rng.next() & 0x7F) as u8;
            }
        }
        const sl = buf[0..flen];
        // Also verify SIMD vs scalar escape finder on every fuzz buffer
        expect_eq(simdOmFindNeedEscape(sl), scalarOmFindNeedEscape(sl));
        if (doc := tryParseOm(alloc, sl)) {
            let dm = doc;
            dm.deinit(alloc);
        }
    }
}

fn main() {
    test("HdrHistogram precision, clamping, monotonicity & saturating merge", () => {
        runTestHdrHistogramStrict()?;
    });
    test("OpenMetrics 1.0 strict names, escaping, special floats & control-char rejection", () => {
        runTestOpenMetricsStrict()?;
    });
    test("4-lane & 16-lane SIMD vs Scalar 0-diff differential verification", () => {
        runTestSimdZeroDiff()?;
    });
    test("1600-case deterministic PRNG Fuzz (800 HDR + 800 OpenMetrics)", () => {
        runTestObsFuzz1600()?;
    });
}
