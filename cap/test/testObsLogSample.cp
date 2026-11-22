// Origin: Pure-CP Stdlib Test Suite (lib/obs/logSample.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Adaptive Structured Log Sampler & Storm Suppressor:
//   1. Exact Zap-style initial_burst + thereafter_every (1-in-N) & drop-all (thereafter_every = 0)
//   2. Severity bypass policy (critical/fatal always emitted) & per-(level, hash) bucket isolation
//   3. Window rollover dropped_in_prev_window reporting, drainSummaries() & flushExpiredWindows()
//   4. 64-bucket LRU eviction preserving storm suppression summaries before slot reclamation
//   5. Backwards-clock resilience & reset() full counter/state cleanup
//   6. Invalid config guards, deterministic logSignatureHash & cap.obs namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    LOG_SAMPLER_MAX_BUCKETS,
    LOG_SAMPLER_MAX_SUMMARIES,
    logSignatureHash,
    LogSampleLevel,
    LogSampleVerdict,
    LogSamplerConfig,
    LogSampleDecision,
    LogSuppressionSummary,
    LogSamplerStats,
    AdaptiveLogSampler,
} from "cap:obs";

test("LogSampler: exact initial_burst + thereafter_every 1-in-N & drop-all mode") {
    let cfg = LogSamplerConfig.defaults();
    cfg.window_ms = 1000;
    cfg.initial_burst = 3;
    cfg.thereafter_every = 5;

    let sampler = AdaptiveLogSampler.init(cfg) orelse {
        expect_true(false);
        return;
    };

    // 15 identical events in one window:
    //   #1, #2, #3 -> emit_burst (3)
    //   #4..#7     -> dropped (4)
    //   #8         -> emit_sampled (1)  [(8-3)%5 == 0]
    //   #9..#12    -> dropped (4)
    //   #13        -> emit_sampled (1)  [(13-3)%5 == 0]
    //   #14, #15   -> dropped (2)
    let burst_cnt: usize = 0;
    let sampled_cnt: usize = 0;
    let dropped_cnt: usize = 0;

    let i: usize = 0;
    while (i < 15) : (i += 1) {
        const d = sampler.shouldLogMessage(LogSampleLevel.warn, "db connection refused", 1000 + (i as u64));
        if (d.verdict == LogSampleVerdict.emit_burst) {
            expect_true(d.should_emit);
            burst_cnt += 1;
        } else if (d.verdict == LogSampleVerdict.emit_sampled) {
            expect_true(d.should_emit);
            sampled_cnt += 1;
        } else {
            expect_eq(d.should_emit, false);
            dropped_cnt += 1;
        }
    }
    expect_eq(burst_cnt, 3 as usize);
    expect_eq(sampled_cnt, 2 as usize);
    expect_eq(dropped_cnt, 10 as usize);

    // thereafter_every == 0 -> drop everything after initial_burst
    let cfg_drop = LogSamplerConfig.defaults();
    cfg_drop.initial_burst = 2;
    cfg_drop.thereafter_every = 0;
    let s2 = AdaptiveLogSampler.init(cfg_drop) orelse { expect_true(false); return; };
    let emitted2: usize = 0;
    let j: usize = 0;
    while (j < 20) : (j += 1) {
        if (s2.shouldLog(LogSampleLevel.info, 42, 5000).should_emit) {
            emitted2 += 1;
        }
    }
    expect_eq(emitted2, 2 as usize);
    expect_eq(s2.stats().total_dropped, 18 as u64);
}

test("LogSampler: severity bypass (critical/fatal) & independent (level, hash) buckets") {
    let cfg = LogSamplerConfig.defaults();
    cfg.initial_burst = 1;
    cfg.thereafter_every = 0;
    cfg.bypass_min_level = LogSampleLevel.critical;
    cfg.enable_bypass = true;

    let sampler = AdaptiveLogSampler.init(cfg) orelse {
        expect_true(false);
        return;
    };

    // Critical & fatal always bypass suppression without consuming bucket slots
    let i: usize = 0;
    while (i < 25) : (i += 1) {
        const d1 = sampler.shouldLogMessage(LogSampleLevel.critical, "disk full", 1000);
        const d2 = sampler.shouldLogMessage(LogSampleLevel.fatal, "panic", 1000);
        expect_eq(d1.verdict, LogSampleVerdict.emit_bypass);
        expect_eq(d2.verdict, LogSampleVerdict.emit_bypass);
    }
    expect_eq(sampler.stats().total_bypassed, 50 as u64);
    expect_eq(sampler.stats().active_buckets, 0 as usize);

    // Same message at different levels (info vs err) occupies separate buckets
    const sig: u64 = logSignatureHash("retrying request");
    expect_true(sampler.shouldLog(LogSampleLevel.info, sig, 1000).should_emit);
    expect_eq(sampler.shouldLog(LogSampleLevel.info, sig, 1001).should_emit, false);
    expect_true(sampler.shouldLog(LogSampleLevel.err, sig, 1002).should_emit);
    expect_eq(sampler.stats().active_buckets, 2 as usize);
}

test("LogSampler: window rollover suppression reporting, drainSummaries & flushExpiredWindows") {
    let cfg = LogSamplerConfig.defaults();
    cfg.window_ms = 1000;
    cfg.initial_burst = 3;
    cfg.thereafter_every = 10;

    let sampler = AdaptiveLogSampler.init(cfg) orelse {
        expect_true(false);
        return;
    };

    // Window 1 [1000, 2000): 20 events -> 3 burst + 1 sampled (#13) = 4 emitted, 16 dropped
    let i: usize = 0;
    while (i < 20) : (i += 1) {
        * = sampler.shouldLogMessage(LogSampleLevel.warn, "upstream timeout", 1000 + (i as u64));
    }

    // Window 2 [2000, 3000): first event rolls window and reports 16 dropped in previous window!
    const rolled = sampler.shouldLogMessage(LogSampleLevel.warn, "upstream timeout", 2000);
    expect_true(rolled.window_rolled);
    expect_true(rolled.should_emit);
    expect_eq(rolled.verdict, LogSampleVerdict.emit_burst);
    expect_eq(rolled.dropped_in_prev_window, 16 as u64);
    expect_eq(rolled.window_seen_count, 1 as u64);

    // Another bucket experiences a storm then goes silent -> flushExpiredWindows reports it
    let k: usize = 0;
    while (k < 10) : (k += 1) {
        * = sampler.shouldLogMessage(LogSampleLevel.info, "cache miss", 2100);
    }
    expect_eq(sampler.flushExpiredWindows(3200), 1 as usize);

    let sums: [8]LogSuppressionSummary = [LogSuppressionSummary{}; 8];
    let sums_s: []LogSuppressionSummary = sums[0..];
    const n_sum: usize = sampler.drainSummaries(*sums_s);
    expect_eq(n_sum, 2 as usize);

    // Summary 0: upstream timeout (20 seen, 4 emitted, 16 dropped)
    expect_eq(sums[0].total_seen, 20 as u64);
    expect_eq(sums[0].emitted_count, 4 as u64);
    expect_eq(sums[0].dropped_count, 16 as u64);

    // Summary 1: cache miss (10 seen, 3 emitted, 7 dropped)
    expect_eq(sums[1].total_seen, 10 as u64);
    expect_eq(sums[1].emitted_count, 3 as u64);
    expect_eq(sums[1].dropped_count, 7 as u64);

    // Drained queue is now empty
    expect_eq(sampler.drainSummaries(*sums_s), 0 as usize);
}

test("LogSampler: 64-bucket LRU eviction preserves storm summary before reclaiming slot") {
    let cfg = LogSamplerConfig.defaults();
    cfg.window_ms = 60000;
    cfg.initial_burst = 1;
    cfg.thereafter_every = 0;

    let sampler = AdaptiveLogSampler.init(cfg) orelse {
        expect_true(false);
        return;
    };

    // Bucket with hash 1000 is created first and drops 5 events
    let c: usize = 0;
    while (c < 6) : (c += 1) {
        * = sampler.shouldLog(LogSampleLevel.warn, 1000, 1000);
    }

    // Fill remaining 63 buckets (hashes 1001..1063) -> bucket 1000 is now strict LRU
    let i: usize = 1;
    while (i < LOG_SAMPLER_MAX_BUCKETS) : (i += 1) {
        const d = sampler.shouldLog(LogSampleLevel.info, (1000 + i) as u64, 1000);
        expect_eq(d.evicted_lru, false);
    }
    expect_eq(sampler.stats().active_buckets, LOG_SAMPLER_MAX_BUCKETS);

    // 65th distinct fingerprint evicts LRU bucket 1000 and archives its 5 dropped events
    const d_evict = sampler.shouldLog(LogSampleLevel.info, 9999, 1050);
    expect_true(d_evict.evicted_lru);
    expect_true(d_evict.should_emit);
    expect_eq(sampler.stats().total_evictions, 1 as u64);

    let sums: [4]LogSuppressionSummary = [LogSuppressionSummary{}; 4];
    let sums_s: []LogSuppressionSummary = sums[0..];
    const n_s: usize = sampler.drainSummaries(*sums_s);
    expect_eq(n_s, 1 as usize);
    expect_eq(sums[0].signature_hash, 1000 as u64);
    expect_eq(sums[0].dropped_count, 5 as u64);
}

test("LogSampler: backwards clock safety & reset() full cleanup") {
    let cfg = LogSamplerConfig.defaults();
    cfg.window_ms = 1000;
    cfg.initial_burst = 2;
    cfg.thereafter_every = 0;

    let sampler = AdaptiveLogSampler.init(cfg) orelse {
        expect_true(false);
        return;
    };

    expect_true(sampler.shouldLog(LogSampleLevel.info, 7, 5000).should_emit);
    expect_true(sampler.shouldLog(LogSampleLevel.info, 7, 4500).should_emit); // clock stepped backwards -> no underflow/roll
    const d3 = sampler.shouldLog(LogSampleLevel.info, 7, 4900);
    expect_eq(d3.should_emit, false);
    expect_eq(d3.window_rolled, false);

    sampler.reset();
    const st = sampler.stats();
    expect_eq(st.active_buckets, 0 as usize);
    expect_eq(st.total_evaluated, 0 as u64);
    expect_eq(st.total_emitted, 0 as u64);
    expect_eq(st.total_dropped, 0 as u64);
}

test("LogSampler: invalid config guards, deterministic hash & cap.obs facade parity") {
    let bad = LogSamplerConfig.defaults();
    bad.window_ms = 0;
    expect_true(AdaptiveLogSampler.init(bad) == null);

    // Deterministic FNV-1a signature hashing
    expect_eq(logSignatureHash("hello"), logSignatureHash("hello"));
    expect_true(logSignatureHash("hello") != logSignatureHash("world"));

    // Verify cap.obs namespace facade parity
    expect_eq(cap.obs.LOG_SAMPLER_MAX_BUCKETS, LOG_SAMPLER_MAX_BUCKETS);
    expect_eq(cap.obs.LOG_SAMPLER_MAX_SUMMARIES, LOG_SAMPLER_MAX_SUMMARIES);
    expect_eq(cap.obs.logSignatureHash("cap"), logSignatureHash("cap"));
}
