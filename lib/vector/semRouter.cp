// Origin: Pure-CP Stdlib (Zero-Alloc SIMD Semantic Intent Router for Fast AI Agent Pre-Routing) | Created: 2026-11-22
// Pure CP zero-heap-allocation SIMD Semantic Intent Router:
//   - Microsecond pre-LLM intent classification using CP native 8-lane SIMD `@dot` + `@sqrt`
//   - Fixed inline route table (`SEM_ROUTER_MAX_ROUTES = 32`) & exemplar map (`SEM_ROUTER_MAX_EXEMPLARS = 128`)
//   - Caller-owned flat `f32` exemplar pool with automatic L2 normalization on ingest & scale-invariant queries
//   - Supports both `max_exemplar` (max cosine across route exemplars) and `centroid_mean` (mean cosine) modes
//   - Per-route independent confidence threshold (`min_threshold_f32`) + Top-1/Top-2 `min_margin_f32` ambiguity guard

const std = @import("std");

/// Maximum registered semantic intent routes (zero dynamic heap).
pub const SEM_ROUTER_MAX_ROUTES: usize = 32;

/// Maximum exemplar embeddings stored across all routes (zero dynamic heap).
pub const SEM_ROUTER_MAX_EXEMPLARS: usize = 128;

/// Maximum byte length of a route name identifier.
pub const SEM_ROUTER_NAME_MAX_LEN: usize = 48;

const NORM_SQ_EPS: f32 = 1.0e-12;
const SCORE_TIE_EPS: f32 = 1.0e-7;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn clampCosineF32(x: f32): f32 {
    if (x > 1.0) return 1.0;
    if (x < -1.0) return -1.0;
    return x;
}

fn allFiniteF32(xs: []const f32): bool {
    let i: usize = 0;
    while (i < xs.len) : (i += 1) {
        if (!isFiniteF32(xs[i])) return false;
    }
    return true;
}

// ============================================================================
// 1. Public Enums, Configuration & Decision Models
// ============================================================================

/// Aggregation mode across multiple exemplar embeddings belonging to the same route.
pub enum SemanticAggMode {
    max_exemplar,
    centroid_mean,
}

/// Deterministic routing outcome returned by `SemanticIntentRouter.route_query`.
pub enum SemanticRouteStatus {
    matched,
    below_threshold,
    ambiguous_margin,
    no_routes,
    invalid_query,
}

/// Configuration for `SemanticIntentRouter`.
pub struct SemanticRouterConfig {
    dim: usize = 0;
    agg_mode: SemanticAggMode = SemanticAggMode.max_exemplar;
    /// Minimum required score gap `(best_score - second_score)` when >= 2 routes are scored.
    min_margin_f32: f32 = 0.05;

    /// Validates `dim > 0` and finite `min_margin_f32 in [0.0, 2.0]`.
    fn isValid(): bool {
        if (_.dim == 0) return false;
        if (!isFiniteF32(_.min_margin_f32) || _.min_margin_f32 < 0.0 || _.min_margin_f32 > 2.0) return false;
        return true;
    }
}

/// Ranked route candidate written into `out_top_hits` by `route_query`.
pub struct SemanticRouteHit {
    route_id: u32 = 0;
    score_f32: f32 = 0.0;
    passed_threshold: bool = false;
}

/// Final routing decision returned by `SemanticIntentRouter.route_query`.
pub struct SemanticRouteDecision {
    status: SemanticRouteStatus = SemanticRouteStatus.no_routes;
    route_id: u32 = 0;
    best_score_f32: f32 = 0.0;
    second_score_f32: f32 = 0.0;
    margin_f32: f32 = 0.0;
}

/// Operational telemetry snapshot for `SemanticIntentRouter`.
pub struct SemanticRouterStats {
    route_count: usize = 0;
    exemplar_count: usize = 0;
    dim: usize = 0;
    total_queries: u64 = 0;
    matched_count: u64 = 0;
    fallback_count: u64 = 0;
}

struct SemanticRouteSlot {
    route_id: u32 = 0;
    name_buf: [SEM_ROUTER_NAME_MAX_LEN]u8 = [0; SEM_ROUTER_NAME_MAX_LEN];
    name_len: usize = 0;
    min_threshold_f32: f32 = 0.70;
    exemplar_count: usize = 0;
    occupied: bool = false;
}

// ============================================================================
// 2. Zero-Alloc SIMD Semantic Intent Router
// ============================================================================

/// Zero-heap-allocation SIMD semantic intent router backed by a caller-owned `vec_pool`.
pub struct SemanticIntentRouter {
    cfg: SemanticRouterConfig = SemanticRouterConfig{},
    vec_pool: []f32;
    pool_exemplar_cap: usize = 0;
    routes: [SEM_ROUTER_MAX_ROUTES]SemanticRouteSlot = [SemanticRouteSlot{}; SEM_ROUTER_MAX_ROUTES],
    route_count: usize = 0;
    exemplar_route_idx: [SEM_ROUTER_MAX_EXEMPLARS]usize = [0; SEM_ROUTER_MAX_EXEMPLARS],
    exemplar_count: usize = 0;
    total_queries: u64 = 0;
    matched_count: u64 = 0;
    fallback_count: u64 = 0;

    /// Initializes a `SemanticIntentRouter` over caller-owned flat `vec_pool`.
    /// Requires `cfg.isValid()` and `vec_pool.len >= cfg.dim`.
    static fn init(cfg: SemanticRouterConfig, vec_pool: *[]f32): SemanticIntentRouter? {
        if (!cfg.isValid()) return null;
        const raw_cap: usize = vec_pool.len / cfg.dim;
        if (raw_cap == 0) return null;
        const cap_ex: usize = if (raw_cap > SEM_ROUTER_MAX_EXEMPLARS) SEM_ROUTER_MAX_EXEMPLARS else raw_cap;

        return SemanticIntentRouter{
            cfg: cfg,
            vec_pool: vec_pool,
            pool_exemplar_cap: cap_ex,
        };
    }

    /// Registers a new intent route with a unique `route_id`, non-empty `name` (<= 48B),
    /// and per-route cosine threshold `min_threshold_f32 in [-1.0, 1.0]`.
    fn register_route(
        *_,
        route_id: u32,
        name: []const u8,
        min_threshold_f32: f32,
    ): bool {
        if (name.len == 0 || name.len > SEM_ROUTER_NAME_MAX_LEN) return false;
        if (!isFiniteF32(min_threshold_f32) || min_threshold_f32 < -1.0 || min_threshold_f32 > 1.0) return false;
        if (_.route_count >= SEM_ROUTER_MAX_ROUTES) return false;

        let i: usize = 0;
        while (i < _.route_count) : (i += 1) {
            if (_.routes[i].occupied && _.routes[i].route_id == route_id) {
                return false;
            }
        }

        let slot = SemanticRouteSlot{
            route_id: route_id,
            name_buf: [0; SEM_ROUTER_NAME_MAX_LEN],
            name_len: name.len,
            min_threshold_f32: min_threshold_f32,
            exemplar_count: 0,
            occupied: true,
        };
        let k: usize = 0;
        while (k < name.len) : (k += 1) {
            slot.name_buf[k] = name[k];
        }
        _.routes[_.route_count] = slot;
        _.route_count += 1;
        return true;
    }

    /// L2-normalizes `embedding` via SIMD `@dot` + `@sqrt` and appends it as an exemplar for `route_id`.
    /// Returns `false` on unknown route, dim mismatch, full exemplar pool, zero vector, or non-finite values.
    fn add_exemplar(*_, route_id: u32, embedding: []const f32): bool {
        const dim: usize = _.cfg.dim;
        if (embedding.len != dim) return false;
        if (_.exemplar_count >= _.pool_exemplar_cap || _.exemplar_count >= SEM_ROUTER_MAX_EXEMPLARS) return false;
        if (!allFiniteF32(embedding)) return false;

        let route_idx: usize = SEM_ROUTER_MAX_ROUTES;
        let r: usize = 0;
        while (r < _.route_count) : (r += 1) {
            if (_.routes[r].occupied && _.routes[r].route_id == route_id) {
                route_idx = r;
                break;
            }
        }
        if (route_idx >= _.route_count) return false;

        const norm_sq: f32 = @dot(embedding, embedding);
        if (!isFiniteF32(norm_sq) || norm_sq <= NORM_SQ_EPS) return false;
        const inv_norm: f32 = 1.0 / @sqrt(norm_sq);

        const base: usize = _.exemplar_count * dim;
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            _.vec_pool[base + d] = embedding[d] * inv_norm;
        }

        _.exemplar_route_idx[_.exemplar_count] = route_idx;
        _.routes[route_idx].exemplar_count += 1;
        _.exemplar_count += 1;
        return true;
    }

    /// Routes `query_embedding` across all registered routes with exemplars using 8-lane SIMD `@dot`,
    /// populates `out_top_hits` in descending `(score_f32 desc, route_id asc)` order, and enforces
    /// per-route `min_threshold_f32` and Top-1/Top-2 `min_margin_f32` guardrails.
    fn route_query(
        *_,
        query_embedding: []const f32,
        out_top_hits: *[]SemanticRouteHit,
    ): SemanticRouteDecision {
        const dim: usize = _.cfg.dim;
        if (query_embedding.len != dim || !allFiniteF32(query_embedding)) {
            _.total_queries += 1;
            _.fallback_count += 1;
            return SemanticRouteDecision{ status: SemanticRouteStatus.invalid_query };
        }

        const q_norm_sq: f32 = @dot(query_embedding, query_embedding);
        if (!isFiniteF32(q_norm_sq) || q_norm_sq <= NORM_SQ_EPS) {
            _.total_queries += 1;
            _.fallback_count += 1;
            return SemanticRouteDecision{ status: SemanticRouteStatus.invalid_query };
        }

        if (_.route_count == 0 || _.exemplar_count == 0) {
            _.total_queries += 1;
            _.fallback_count += 1;
            return SemanticRouteDecision{ status: SemanticRouteStatus.no_routes };
        }

        const inv_q_norm: f32 = 1.0 / @sqrt(q_norm_sq);

        // Accumulate per-route scores in a single pass over all stored exemplars
        let route_accum: [SEM_ROUTER_MAX_ROUTES]f32 = [0.0; SEM_ROUTER_MAX_ROUTES];
        let route_seen: [SEM_ROUTER_MAX_ROUTES]bool = [false; SEM_ROUTER_MAX_ROUTES];

        let e: usize = 0;
        while (e < _.exemplar_count) : (e += 1) {
            const r_idx: usize = _.exemplar_route_idx[e];
            const base: usize = e * dim;
            const ex_slice: []const f32 = _.vec_pool[base..(base + dim)];
            const sim: f32 = clampCosineF32(@dot(query_embedding, ex_slice) * inv_q_norm);

            if (_.cfg.agg_mode == SemanticAggMode.max_exemplar) {
                if (!route_seen[r_idx] || sim > route_accum[r_idx]) {
                    route_accum[r_idx] = sim;
                    route_seen[r_idx] = true;
                }
            } else {
                route_accum[r_idx] += sim;
                route_seen[r_idx] = true;
            }
        }

        // Collect all scored routes into stack buffer
        let ranked: [SEM_ROUTER_MAX_ROUTES]SemanticRouteHit = [SemanticRouteHit{}; SEM_ROUTER_MAX_ROUTES];
        let scored_count: usize = 0;
        let r: usize = 0;
        while (r < _.route_count) : (r += 1) {
            if (route_seen[r] && _.routes[r].exemplar_count > 0) {
                let final_score: f32 = route_accum[r];
                if (_.cfg.agg_mode == SemanticAggMode.centroid_mean) {
                    final_score = clampCosineF32(final_score / (_.routes[r].exemplar_count as f32));
                }
                ranked[scored_count] = SemanticRouteHit{
                    route_id: _.routes[r].route_id,
                    score_f32: final_score,
                    passed_threshold: final_score >= _.routes[r].min_threshold_f32,
                };
                scored_count += 1;
            }
        }

        if (scored_count == 0) {
            _.total_queries += 1;
            _.fallback_count += 1;
            return SemanticRouteDecision{ status: SemanticRouteStatus.no_routes };
        }

        // Deterministic insertion sort: score_f32 desc, tie-break by route_id asc
        let s: usize = 1;
        while (s < scored_count) : (s += 1) {
            const key_hit = ranked[s];
            let j: usize = s;
            while (j > 0) {
                const prev = ranked[j - 1];
                const diff: f32 = key_hit.score_f32 - prev.score_f32;
                const ahead: bool = if (diff > SCORE_TIE_EPS) true else if (diff < -SCORE_TIE_EPS) false else (key_hit.route_id < prev.route_id);
                if (!ahead) break;
                ranked[j] = prev;
                j -= 1;
            }
            ranked[j] = key_hit;
        }

        // Populate caller's out_top_hits slice
        const to_write: usize = if (scored_count < out_top_hits.len) scored_count else out_top_hits.len;
        let w: usize = 0;
        while (w < to_write) : (w += 1) {
            out_top_hits[w] = ranked[w];
        }

        const best = ranked[0];
        const has_second: bool = scored_count >= 2;
        const second_score: f32 = if (has_second) ranked[1].score_f32 else 0.0;
        const margin: f32 = if (has_second) (best.score_f32 - second_score) else 0.0;

        _.total_queries += 1;
        if (!best.passed_threshold) {
            _.fallback_count += 1;
            return SemanticRouteDecision{
                status: SemanticRouteStatus.below_threshold,
                route_id: best.route_id,
                best_score_f32: best.score_f32,
                second_score_f32: second_score,
                margin_f32: margin,
            };
        }
        if (has_second && margin < _.cfg.min_margin_f32) {
            _.fallback_count += 1;
            return SemanticRouteDecision{
                status: SemanticRouteStatus.ambiguous_margin,
                route_id: best.route_id,
                best_score_f32: best.score_f32,
                second_score_f32: second_score,
                margin_f32: margin,
            };
        }

        _.matched_count += 1;
        return SemanticRouteDecision{
            status: SemanticRouteStatus.matched,
            route_id: best.route_id,
            best_score_f32: best.score_f32,
            second_score_f32: second_score,
            margin_f32: margin,
        };
    }

    /// Returns live route/exemplar occupancy and cumulative routing telemetry.
    fn stats(): SemanticRouterStats {
        return SemanticRouterStats{
            route_count: _.route_count,
            exemplar_count: _.exemplar_count,
            dim: _.cfg.dim,
            total_queries: _.total_queries,
            matched_count: _.matched_count,
            fallback_count: _.fallback_count,
        };
    }
}
