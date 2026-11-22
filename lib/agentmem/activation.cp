// Origin: Pure-CP Stdlib (AI GraphRAG Spreading Activation & Personalized PageRank Memory Recall) | Created: 2026-11-22
// Zero-heap-allocation multi-hop GraphRAG memory recall engine providing:
//   1. Decayed Spreading Activation with noise-floor thresholding and optional bidirectional propagation
//   2. Personalized PageRank (PPR) with weighted out-degrees, dangling-node mass recovery, and L1 early stopping

const std = @import("std");

/// Weighted directed edge in an agent memory graph.
pub struct MemGraphEdge {
    src: u32 = 0;
    dst: u32 = 0;
    weight: f32 = 1.0;
}

/// Ranked activated memory node produced by GraphRAG recall algorithms.
pub struct ActivatedNode {
    node_id: u32 = 0;
    score: f32 = 0.0;
}

/// Configuration parameters for multi-hop spreading activation.
pub struct SpreadingConfig {
    decay: f32 = 0.6;
    hops: u32 = 2;
    min_threshold: f32 = 0.01;
    bidirectional: bool = false;
}

fn isValidFiniteF32(v: f32): bool {
    return !std.math.isNan(v) && !std.math.isInf(v);
}

fn absF32(v: f32): f32 {
    return if (v < 0.0) -v else v;
}

fn isValidEdge(e: MemGraphEdge, num_nodes: usize): bool {
    if (usize(e.src) >= num_nodes || usize(e.dst) >= num_nodes) return false;
    if (!isValidFiniteF32(e.weight) || e.weight <= 0.0) return false;
    return true;
}

fn extractTopK(
    scores: []const f32,
    num_nodes: usize,
    min_score: f32,
    out_top_k: *[]ActivatedNode,
): usize {
    if (out_top_k.len == 0 || num_nodes == 0) return 0;

    let written: usize = 0;
    let i: usize = 0;
    while (i < num_nodes) : (i += 1) {
        const s = scores[i];
        if (!isValidFiniteF32(s) || s <= 0.0 || s < min_score) continue;

        const cand = ActivatedNode{
            node_id: u32(i),
            score: s,
        };

        // Determine insertion position (descending score; tie-break by ascending node_id).
        let pos: usize = written;
        while (pos > 0) {
            const prev = out_top_k[pos - 1];
            const better = (cand.score > prev.score) ||
                (cand.score == prev.score && cand.node_id < prev.node_id);
            if (!better) break;
            if (pos < out_top_k.len) {
                out_top_k[pos] = prev;
            }
            pos -= 1;
        }

        if (pos < out_top_k.len) {
            out_top_k[pos] = cand;
            if (written < out_top_k.len) {
                written += 1;
            }
        }
    }

    return written;
}

/// Multi-hop spreading activation over a weighted memory graph using caller-owned double buffers.
/// Propagates activation energy from `seed_nodes` along weighted edges for `cfg.hops` iterations,
/// attenuating each hop by `cfg.decay`, pruning values below `cfg.min_threshold`, and writing
/// descending Top-K activated nodes into `out_top_k`.
/// Returns `null` on invalid configuration or insufficient scratch buffer capacity.
pub fn spreadActivation(
    num_nodes: usize,
    edges: []const MemGraphEdge,
    seed_nodes: []const u32,
    seed_weights: []const f32,
    cfg: SpreadingConfig,
    scores_a: *[]f32,
    scores_b: *[]f32,
    out_top_k: *[]ActivatedNode,
): usize? {
    if (seed_nodes.len != seed_weights.len) return null;
    if (!isValidFiniteF32(cfg.decay) || cfg.decay <= 0.0 || cfg.decay > 1.0) return null;
    if (!isValidFiniteF32(cfg.min_threshold) || cfg.min_threshold < 0.0) return null;
    if (num_nodes == 0) return 0;
    if (scores_a.len < num_nodes || scores_b.len < num_nodes) return null;

    let i: usize = 0;
    while (i < num_nodes) : (i += 1) {
        scores_a[i] = 0.0;
        scores_b[i] = 0.0;
    }

    // Seed initial activation energies.
    let s_idx: usize = 0;
    let has_valid_seed = false;
    while (s_idx < seed_nodes.len) : (s_idx += 1) {
        const sid = usize(seed_nodes[s_idx]);
        const sw = seed_weights[s_idx];
        if (sid < num_nodes && isValidFiniteF32(sw) && sw > 0.0) {
            scores_a[sid] += sw;
            has_valid_seed = true;
        }
    }

    if (!has_valid_seed) return 0;

    let hop: u32 = 0;
    while (hop < cfg.hops) : (hop += 1) {
        let z: usize = 0;
        while (z < num_nodes) : (z += 1) {
            scores_b[z] = 0.0;
        }

        // Propagate decayed energy from active nodes along valid edges.
        let e_idx: usize = 0;
        while (e_idx < edges.len) : (e_idx += 1) {
            const e = edges[e_idx];
            if (!isValidEdge(e, num_nodes)) continue;

            const u = usize(e.src);
            const v = usize(e.dst);
            const au = scores_a[u];
            if (au > 0.0 && au >= cfg.min_threshold) {
                scores_b[v] += au * e.weight * cfg.decay;
            }

            if (cfg.bidirectional && u != v) {
                const av = scores_a[v];
                if (av > 0.0 && av >= cfg.min_threshold) {
                    scores_b[u] += av * e.weight * cfg.decay;
                }
            }
        }

        // Re-anchor seed nodes so hop-k retains truncated Neumann series sum_{j=0..k} (d*W)^j * s.
        let si: usize = 0;
        while (si < seed_nodes.len) : (si += 1) {
            const sid = usize(seed_nodes[si]);
            const sw = seed_weights[si];
            if (sid < num_nodes && isValidFiniteF32(sw) && sw > 0.0) {
                scores_b[sid] += sw;
            }
        }

        // Prune noise below min_threshold and copy back to scores_a.
        let k: usize = 0;
        while (k < num_nodes) : (k += 1) {
            const val = scores_b[k];
            scores_a[k] = if (isValidFiniteF32(val) && val >= cfg.min_threshold) val else 0.0;
        }
    }

    return extractTopK(scores_a[0..num_nodes], num_nodes, cfg.min_threshold, out_top_k);
}

/// Zero-heap-allocation Personalized PageRank (PPR) for GraphRAG entity relevance re-ranking.
/// Supports weighted directed edges, dangling-node probability mass recovery back to the
/// personalized seed distribution (preserving sum(rank) == 1.0), and L1 convergence early stopping.
/// Returns `null` on invalid parameters or insufficient scratch buffer capacity.
pub fn personalizedPageRank(
    num_nodes: usize,
    edges: []const MemGraphEdge,
    seed_nodes: []const u32,
    seed_weights: []const f32,
    damping: f32,
    max_iters: u32,
    tol: f32,
    rank_cur: *[]f32,
    rank_next: *[]f32,
    out_deg: *[]f32,
    out_top_k: *[]ActivatedNode,
): usize? {
    if (seed_nodes.len != seed_weights.len) return null;
    if (!isValidFiniteF32(damping) || damping <= 0.0 || damping >= 1.0) return null;
    if (!isValidFiniteF32(tol) || tol < 0.0) return null;
    if (num_nodes == 0) return 0;
    if (rank_cur.len < num_nodes || rank_next.len < num_nodes || out_deg.len < num_nodes) {
        return null;
    }

    let i: usize = 0;
    while (i < num_nodes) : (i += 1) {
        rank_cur[i] = 0.0;
        rank_next[i] = 0.0;
        out_deg[i] = 0.0;
    }

    // Compute normalized seed distribution mass.
    let seed_sum: f32 = 0.0;
    let s_idx: usize = 0;
    while (s_idx < seed_nodes.len) : (s_idx += 1) {
        const sid = usize(seed_nodes[s_idx]);
        const sw = seed_weights[s_idx];
        if (sid < num_nodes && isValidFiniteF32(sw) && sw > 0.0) {
            seed_sum += sw;
        }
    }
    if (seed_sum <= 0.0) return 0;

    // Precompute weighted out-degrees, ignoring out-of-bounds or non-positive edges.
    let e_idx: usize = 0;
    while (e_idx < edges.len) : (e_idx += 1) {
        const e = edges[e_idx];
        if (!isValidEdge(e, num_nodes)) continue;
        out_deg[usize(e.src)] += e.weight;
    }

    // Initialize rank_cur with the normalized personalized seed distribution.
    let init_s: usize = 0;
    while (init_s < seed_nodes.len) : (init_s += 1) {
        const sid = usize(seed_nodes[init_s]);
        const sw = seed_weights[init_s];
        if (sid < num_nodes && isValidFiniteF32(sw) && sw > 0.0) {
            rank_cur[sid] += sw / seed_sum;
        }
    }

    let iter: u32 = 0;
    while (iter < max_iters) : (iter += 1) {
        let dangling_mass: f32 = 0.0;
        let u: usize = 0;
        while (u < num_nodes) : (u += 1) {
            rank_next[u] = 0.0;
            if (out_deg[u] <= 0.0) {
                dangling_mass += rank_cur[u];
            }
        }

        // Distribute rank along weighted outgoing edges.
        let ei: usize = 0;
        while (ei < edges.len) : (ei += 1) {
            const e = edges[ei];
            if (!isValidEdge(e, num_nodes)) continue;
            const src_u = usize(e.src);
            const dst_v = usize(e.dst);
            const deg = out_deg[src_u];
            if (deg > 0.0) {
                rank_next[dst_v] += damping * rank_cur[src_u] * (e.weight / deg);
            }
        }

        // Teleportation + dangling-node mass recovery back to personalized seed distribution.
        const teleport_mass: f32 = (1.0 - damping) + damping * dangling_mass;
        let si: usize = 0;
        while (si < seed_nodes.len) : (si += 1) {
            const sid = usize(seed_nodes[si]);
            const sw = seed_weights[si];
            if (sid < num_nodes && isValidFiniteF32(sw) && sw > 0.0) {
                rank_next[sid] += teleport_mass * (sw / seed_sum);
            }
        }

        // Compute L1 delta and update rank_cur.
        let diff: f32 = 0.0;
        let k: usize = 0;
        while (k < num_nodes) : (k += 1) {
            diff += absF32(rank_next[k] - rank_cur[k]);
            rank_cur[k] = rank_next[k];
        }

        if (diff <= tol) {
            break;
        }
    }

    return extractTopK(rank_cur[0..num_nodes], num_nodes, 0.0, out_top_k);
}
