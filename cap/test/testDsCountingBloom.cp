// Origin: Pure-CP Stdlib Test Suite (lib/ds/countingBloom.cp) | Created: 2026-11-23
// Production Deep Verification Suite for 4-Bit Saturating Counting Bloom Filter with Deletion:
//   1. Basic insert / contains / minCount / remove lifecycle (both string & u64 keys)
//   2. Absent-key remove guard (must NEVER decrement buckets or corrupt co-located keys)
//   3. 4-bit saturation protection (counter at 15 stays saturated on remove -> zero false negatives)
//   4. Saturating union `mergeAdd` & mismatched configuration rejection
//   5. Cardinality estimation (`estimatedItems`), theoretical FPR (`estimatedFpr`) & `clear()`
//   6. Invalid parameter guards (non-power-of-two, out-of-range m/k) & `cap.ds` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    CBLOOM_MAX_BUCKETS,
    CBLOOM_MIN_BUCKETS,
    CBLOOM_STORAGE_BYTES,
    CBLOOM_MAX_HASHES,
    CBLOOM_MAX_COUNTER,
    CountingBloomFilter,
} from "cap:ds";

test("CountingBloom: insert, contains, minCount & clean deletion lifecycle") {
    let bf = CountingBloomFilter.init(1024, 4) ?? return;

    expect_true(bf.insert("alpha"));
    expect_true(bf.insert("beta"));
    expect_eq(bf.insert("alpha"), false); // second insert sees all buckets non-zero

    expect_true(bf.contains("alpha"));
    expect_true(bf.contains("beta"));
    expect_eq(bf.contains("gamma"), false);
    expect_eq(bf.minCount("alpha"), 2 as u8);
    expect_eq(bf.minCount("beta"), 1 as u8);

    // First remove decrements "alpha" from 2 -> 1 (still present)
    expect_true(bf.remove("alpha"));
    expect_true(bf.contains("alpha"));
    expect_eq(bf.minCount("alpha"), 1 as u8);

    // Second remove decrements "alpha" from 1 -> 0 (cleanly deleted)
    expect_true(bf.remove("alpha"));
    expect_eq(bf.contains("alpha"), false);
    expect_true(bf.contains("beta")); // untouched

    // u64 convenience API
    expect_true(bf.insertU64(0xDEAD_BEEF_1234));
    expect_true(bf.containsU64(0xDEAD_BEEF_1234));
    expect_true(bf.removeU64(0xDEAD_BEEF_1234));
    expect_eq(bf.containsU64(0xDEAD_BEEF_1234), false);
}

test("CountingBloom: absent-key remove guard protects co-located keys") {
    let bf = CountingBloomFilter.init(512, 4) ?? return;

    // Insert 40 keys
    let i: u64 = 1;
    while (i <= 40) : (i += 1) {
        * = bf.insertU64(i);
    }
    const nz_before = bf.nonZeroBuckets();

    // Attempt removing 20 absent keys (1001..1020) -> rejected without decrementing any bucket
    let j: u64 = 1001;
    let rejected: u64 = 0;
    while (j <= 1020) : (j += 1) {
        if (!bf.removeU64(j)) {
            rejected += 1;
        }
    }
    expect_true(rejected >= 18); // at low fill, virtually all are rejected
    expect_eq(bf.stats().rejected_removes, rejected);
    expect_eq(bf.nonZeroBuckets(), nz_before);

    // Every originally inserted key must still be present (0 false negatives)
    let k: u64 = 1;
    while (k <= 40) : (k += 1) {
        expect_true(bf.containsU64(k));
    }
}

test("CountingBloom: 4-bit saturation protection guarantees zero false negatives") {
    let bf = CountingBloomFilter.init(256, 3) ?? return;

    // Insert "hot-key" 20 times (exceeds 4-bit max 15)
    let i: usize = 0;
    while (i < 20) : (i += 1) {
        * = bf.insert("hot-key");
    }
    expect_eq(bf.minCount("hot-key"), CBLOOM_MAX_COUNTER);
    expect_true(bf.stats().saturated_buckets >= 3);
    expect_true(bf.stats().saturation_hits > 0);

    // Removing "hot-key" 25 times MUST keep saturated buckets at 15 (never drops to 0!)
    let r: usize = 0;
    while (r < 25) : (r += 1) {
        expect_true(bf.remove("hot-key"));
    }
    expect_true(bf.contains("hot-key"));
    expect_eq(bf.minCount("hot-key"), CBLOOM_MAX_COUNTER);
}

test("CountingBloom: saturating union mergeAdd & configuration mismatch rejection") {
    let a = CountingBloomFilter.init(512, 4) ?? return;
    let b = CountingBloomFilter.init(512, 4) ?? return;
    let other_cfg = CountingBloomFilter.init(256, 4) ?? return;

    // Insert 10 copies of "shared" in a and 10 copies in b -> merged saturates cleanly at 15
    let i: usize = 0;
    while (i < 10) : (i += 1) {
        * = a.insert("shared");
        * = b.insert("shared");
    }
    * = a.insert("only-a");
    * = b.insert("only-b");

    expect_eq(a.mergeAdd(other_cfg), false);
    expect_true(a.mergeAdd(b));

    expect_true(a.contains("only-a"));
    expect_true(a.contains("only-b"));
    expect_true(a.contains("shared"));
    expect_eq(a.minCount("shared"), CBLOOM_MAX_COUNTER);
}

test("CountingBloom: cardinality estimation, theoretical FPR & clear()") {
    let bf = CountingBloomFilter.initDefault();
    expect_eq(bf.nonZeroBuckets(), 0 as usize);
    expect_eq(bf.estimatedItems(), 0.0 as f64);
    expect_eq(bf.estimatedFpr(), 0.0 as f64);

    // Insert 200 distinct u64 keys into 4096-bucket, 4-hash filter
    let i: u64 = 1;
    while (i <= 200) : (i += 1) {
        * = bf.insertU64(i * 7919);
    }

    const st = bf.stats();
    expect_eq(st.num_buckets, CBLOOM_MAX_BUCKETS);
    expect_eq(st.num_hashes, 4 as u8);
    expect_eq(st.total_inserts, 200 as u64);
    // Swamidass & Baldi estimate should be close to 200 (within [160, 240])
    expect_true(st.estimated_items > 160.0 && st.estimated_items < 240.0);
    // Theoretical FPR at 200 items in 4096 buckets with k=4 is ~0.001 (well under 0.02)
    expect_true(st.estimated_fpr > 0.0 && st.estimated_fpr < 0.02);

    bf.clear();
    expect_eq(bf.nonZeroBuckets(), 0 as usize);
    expect_eq(bf.containsU64(7919), false);
    expect_eq(bf.stats().total_inserts, 0 as u64);
}

test("CountingBloom: invalid parameter guards & cap.ds namespace facade parity") {
    // Reject non-power-of-two, too small, too large, and invalid hash_count
    expect_true(CountingBloomFilter.init(0, 4) == null);
    expect_true(CountingBloomFilter.init(8, 4) == null);
    expect_true(CountingBloomFilter.init(100, 4) == null);
    expect_true(CountingBloomFilter.init(8192, 4) == null);
    expect_true(CountingBloomFilter.init(1024, 0) == null);
    expect_true(CountingBloomFilter.init(1024, CBLOOM_MAX_HASHES + 1) == null);

    // cap.ds namespace facade parity
    let facade_bf = cap.ds.CountingBloomFilter.init(CBLOOM_MIN_BUCKETS, 2) ?? return;
    expect_true(facade_bf.insert("ok"));
    expect_true(facade_bf.contains("ok"));
    expect_true(facade_bf.remove("ok"));
    expect_eq(facade_bf.contains("ok"), false);
    expect_eq(CBLOOM_STORAGE_BYTES, 2048 as usize);
}
