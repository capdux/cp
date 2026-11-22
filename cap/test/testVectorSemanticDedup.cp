// Origin: Pure-CP Stdlib Test Suite (lib/vector/semanticDedup.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Streaming Semantic Near-Duplicate Detector & Online Centroid Clustering:
//   1. Single-pass SIMD `@dot` streaming `ingest`: scale invariance, duplicate hit, canonical_doc_id & min_member_sim
//   2. Online sliding centroid update (`update_centroid = true` vs frozen `false`) preserving unit L2 norm
//   3. Zero-alloc `dedupBatch` corpus cleaning, output buffer truncation safety & `dedup_ratio_bps` telemetry
//   4. `findNearestClusters` descending Top-K retrieval & weighted `mergeClusters` compaction
//   5. Invalid config / NaN / Inf / zero-vector / dimension-mismatch numerical hygiene guards
//   6. 128-cluster capacity saturation, post-merge slot recovery & `cap.vector` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    SEM_DEDUP_MAX_CLUSTERS,
    SEM_DEDUP_MAX_DIM,
    SEM_DEDUP_BPS_SCALE,
    SemanticDedupStatus,
    SemanticDedupConfig,
    SemanticClusterHit,
    SemanticDedupBatchReport,
    SemanticDedupStats,
    SemanticDeduplicator,
} from "cap:vector";

fn absF32(x: f32): f32 {
    return if (x < 0.0) -x else x;
}

test("SemanticDedup: streaming ingest, scale invariance, duplicate canonical_doc_id & min_member_sim") {
    let dedup = SemanticDeduplicator.init(SemanticDedupConfig.make(4, 0.95, true)) ?? return;

    // Unnormalized vectors with same direction should normalize identically (cos = 1.0)
    const v1: [4]f32 = [3.0, 4.0, 0.0, 0.0];
    const v1_scaled: [4]f32 = [6.0, 8.0, 0.0, 0.0];
    const v1_near: [4]f32 = [3.0, 4.2, 0.3, 0.0]; // cos ~= 0.997
    const v2: [4]f32 = [0.0, 0.0, 1.0, 0.0];      // orthogonal (cos = 0.0)

    const r1 = dedup.ingest(1001, v1[0..]);
    expect_eq(r1.status, SemanticDedupStatus.novel);
    expect_true(r1.kept);
    expect_eq(r1.cluster_id, @as(u32, 1));
    expect_eq(r1.canonical_doc_id, @as(u64, 1001));
    expect_eq(r1.cluster_member_count, @as(u32, 1));

    const r2 = dedup.ingest(1002, v1_scaled[0..]);
    expect_eq(r2.status, SemanticDedupStatus.duplicate);
    expect_true(!r2.kept);
    expect_eq(r2.cluster_id, @as(u32, 1));
    expect_eq(r2.canonical_doc_id, @as(u64, 1001));
    expect_true(r2.similarity > 0.999);
    expect_eq(r2.cluster_member_count, @as(u32, 2));

    const r3 = dedup.ingest(1003, v1_near[0..]);
    expect_eq(r3.status, SemanticDedupStatus.duplicate);
    expect_eq(r3.canonical_doc_id, @as(u64, 1001));
    expect_eq(r3.cluster_member_count, @as(u32, 3));

    const r4 = dedup.ingest(2001, v2[0..]);
    expect_eq(r4.status, SemanticDedupStatus.novel);
    expect_eq(r4.cluster_id, @as(u32, 2));
    expect_eq(r4.canonical_doc_id, @as(u64, 2001));

    if (c1 := dedup.getClusterById(1)) {
        expect_eq(c1.member_count, @as(u32, 3));
        expect_true(c1.min_member_sim >= 0.95 and c1.min_member_sim < 1.0);
        expect_true(c1.avgMemberSim() > 0.98);
    } else {
        expect_true(false);
    }
}

test("SemanticDedup: sliding centroid update vs frozen centroid mode") {
    const base: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const tilted: [4]f32 = [0.95, 0.31225, 0.0, 0.0]; // cos = 0.95

    let d_slide = SemanticDeduplicator.init(SemanticDedupConfig.make(4, 0.90, true)) ?? return;
    let d_frozen = SemanticDeduplicator.init(SemanticDedupConfig.make(4, 0.90, false)) ?? return;

    * = d_slide.ingest(1, base[0..]);
    * = d_slide.ingest(2, tilted[0..]);
    * = d_frozen.ingest(1, base[0..]);
    * = d_frozen.ingest(2, tilted[0..]);

    if (cs := d_slide.getClusterById(1)) {
        // Centroid shifted toward tilted (y > 0.1) and remains unit-normalized
        expect_true(cs.centroid[1] > 0.1);
        const norm_sq = cs.centroid[0] * cs.centroid[0] + cs.centroid[1] * cs.centroid[1];
        expect_true(absF32(norm_sq - 1.0) < 0.0001);
    }
    if (cf := d_frozen.getClusterById(1)) {
        // Frozen centroid stays exactly [1, 0, 0, 0]
        expect_true(absF32(cf.centroid[0] - 1.0) < 0.00001);
        expect_true(absF32(cf.centroid[1]) < 0.00001);
    }
}

test("SemanticDedup: dedupBatch corpus cleaning, truncation safety & dedup_ratio_bps") {
    let dedup = SemanticDeduplicator.init(SemanticDedupConfig.make(4, 0.95, true)) ?? return;

    const ids: [6]u64 = [10, 11, 12, 13, 14, 15];
    const vecs: [24]f32 = [
        1.0, 0.0, 0.0, 0.0,  // 10: novel
        0.99, 0.1, 0.0, 0.0, // 11: dup of 10
        0.0, 1.0, 0.0, 0.0,  // 12: novel
        0.0, 0.0, 0.0, 0.0,  // 13: invalid zero vector
        0.0, 0.99, 0.1, 0.0, // 14: dup of 12
        0.0, 0.0, 1.0, 0.0,  // 15: novel
    ];

    // Intentionally pass out_kept_ids of length 2 to verify truncation safety
    let kept: [2]u64 = [0; 2];
    let kept_s: []u64 = kept[0..];
    const rep: SemanticDedupBatchReport = dedup.dedupBatch(ids[0..], vecs[0..], *kept_s);
    expect_eq(rep.processed, @as(usize, 6));
    expect_eq(rep.novel_count, @as(usize, 3));
    expect_eq(rep.duplicate_count, @as(usize, 2));
    expect_eq(rep.invalid_count, @as(usize, 1));
    expect_eq(rep.kept_written, @as(usize, 2)); // capped safely at kept.len
    expect_eq(kept[0], @as(u64, 10));
    expect_eq(kept[1], @as(u64, 12));
    // 2 dups / (3 novel + 2 dup) = 4000 bps (40%)
    expect_eq(rep.dedup_ratio_bps, @as(u32, 4000));
}

test("SemanticDedup: findNearestClusters Top-K & weighted mergeClusters compaction") {
    let dedup = SemanticDeduplicator.init(SemanticDedupConfig.make(4, 0.98, true)) ?? return;

    const va: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const vb: [4]f32 = [0.90, 0.43589, 0.0, 0.0]; // cos(va, vb) = 0.90 (< 0.98 so distinct cluster)
    const vc: [4]f32 = [0.0, 0.0, 1.0, 0.0];      // orthogonal

    * = dedup.ingest(101, va[0..]);
    * = dedup.ingest(102, va[0..]); // cluster 1 has 2 members
    * = dedup.ingest(201, vb[0..]); // cluster 2 has 1 member
    * = dedup.ingest(301, vc[0..]); // cluster 3 has 1 member

    let hits: [3]SemanticClusterHit = [SemanticClusterHit{}; 3];
    let hits_s: []SemanticClusterHit = hits[0..];
    const q: [4]f32 = [0.98, 0.2, 0.0, 0.0];
    const n = dedup.findNearestClusters(q[0..], 2, *hits_s);
    expect_eq(n, @as(usize, 2));
    expect_eq(hits[0].canonical_doc_id, @as(u64, 101));
    expect_eq(hits[1].canonical_doc_id, @as(u64, 201));
    expect_true(hits[0].similarity > hits[1].similarity);

    // Compact clusters with looser merge_threshold = 0.85 -> merges cluster 2 into cluster 1
    const merged = dedup.mergeClusters(0.85);
    expect_eq(merged, @as(usize, 1));
    expect_eq(dedup.stats().active_clusters, @as(usize, 2));
    expect_eq(dedup.stats().merged_clusters, @as(u64, 1));
    if (c1 := dedup.getClusterById(1)) {
        expect_eq(c1.member_count, @as(u32, 3));
        expect_true(c1.min_member_sim <= 0.91);
    }
}

test("SemanticDedup: numerical hygiene guards (invalid config, NaN/Inf, zero vec, bad dim)") {
    const zero: f32 = 0.0;
    const nan_val: f32 = zero / zero;
    const inf_val: f32 = 1.0 / zero;

    expect_true(SemanticDeduplicator.init(SemanticDedupConfig.make(0, 0.9, true)) == null);
    expect_true(SemanticDeduplicator.init(SemanticDedupConfig.make(SEM_DEDUP_MAX_DIM + 1, 0.9, true)) == null);
    expect_true(SemanticDeduplicator.init(SemanticDedupConfig.make(4, 1.5, true)) == null);
    expect_true(SemanticDeduplicator.init(SemanticDedupConfig.make(4, nan_val, true)) == null);

    let dedup = SemanticDeduplicator.init(SemanticDedupConfig.make(4, 0.9, true)) ?? return;

    const v_zero: [4]f32 = [0.0, 0.0, 0.0, 0.0];
    const v_nan: [4]f32 = [1.0, nan_val, 0.0, 0.0];
    const v_inf: [4]f32 = [1.0, inf_val, 0.0, 0.0];
    const v_short: [2]f32 = [1.0, 0.0];

    expect_eq(dedup.ingest(1, v_zero[0..]).status, SemanticDedupStatus.invalid_vector);
    expect_eq(dedup.ingest(2, v_nan[0..]).status, SemanticDedupStatus.invalid_vector);
    expect_eq(dedup.ingest(3, v_inf[0..]).status, SemanticDedupStatus.invalid_vector);
    expect_eq(dedup.ingest(4, v_short[0..]).status, SemanticDedupStatus.invalid_vector);
    expect_eq(dedup.stats().invalid_count, @as(u64, 4));
    expect_eq(dedup.stats().active_clusters, @as(usize, 0));
}

test("SemanticDedup: 128-cluster saturation, merge recovery & cap.vector facade parity") {
    // Use dim=2 with evenly spaced unit-circle angles (cos(pi/128) ~= 0.9997) and threshold 0.99999
    let dedup = cap.vector.SemanticDeduplicator.init(
        cap.vector.SemanticDedupConfig.make(2, 0.99999, false),
    ) ?? return;

    let i: usize = 0;
    while (i < SEM_DEDUP_MAX_CLUSTERS) : (i += 1) {
        const angle: f32 = @as(f32, @floatFromInt(i)) * 0.049087385; // 2*pi / 128
        const vec: [2]f32 = [@cos(angle), @sin(angle)];
        const r = dedup.ingest(@as(u64, @intCast(i + 1)), vec[0..]);
        expect_eq(r.status, SemanticDedupStatus.novel);
    }

    expect_eq(dedup.stats().active_clusters, SEM_DEDUP_MAX_CLUSTERS);

    // 129th distinct angle hits capacity_full
    const extra: [2]f32 = [@cos(@as(f32, 0.0245)), @sin(@as(f32, 0.0245))];
    const r_full = dedup.ingest(999, extra[0..]);
    expect_eq(r_full.status, SemanticDedupStatus.capacity_full);
    expect_eq(dedup.stats().capacity_full_count, @as(u64, 1));

    // Compacting nearby clusters frees slots so new novel vectors can be ingested again
    const freed = dedup.mergeClusters(0.99);
    expect_true(freed > 0);
    const st: SemanticDedupStats = dedup.stats();
    expect_true(st.active_clusters < SEM_DEDUP_MAX_CLUSTERS);
    expect_eq(st.cluster_capacity, SEM_DEDUP_MAX_CLUSTERS);
    expect_eq(SEM_DEDUP_BPS_SCALE, @as(u32, 10_000));
}
