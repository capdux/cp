// Production Deep Test Suite for cap:vector RAPTOR Hierarchical Summary Tree (lib/vector/raptor_tree.cp)
import { test, expect_eq, expect_true } from "cap:test";
import {
    RAPTOR_MAX_NODES,
    RAPTOR_MAX_LEVELS,
    RAPTOR_NO_NODE,
    RaptorNodeKind,
    RaptorSearchMode,
    RaptorScoreFusion,
    RaptorTreeConfig,
    RaptorSearchHit,
    RaptorTree,
} from "cap:vector";

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    const diff: f32 = if (a >= b) (a - b) else (b - a);
    return diff <= eps;
}

// ============================================================================
// Test 1: Multi-level tree build, mean summary synthesis & iterative BFS/DFS
// ============================================================================
test("raptor_tree: multi-level build, children mean synthesis & iterative BFS/DFS traversal", fn() {
    let vec_buf: [128]f32 = [0.0; 128]; // 32 nodes x 4D
    let vec_slice: []f32 = vec_buf[0..];
    const cfg = RaptorTreeConfig{
        dim: 4,
        max_nodes: 32,
    };
    let tree = RaptorTree.init(cfg, *vec_slice) ?? return;

    const e_a1: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const e_a2: [4]f32 = [0.8, 0.6, 0.0, 0.0];
    const e_b1: [4]f32 = [0.0, 0.0, 1.0, 0.0];
    const e_b2: [4]f32 = [0.0, 0.0, 0.6, 0.8];

    const l0 = tree.addLeaf(10, e_a1[0..], 100) ?? RAPTOR_NO_NODE;
    const l1 = tree.addLeaf(11, e_a2[0..], 120) ?? RAPTOR_NO_NODE;
    const l2 = tree.addLeaf(12, e_b1[0..], 90) ?? RAPTOR_NO_NODE;
    const l3 = tree.addLeaf(13, e_b2[0..], 110) ?? RAPTOR_NO_NODE;
    expect_eq(l0, 0 as u16);
    expect_eq(l3, 3 as u16);

    const ch_a: [2]u16 = [l0, l1];
    const ch_b: [2]u16 = [l2, l3];
    const s_a = tree.addSummaryFromChildrenMean(100, 1, RaptorNodeKind.cluster_summary, 60, ch_a[0..]) ?? RAPTOR_NO_NODE;
    const s_b = tree.addSummaryFromChildrenMean(101, 1, RaptorNodeKind.cluster_summary, 55, ch_b[0..]) ?? RAPTOR_NO_NODE;
    expect_eq(s_a, 4 as u16);
    expect_eq(s_b, 5 as u16);

    const ch_root: [2]u16 = [s_a, s_b];
    const root_vec: [4]f32 = [0.5, 0.5, 0.5, 0.5];
    const root = tree.addSummaryNode(1000, 2, RaptorNodeKind.root_abstraction, root_vec[0..], 40, ch_root[0..]) ?? RAPTOR_NO_NODE;
    expect_eq(root, 6 as u16);

    // Check stats
    const st = tree.stats();
    expect_eq(st.total_nodes, 7 as usize);
    expect_eq(st.max_level, 2 as u8);
    expect_eq(st.leaf_count, 4 as usize);
    expect_eq(st.summary_count, 2 as usize);
    expect_eq(st.root_count, 1 as usize);
    expect_eq(st.level_counts[0], 4 as usize);
    expect_eq(st.level_counts[1], 2 as usize);
    expect_eq(st.level_counts[2], 1 as usize);
    expect_eq(st.total_tokens, 575 as u64);

    // Check mean summary normalization
    if (s_a_emb := tree.getEmbedding(s_a)) {
        const norm_sq: f32 = @dot(s_a_emb, s_a_emb);
        expect_true(approxEqF32(norm_sq, 1.0, 1.0e-5));
    } else {
        expect_true(false);
    }

    // Iterative BFS from root: root -> s_a, s_b -> l0, l1, l2, l3
    let bfs_out: [16]u16 = [RAPTOR_NO_NODE; 16];
    let bfs_s: []u16 = bfs_out[0..];
    const bfs_n = tree.traverseBfs(root, *bfs_s);
    expect_eq(bfs_n, 7 as usize);
    expect_eq(bfs_out[0], root);
    expect_eq(bfs_out[1], s_a);
    expect_eq(bfs_out[2], s_b);
    expect_eq(bfs_out[3], l0);
    expect_eq(bfs_out[4], l1);
    expect_eq(bfs_out[5], l2);
    expect_eq(bfs_out[6], l3);

    // Iterative pre-order DFS from root: root -> s_a -> l0 -> l1 -> s_b -> l2 -> l3
    let dfs_out: [16]u16 = [RAPTOR_NO_NODE; 16];
    let dfs_s: []u16 = dfs_out[0..];
    const dfs_n = tree.traverseDfs(root, *dfs_s);
    expect_eq(dfs_n, 7 as usize);
    expect_eq(dfs_out[0], root);
    expect_eq(dfs_out[1], s_a);
    expect_eq(dfs_out[2], l0);
    expect_eq(dfs_out[3], l1);
    expect_eq(dfs_out[4], s_b);
    expect_eq(dfs_out[5], l2);
    expect_eq(dfs_out[6], l3);

    // Ancestors of l1: s_a -> root
    let anc_out: [8]u16 = [RAPTOR_NO_NODE; 8];
    let anc_s: []u16 = anc_out[0..];
    const anc_n = tree.collectAncestors(l1, *anc_s);
    expect_eq(anc_n, 2 as usize);
    expect_eq(anc_out[0], s_a);
    expect_eq(anc_out[1], root);
});

// ============================================================================
// Test 2: Incremental cosine cluster assignment & @fma centroid update
// ============================================================================
test("raptor_tree: incremental cosine cluster assignment & FMA centroid update", fn() {
    let vec_buf: [64]f32 = [0.0; 64]; // 16 nodes x 4D
    let vec_slice: []f32 = vec_buf[0..];
    const cfg = RaptorTreeConfig{
        dim: 4,
        max_nodes: 16,
        cluster_sim_threshold: 0.70,
    };
    let tree = RaptorTree.init(cfg, *vec_slice) ?? return;

    const empty_ch: [0]u16 = [];
    const c_x_vec: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const c_y_vec: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    const c_x = tree.addSummaryNode(200, 1, RaptorNodeKind.cluster_summary, c_x_vec[0..], 50, empty_ch[0..]) ?? RAPTOR_NO_NODE;
    const c_y = tree.addSummaryNode(201, 1, RaptorNodeKind.cluster_summary, c_y_vec[0..], 50, empty_ch[0..]) ?? RAPTOR_NO_NODE;

    // New leaf close to X-axis should attach to c_x and shift c_x centroid via @fma
    const new_x_leaf: [4]f32 = [0.8, 0.6, 0.0, 0.0];
    if (asgn := tree.assignLeafIncremental(10, new_x_leaf[0..], 80, 1, true)) {
        expect_true(asgn.matched_existing);
        expect_eq(asgn.target_parent_slot, c_x);
        expect_true(approxEqF32(asgn.similarity, 0.8, 1.0e-5));
        // Unit-sphere l2_sq between [1,0,0,0] and [0.8,0.6,0,0] is (0.2)^2 + (0.6)^2 = 0.4
        expect_true(approxEqF32(asgn.l2_sq_dist, 0.4, 1.0e-4));

        if (leaf_node := tree.getNode(asgn.leaf_slot)) {
            expect_eq(leaf_node.parent_idx, c_x);
        } else {
            expect_true(false);
        }
    } else {
        expect_true(false);
    }

    // Verify c_x centroid moved toward +Y and stayed unit-normalized
    if (updated_cx := tree.getEmbedding(c_x)) {
        expect_true(updated_cx[1] > 0.25);
        expect_true(approxEqF32(@dot(updated_cx, updated_cx), 1.0, 1.0e-5));
    } else {
        expect_true(false);
    }

    // Orthogonal leaf along Z-axis should fail threshold (0.0 < 0.70) and stay unattached
    const ortho_leaf: [4]f32 = [0.0, 0.0, 1.0, 0.0];
    if (asgn2 := tree.assignLeafIncremental(11, ortho_leaf[0..], 75, 1, true)) {
        expect_true(!asgn2.matched_existing);
        expect_eq(asgn2.target_parent_slot, RAPTOR_NO_NODE);
        if (ln := tree.getNode(asgn2.leaf_slot)) {
            expect_eq(ln.parent_idx, RAPTOR_NO_NODE);
        } else {
            expect_true(false);
        }
    } else {
        expect_true(false);
    }
    expect_eq(c_y, 1 as u16);
});

// ============================================================================
// Test 3: Collapsed Tree joint retrieval with Linear & RRF level-weighted fusion
// ============================================================================
test("raptor_tree: collapsed tree joint retrieval with linear & RRF level-weighted fusion", fn() {
    let vec_buf: [64]f32 = [0.0; 64]; // 16 nodes x 4D
    let vec_slice: []f32 = vec_buf[0..];
    const cfg = RaptorTreeConfig{
        dim: 4,
        max_nodes: 16,
        rrf_k: 10.0,
    };
    let tree = RaptorTree.init(cfg, *vec_slice) ?? return;

    const e_leaf_best: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const e_leaf_mid: [4]f32 = [0.7, 0.7, 0.0, 0.0];
    const e_sum: [4]f32 = [0.9, 0.43589, 0.0, 0.0]; // cos ~ 0.9 with [1,0,0,0]
    const e_root: [4]f32 = [0.6, 0.8, 0.0, 0.0];

    const l0 = tree.addLeaf(1, e_leaf_best[0..], 100) ?? RAPTOR_NO_NODE;
    const l1 = tree.addLeaf(2, e_leaf_mid[0..], 100) ?? RAPTOR_NO_NODE;
    const ch: [2]u16 = [l0, l1];
    const s0 = tree.addSummaryNode(10, 1, RaptorNodeKind.cluster_summary, e_sum[0..], 60, ch[0..]) ?? RAPTOR_NO_NODE;
    const rch: [1]u16 = [s0];
    * = tree.addSummaryNode(100, 2, RaptorNodeKind.root_abstraction, e_root[0..], 30, rch[0..]);

    const q: [4]f32 = [2.0, 0.0, 0.0, 0.0]; // unnormalized query to verify scale invariance
    let hits: [4]RaptorSearchHit = [RaptorSearchHit{}; 4];
    let hits_s: []RaptorSearchHit = hits[0..];

    // Equal weights: linear_weighted puts leaf 1 (sim=1.0) ahead of summary 10 (sim~0.9)
    const n1 = tree.search(q[0..], RaptorSearchMode.collapsed_tree, RaptorScoreFusion.linear_weighted, 4, 2, *hits_s);
    expect_eq(n1, 4 as usize);
    expect_eq(hits[0].node_id, 1 as u32);
    expect_eq(hits[1].node_id, 10 as u32);

    // Boost Level 1 summary weight to 2.0 -> summary 10 (2.0 * 0.9 = 1.8) overtakes leaf 1 (1.0 * 1.0 = 1.0)
    expect_true(tree.setLevelWeight(1, 2.0));
    const n2 = tree.search(q[0..], RaptorSearchMode.collapsed_tree, RaptorScoreFusion.linear_weighted, 4, 2, *hits_s);
    expect_eq(n2, 4 as usize);
    expect_eq(hits[0].node_id, 10 as u32);
    expect_eq(hits[1].node_id, 1 as u32);

    // RRF mode: summary 10 is rank 1 in Level 1 -> 2.0 / (10 + 1) = 0.1818 > leaf 1 (1.0 / 11 = 0.0909)
    const n3 = tree.search(q[0..], RaptorSearchMode.collapsed_tree, RaptorScoreFusion.reciprocal_rank, 4, 2, *hits_s);
    expect_eq(n3, 4 as usize);
    expect_eq(hits[0].node_id, 10 as u32);
    expect_true(approxEqF32(hits[0].fused_score, 2.0 / 11.0, 1.0e-5));

    // Level-range filtered collapsed search: Level 0 only
    const n_leaf_only = tree.searchCollapsed(q[0..], 4, RaptorScoreFusion.linear_weighted, 0, 0, *hits_s);
    expect_eq(n_leaf_only, 2 as usize);
    expect_eq(hits[0].level, 0 as u8);
    expect_eq(hits[1].level, 0 as u8);
});

// ============================================================================
// Test 4: Tree Traversal top-down beam pruning vs Collapsed Tree
// ============================================================================
test("raptor_tree: tree traversal top-down beam pruning vs collapsed tree", fn() {
    let vec_buf: [128]f32 = [0.0; 128]; // 32 nodes x 4D
    let vec_slice: []f32 = vec_buf[0..];
    const cfg = RaptorTreeConfig{
        dim: 4,
        max_nodes: 32,
    };
    let tree = RaptorTree.init(cfg, *vec_slice) ?? return;

    // Branch A (aligned with X)
    const e_a_leaf1: [4]f32 = [0.95, 0.31, 0.0, 0.0];
    const e_a_leaf2: [4]f32 = [0.90, 0.0, 0.43, 0.0];
    const l_a1 = tree.addLeaf(1, e_a_leaf1[0..], 100) ?? RAPTOR_NO_NODE;
    const l_a2 = tree.addLeaf(2, e_a_leaf2[0..], 100) ?? RAPTOR_NO_NODE;
    const ch_a: [2]u16 = [l_a1, l_a2];
    const e_sum_a: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const s_a = tree.addSummaryNode(10, 1, RaptorNodeKind.cluster_summary, e_sum_a[0..], 50, ch_a[0..]) ?? RAPTOR_NO_NODE;
    const rch_a: [1]u16 = [s_a];
    * = tree.addSummaryNode(100, 2, RaptorNodeKind.root_abstraction, e_sum_a[0..], 30, rch_a[0..]);

    // Branch B (root & summary aligned with Y, but contains a hidden outlier leaf aligned with X)
    const e_b_outlier: [4]f32 = [0.99, 0.14, 0.0, 0.0];
    const e_b_leaf2: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    const l_b1 = tree.addLeaf(3, e_b_outlier[0..], 100) ?? RAPTOR_NO_NODE;
    const l_b2 = tree.addLeaf(4, e_b_leaf2[0..], 100) ?? RAPTOR_NO_NODE;
    const ch_b: [2]u16 = [l_b1, l_b2];
    const e_sum_b: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    const s_b = tree.addSummaryNode(20, 1, RaptorNodeKind.cluster_summary, e_sum_b[0..], 50, ch_b[0..]) ?? RAPTOR_NO_NODE;
    const rch_b: [1]u16 = [s_b];
    * = tree.addSummaryNode(200, 2, RaptorNodeKind.root_abstraction, e_sum_b[0..], 30, rch_b[0..]);

    const q: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    let hits: [8]RaptorSearchHit = [RaptorSearchHit{}; 8];
    let hits_s: []RaptorSearchHit = hits[0..];

    // Tree Traversal with beam_per_level = 1 prunes Branch B at root level -> node 3 (outlier) is NOT visited
    const n_tt = tree.searchTreeTraversal(q[0..], 1, 8, RaptorScoreFusion.linear_weighted, *hits_s);
    expect_eq(n_tt, 3 as usize); // root 100 -> summary 10 -> best leaf 1
    let saw_branch_b: bool = false;
    let i: usize = 0;
    while (i < n_tt) : (i += 1) {
        if (hits[i].node_id == 3 || hits[i].node_id == 4 || hits[i].node_id == 20 || hits[i].node_id == 200) {
            saw_branch_b = true;
        }
    }
    expect_true(!saw_branch_b);

    // Collapsed Tree evaluates all nodes flatly -> finds outlier leaf 3
    const n_col = tree.searchCollapsed(q[0..], 4, RaptorScoreFusion.linear_weighted, 0, 2, *hits_s);
    expect_eq(n_col, 4 as usize);
    let saw_outlier: bool = false;
    let j: usize = 0;
    while (j < n_col) : (j += 1) {
        if (hits[j].node_id == 3) saw_outlier = true;
    }
    expect_true(saw_outlier);
});

// ============================================================================
// Test 5: Duplicate child deduplication & deterministic tie-breaking
// ============================================================================
test("raptor_tree: duplicate child deduplication & deterministic tie-breaking", fn() {
    let vec_buf: [64]f32 = [0.0; 64];
    let vec_slice: []f32 = vec_buf[0..];
    const cfg = RaptorTreeConfig{ dim: 4, max_nodes: 16 };
    let tree = RaptorTree.init(cfg, *vec_slice) ?? return;

    const identical: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    // Insert with descending node_ids to verify tie-breaking sorts ascending by node_id
    const l_hi = tree.addLeaf(50, identical[0..], 10) ?? RAPTOR_NO_NODE;
    const l_lo = tree.addLeaf(10, identical[0..], 10) ?? RAPTOR_NO_NODE;
    const l_mid = tree.addLeaf(30, identical[0..], 10) ?? RAPTOR_NO_NODE;

    // Pass duplicate child slots to addSummaryNode -> should deduplicate cleanly
    const dup_children: [5]u16 = [l_hi, l_lo, l_hi, l_mid, l_lo];
    const s0 = tree.addSummaryNode(20, 1, RaptorNodeKind.cluster_summary, identical[0..], 20, dup_children[0..]) ?? RAPTOR_NO_NODE;

    if (sn := tree.getNode(s0)) {
        expect_eq(sn.child_count, 3 as u16);
        expect_eq(sn.cluster_size, 3 as u32);
    } else {
        expect_true(false);
    }

    let hits: [4]RaptorSearchHit = [RaptorSearchHit{}; 4];
    let hits_s: []RaptorSearchHit = hits[0..];
    const n = tree.searchCollapsed(identical[0..], 4, RaptorScoreFusion.linear_weighted, 0, 1, *hits_s);
    expect_eq(n, 4 as usize);
    // All scores tie at 1.0 -> deterministic ascending node_id order: 10, 20, 30, 50
    expect_eq(hits[0].node_id, 10 as u32);
    expect_eq(hits[1].node_id, 20 as u32);
    expect_eq(hits[2].node_id, 30 as u32);
    expect_eq(hits[3].node_id, 50 as u32);
});

// ============================================================================
// Test 6: High-dimension (>512D fallback path) scale-invariant retrieval
// ============================================================================
test("raptor_tree: high-dimension (>512D) scale-invariant retrieval", fn() {
    const DIM: usize = 600;
    let vec_buf: [2400]f32 = [0.0; 2400]; // 4 nodes x 600D
    let vec_slice: []f32 = vec_buf[0..];
    const cfg = RaptorTreeConfig{
        dim: DIM,
        max_nodes: 4,
    };
    let tree = RaptorTree.init(cfg, *vec_slice) ?? return;

    let v0: [600]f32 = [0.0; 600];
    let v1: [600]f32 = [0.0; 600];
    let q: [600]f32 = [0.0; 600];
    v0[0] = 3.0;
    v0[599] = 4.0;
    v1[1] = 1.0;
    q[0] = 6.0;
    q[599] = 8.0; // parallel to v0, scaled 2x

    const l0 = tree.addLeaf(1, v0[0..], 100) ?? RAPTOR_NO_NODE;
    const l1 = tree.addLeaf(2, v1[0..], 100) ?? RAPTOR_NO_NODE;
    const ch: [2]u16 = [l0, l1];
    * = tree.addSummaryFromChildrenMean(10, 1, RaptorNodeKind.cluster_summary, 50, ch[0..]);

    let hits: [3]RaptorSearchHit = [RaptorSearchHit{}; 3];
    let hits_s: []RaptorSearchHit = hits[0..];
    const n = tree.searchCollapsed(q[0..], 3, RaptorScoreFusion.linear_weighted, 0, 1, *hits_s);
    expect_eq(n, 3 as usize);
    expect_eq(hits[0].node_id, 1 as u32);
    expect_true(approxEqF32(hits[0].raw_similarity, 1.0, 1.0e-5));
    expect_true(approxEqF32(hits[0].l2_sq_dist, 0.0, 1.0e-4));
});

// ============================================================================
// Test 7: Re-attachment, level-inversion prevention & zero-alloc reset reuse
// ============================================================================
test("raptor_tree: re-attachment, level-inversion prevention & reset reuse", fn() {
    let vec_buf: [64]f32 = [0.0; 64];
    let vec_slice: []f32 = vec_buf[0..];
    const cfg = RaptorTreeConfig{ dim: 4, max_nodes: 16 };
    let tree = RaptorTree.init(cfg, *vec_slice) ?? return;

    const v: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const l0 = tree.addLeaf(1, v[0..], 10) ?? RAPTOR_NO_NODE;
    const ch0: [1]u16 = [l0];
    const s0 = tree.addSummaryNode(10, 1, RaptorNodeKind.cluster_summary, v[0..], 10, ch0[0..]) ?? RAPTOR_NO_NODE;
    const empty: [0]u16 = [];
    const s1 = tree.addSummaryNode(11, 1, RaptorNodeKind.cluster_summary, v[0..], 10, empty[0..]) ?? RAPTOR_NO_NODE;

    // Reject self-link and level inversion (leaf -> summary or summary -> summary)
    expect_true(!tree.attachChild(s0, s0));
    expect_true(!tree.attachChild(l0, s0));
    expect_true(!tree.attachChild(s0, s1));
    expect_true(!tree.attachChild(99, l0));

    // Re-attach l0 from s0 to s1 -> s0 child_count becomes 0, s1 child_count becomes 1
    expect_true(tree.attachChild(s1, l0));
    if (n_s0 := tree.getNode(s0)) {
        expect_eq(n_s0.child_count, 0 as u16);
    } else {
        expect_true(false);
    }
    if (n_s1 := tree.getNode(s1)) {
        expect_eq(n_s1.child_count, 1 as u16);
    } else {
        expect_true(false);
    }

    // Reset and reuse same backing buffer
    tree.reset();
    expect_eq(tree.stats().total_nodes, 0 as usize);
    const fresh = tree.addLeaf(1, v[0..], 42) ?? RAPTOR_NO_NODE;
    expect_eq(fresh, 0 as u16);
    expect_eq(tree.stats().total_nodes, 1 as usize);
});

// ============================================================================
// Test 8: Strict boundary guards (invalid config, NaN/Inf, zero vec, full cap)
// ============================================================================
test("raptor_tree: strict boundary guards (config, NaN/Inf, zero vec, empty query, full cap)", fn() {
    let tiny_buf: [8]f32 = [0.0; 8];
    let tiny_s: []f32 = tiny_buf[0..];

    // Invalid configs
    expect_true(RaptorTree.init(RaptorTreeConfig{ dim: 0, max_nodes: 2 }, *tiny_s) == null);
    expect_true(RaptorTree.init(RaptorTreeConfig{ dim: 4, max_nodes: 0 }, *tiny_s) == null);
    expect_true(RaptorTree.init(RaptorTreeConfig{ dim: 4, max_nodes: RAPTOR_MAX_NODES + 1 }, *tiny_s) == null);
    expect_true(RaptorTree.init(RaptorTreeConfig{ dim: 4, max_nodes: 3 }, *tiny_s) == null); // buf only fits 2*4=8
    expect_true(RaptorTree.init(RaptorTreeConfig{ dim: 4, max_nodes: 2, rrf_k: 0.0 }, *tiny_s) == null);
    expect_true(RaptorTree.init(RaptorTreeConfig{ dim: 4, max_nodes: 2, cluster_sim_threshold: 1.5 }, *tiny_s) == null);

    // Valid capacity=2 tree
    let tree = RaptorTree.init(RaptorTreeConfig{ dim: 4, max_nodes: 2 }, *tiny_s) ?? return;

    const zero_v: [4]f32 = [0.0, 0.0, 0.0, 0.0];
    const wrong_dim: [3]f32 = [1.0, 0.0, 0.0];
    const nan_v: [4]f32 = [0.0 / 0.0, 1.0, 0.0, 0.0];
    const inf_v: [4]f32 = [1.0 / 0.0, 0.0, 0.0, 0.0];
    const good_v: [4]f32 = [1.0, 0.0, 0.0, 0.0];

    expect_true(tree.addLeaf(1, zero_v[0..], 10) == null);
    expect_true(tree.addLeaf(1, wrong_dim[0..], 10) == null);
    expect_true(tree.addLeaf(1, nan_v[0..], 10) == null);
    expect_true(tree.addLeaf(1, inf_v[0..], 10) == null);

    // Invalid level weight mutations
    expect_true(!tree.setLevelWeight(RAPTOR_MAX_LEVELS as u8, 1.0));
    expect_true(!tree.setLevelWeight(0, -0.5));
    expect_true(!tree.setLevelWeight(0, 0.0 / 0.0));

    // Fill capacity=2 & reject duplicate node_id and overflow
    expect_true(tree.addLeaf(1, good_v[0..], 10) != null);
    expect_true(tree.addLeaf(1, good_v[0..], 10) == null); // duplicate id
    expect_true(tree.addLeaf(2, good_v[0..], 10) != null);
    expect_true(tree.addLeaf(3, good_v[0..], 10) == null); // full capacity

    // Query boundary guards
    let hits: [4]RaptorSearchHit = [RaptorSearchHit{}; 4];
    let hits_s: []RaptorSearchHit = hits[0..];
    let empty_hits: [0]RaptorSearchHit = [];
    let empty_hits_s: []RaptorSearchHit = empty_hits[0..];
    const empty_q: [0]f32 = [];

    expect_eq(tree.searchCollapsed(empty_q[0..], 4, RaptorScoreFusion.linear_weighted, 0, 0, *hits_s), 0 as usize);
    expect_eq(tree.searchCollapsed(zero_v[0..], 4, RaptorScoreFusion.linear_weighted, 0, 0, *hits_s), 0 as usize);
    expect_eq(tree.searchCollapsed(nan_v[0..], 4, RaptorScoreFusion.linear_weighted, 0, 0, *hits_s), 0 as usize);
    expect_eq(tree.searchCollapsed(good_v[0..], 0, RaptorScoreFusion.linear_weighted, 0, 0, *hits_s), 0 as usize);
    expect_eq(tree.searchCollapsed(good_v[0..], 4, RaptorScoreFusion.linear_weighted, 2, 1, *hits_s), 0 as usize);
    expect_eq(tree.searchCollapsed(good_v[0..], 4, RaptorScoreFusion.linear_weighted, 0, 0, *empty_hits_s), 0 as usize);
    expect_eq(tree.searchTreeTraversal(good_v[0..], 0, 4, RaptorScoreFusion.linear_weighted, *hits_s), 0 as usize);
});
