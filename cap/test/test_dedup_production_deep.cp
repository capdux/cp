// Source: cp/cap/test/test_dedup_production_deep.cp
// Deep Production Verification Suite for cap:dedup & cap.dedup
// Covers:
//   1. SimHash.computeText & hammingDistance: 1-2 word edits keep distance <= 3, unrelated texts > 15
//   2. SimHashIndex 4x16-bit pigeonhole band index: 220+ docs sublinear exact recall & distance sorting
//   3. MinHash k-shingle signature Jaccard estimation vs exact true k-shingle Jaccard similarity
//   4. LshIndex banding candidate recall, min_jaccard false-positive filtering & std.testing.allocator zero leaks
//   5. `import cap from "cap"; cap.dedup` dynamic mounting verification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    DedupError,
    SimHashHit,
    LshHit,
    SimHash,
    SimHashIndex,
    MinHash,
    LshIndex,
} from "cap:dedup";

fn exactShingleJaccard(alloc: std.mem.Allocator, a: string, b: string, k: usize): f64 {
    let set_a: std.StringHashMapUnmanaged(void) = .empty;
    defer set_a.deinit(alloc);
    let set_b: std.StringHashMapUnmanaged(void) = .empty;
    defer set_b.deinit(alloc);

    if (a.len >= k) {
        let i: usize = 0;
        while (i <= a.len - k) : (i += 1) {
            set_a.put(alloc, a[i .. i + k], {}) catch unreachable;
        }
    }
    if (b.len >= k) {
        let i: usize = 0;
        while (i <= b.len - k) : (i += 1) {
            set_b.put(alloc, b[i .. i + k], {}) catch unreachable;
        }
    }

    let inter: usize = 0;
    let it = set_a.keyIterator();
    while (kp := it.next()) {
        if (set_b.contains(kp.*)) {
            inter += 1;
        }
    }
    const union_cnt = set_a.count() + set_b.count() - inter;
    if (union_cnt == 0) {
        return 1.0;
    }
    return (inter as f64) / (union_cnt as f64);
}

test("cap:dedup - SimHash.computeText & hammingDistance near-duplicate vs unrelated") {
    const base_article =
        "The CP programming language combines TypeScript ergonomics with Rust ownership safety " +
        "and Zig low-level native compilation to power millions of concurrent AI agents on BEAM.";
    const edited_article =
        "The CP programming language combines TypeScript ergonomics with Rust ownership safety " +
        "and Zig low-level native compilation to power billions of concurrent AI actors on BEAM.";
    const unrelated_article =
        "Sourdough bread baking requires careful hydration ratios, wild yeast fermentation, " +
        "crust scoring, and Dutch oven steam retention at high temperatures.";

    const fp_base = SimHash.computeText(base_article);
    const fp_edit = SimHash.computeText(edited_article);
    const fp_other = SimHash.computeText(unrelated_article);

    expect_eq(fp_base != 0, true);
    expect_eq(SimHash.hammingDistance(fp_base, fp_base), 0);

    const dist_near = SimHash.hammingDistance(fp_base, fp_edit);
    const dist_far = SimHash.hammingDistance(fp_base, fp_other);

    expect_eq(dist_near <= 3, true);
    expect_eq(dist_far > 15, true);

    // Case-insensitivity & empty text behavior
    expect_eq(SimHash.computeText(""), 0);
    expect_eq(SimHash.computeText("   ...   !!! "), 0);
    expect_eq(SimHash.computeText("Hello World Agent"), SimHash.computeText("hello world agent"));
}

test("cap:dedup - SimHashIndex 4-band pigeonhole index with 220+ docs") {
    const alloc = std.testing.allocator;
    let idx = SimHashIndex.init(alloc);
    defer idx.deinit();

    // Insert 220 diverse synthetic documents using a stack-formatted doc_id to verify deep ID ownership
    let id_buf: [32]u8 = [0; 32];
    let text_buf: [256]u8 = [0; 256];
    let i: usize = 0;
    while (i < 220) : (i += 1) {
        const doc_id = std.fmt.bufPrint(&id_buf, "noise-{d}", .{i}) catch unreachable;
        const doc_text = std.fmt.bufPrint(
            &text_buf,
            "telemetry shard {d} metric {d} alpha {d} beta {d} gamma {d} delta {d} epsilon {d}",
            .{ i, i * 17 + 3, i * 31 + 7, i * 53 + 11, i * 71 + 13, i * 97 + 19, i * 113 + 23 },
        ) catch unreachable;
        * = idx.add(doc_id, doc_text) catch unreachable;
    }

    const target_doc =
        "Distributed crawler deduplication pipeline filters redundant HTML pages using 64-bit SimHash " +
        "fingerprints and four 16-bit pigeonhole buckets before indexing into vector storage.";
    const target_edited =
        "Distributed crawler deduplication pipeline filters redundant HTML pages using 64-bit SimHash " +
        "fingerprints and four 16-bit pigeonhole buckets before indexing into graph storage.";
    const target_exact_fp = idx.add("target-orig", target_doc) catch unreachable;
    expect_eq(idx.count(), 221);

    // Query exact match + 1-word edit within Hamming distance <= 3
    const hits_exact = idx.queryNearDuplicates(alloc, target_doc, 0) catch unreachable;
    defer alloc.free(hits_exact);
    expect_eq(hits_exact.len, 1);
    expect_eq(std.mem.eql(u8, hits_exact[0].doc_id, "target-orig"), true);
    expect_eq(hits_exact[0].fingerprint, target_exact_fp);
    expect_eq(hits_exact[0].hamming_dist, 0);

    const hits_near = idx.queryNearDuplicates(alloc, target_edited, 3) catch unreachable;
    defer alloc.free(hits_near);
    expect_eq(hits_near.len >= 1, true);
    expect_eq(std.mem.eql(u8, hits_near[0].doc_id, "target-orig"), true);
    expect_eq(hits_near[0].hamming_dist <= 3, true);

    // Invalid threshold > 64 must return DedupError.InvalidThreshold
    let got_err = false;
    * = idx.queryNearDuplicates(alloc, target_doc, 65) catch e => {
        expect_eq(e == DedupError.InvalidThreshold, true);
        got_err = true;
    };
    expect_eq(got_err, true);
}

test("cap:dedup - MinHash k-shingle Jaccard estimation accuracy") {
    const alloc = std.testing.allocator;
    const num_hashes: usize = 128;
    const k_shingle: usize = 4;

    const doc_a = "Retrieval augmented generation corpus cleaning removes near duplicate web documents efficiently.";
    const doc_b = "Retrieval augmented generation corpus cleaning removes near duplicate web pages efficiently.";
    const doc_c = "Quantum mechanics describes nature at the scale of atoms and subatomic particles.";

    let sig_a: [128]u64 = [0; 128];
    let sig_b: [128]u64 = [0; 128];
    let sig_c: [128]u64 = [0; 128];

    MinHash.computeShingles(doc_a, k_shingle, num_hashes, &sig_a) catch unreachable;
    MinHash.computeShingles(doc_b, k_shingle, num_hashes, &sig_b) catch unreachable;
    MinHash.computeShingles(doc_c, k_shingle, num_hashes, &sig_c) catch unreachable;

    const true_ab = exactShingleJaccard(alloc, doc_a, doc_b, k_shingle);
    const est_ab = MinHash.estimateJaccard(&sig_a, &sig_b) catch unreachable;
    const diff_ab = @abs(true_ab - est_ab);
    expect_eq(diff_ab <= 0.12, true);
    expect_eq(est_ab > 0.65, true);

    const true_ac = exactShingleJaccard(alloc, doc_a, doc_c, k_shingle);
    const est_ac = MinHash.estimateJaccard(&sig_a, &sig_c) catch unreachable;
    const diff_ac = @abs(true_ac - est_ac);
    expect_eq(diff_ac <= 0.10, true);
    expect_eq(est_ac < 0.20, true);

    // Self-similarity must be exactly 1.0
    const est_aa = MinHash.estimateJaccard(&sig_a, &sig_a) catch unreachable;
    expect_eq(est_aa, 1.0);
}

test("cap:dedup - LshIndex candidate recall, threshold filtering & zero memory leaks") {
    const alloc = std.testing.allocator;
    // 20 bands x 4 rows = 80 MinHash hashes
    let lsh = LshIndex.init(alloc, 20, 4) catch unreachable;
    defer lsh.deinit();

    lsh.insert("doc-high-1", "Building high throughput async IO loops in pure CP with zero C dependencies.") catch unreachable;
    lsh.insert("doc-high-2", "Building high throughput async IO loops in pure CP with zero C dependency!") catch unreachable;
    lsh.insert("doc-low", "Building scalable distributed databases in Rust and Erlang BEAM.") catch unreachable;
    lsh.insert("doc-unrelated", "Classical symphony orchestra tuning in A440 concert pitch.") catch unreachable;

    expect_eq(lsh.count(), 4);

    const query = "Building high throughput async IO loops in pure CP with zero C dependencies!";
    const hits = lsh.queryCandidates(alloc, query, 0.70) catch unreachable;
    defer alloc.free(hits);

    // Must recall doc-high-1 and doc-high-2, and exclude doc-low / doc-unrelated
    expect_eq(hits.len, 2);
    expect_eq(hits[0].estimated_jaccard >= hits[1].estimated_jaccard, true);
    expect_eq(hits[0].estimated_jaccard >= 0.70, true);
    expect_eq(hits[1].estimated_jaccard >= 0.70, true);
}

test("cap:dedup - import cap from \"cap\"; cap.dedup dynamic mount verification") {
    const alloc = std.testing.allocator;

    const fp1 = cap.dedup.SimHash.computeText("cap.dedup dynamic mount test document alpha");
    const fp2 = cap.dedup.SimHash.computeText("cap.dedup dynamic mount test document alpha");
    expect_eq(cap.dedup.SimHash.hammingDistance(fp1, fp2), 0);

    let idx = cap.dedup.SimHashIndex.init(alloc);
    defer idx.deinit();
    * = idx.add("dyn-1", "cap.dedup dynamic mount test document alpha") catch unreachable;
    const hits = idx.queryNearDuplicates(alloc, "cap.dedup dynamic mount test document alpha", 1) catch unreachable;
    defer alloc.free(hits);
    expect_eq(hits.len, 1);

    let lsh = cap.dedup.LshIndex.init(alloc, 16, 4) catch unreachable;
    defer lsh.deinit();
    lsh.insert("dyn-lsh-1", "cap.dedup dynamic LSH mount verification text") catch unreachable;
    const lsh_hits = lsh.queryCandidates(alloc, "cap.dedup dynamic LSH mount verification text", 0.9) catch unreachable;
    defer alloc.free(lsh_hits);
    expect_eq(lsh_hits.len, 1);
}
