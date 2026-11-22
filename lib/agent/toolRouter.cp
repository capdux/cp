// Origin: Pure-CP Stdlib (Zero-Alloc SIMD Semantic + Tag Hybrid Tool Router for AI Agents) | Created: 2026-11-22
// Dynamically selects the most relevant Top-K MCP/function tools before sending tool schemas to an LLM,
// using CP native 8-lane SIMD `@dot` cosine similarity, 64-bit tag bitmask filtering, pinned tool
// guarantees, and dual (max_tools + max_schema_tokens) prompt token budget constraints.

fn isFiniteF32(x: f32): bool {
    return (x == x) && (x - x == 0.0);
}

fn clampCosine(v: f32): f32 {
    if (v > 1.0) return 1.0;
    if (v < -1.0) return -1.0;
    return v;
}

fn satAddU32(a: u32, b: u32): u32 {
    if (a > 0xFFFFFFFF - b) return 0xFFFFFFFF;
    return a + b;
}

/// Fixed-layout descriptor for a registered routable AI Agent tool.
pub struct RoutableToolEntry {
    tool_id: u32 = 0;
    name: string = "";
    tag_mask: u64 = 0;
    schema_tokens: u32 = 0;
    pinned: bool = false;
    enabled: bool = true;
}

/// Single routed tool hit emitted by `SemanticToolRouter.routeTools`.
pub struct ToolRouteHit {
    tool_id: u32 = 0;
    slot_index: usize = 0;
    name: string = "";
    score: f32 = 0.0;
    schema_tokens: u32 = 0;
    pinned: bool = false;
}

/// Aggregate routing report returned by `SemanticToolRouter.routeTools`.
pub struct ToolRouteReport {
    selected_count: usize = 0;
    pinned_count: usize = 0;
    used_schema_tokens: u32 = 0;
    filtered_out_count: usize = 0;
    truncated_by_budget: bool = false;
}

/// Zero-heap-allocation SIMD semantic + tag hybrid tool router.
/// Backed by caller-provided `entries` and flat L2-normalized `vecs_flat` (`entries.len * dim`).
pub struct SemanticToolRouter {
    entries: []RoutableToolEntry;
    vecs_flat: []f32;
    dim: usize = 0;
    count: usize = 0;

    /// Initializes a `SemanticToolRouter` over caller-provided buffers.
    /// Returns `null` if `dim == 0`, `entries_buf.len == 0`, or `vecs_flat_buf.len < entries_buf.len * dim`.
    static fn init(
        entries_buf: []RoutableToolEntry,
        vecs_flat_buf: []f32,
        dim: usize,
    ): SemanticToolRouter? {
        if (dim == 0 || entries_buf.len == 0) {
            return null;
        }
        if (entries_buf.len > 0xFFFFFFFFFFFFFFFF / dim) {
            return null;
        }
        const needed = entries_buf.len * dim;
        if (vecs_flat_buf.len < needed) {
            return null;
        }
        return SemanticToolRouter{
            entries: entries_buf,
            vecs_flat: vecs_flat_buf,
            dim: dim,
            count: 0,
        };
    }

    /// Registers a tool with its semantic embedding (automatically L2-normalized using SIMD `@dot`).
    /// Rejects duplicate `tool_id`, empty `name`, dimension mismatch, zero/non-finite vectors, or full capacity.
    fn registerTool(
        *_,
        tool_id: u32,
        name: string,
        tag_mask: u64,
        schema_tokens: u32,
        pinned: bool,
        embedding: []const f32,
    ): bool {
        if (_.count >= _.entries.len) return false;
        if (name.len == 0 || embedding.len != _.dim) return false;

        // Deduplicate by tool_id
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].tool_id == tool_id) {
                return false;
            }
        }

        // Validate finite elements and compute L2 norm squared via SIMD @dot
        let k: usize = 0;
        while (k < _.dim) : (k += 1) {
            if (!isFiniteF32(embedding[k])) {
                return false;
            }
        }
        const norm_sq: f32 = @dot(embedding, embedding);
        if (!isFiniteF32(norm_sq) || norm_sq <= 0.0) {
            return false;
        }

        const inv_norm: f32 = 1.0 / @sqrt(norm_sq);
        const base = _.count * _.dim;
        let j: usize = 0;
        while (j < _.dim) : (j += 1) {
            _.vecs_flat[base + j] = embedding[j] * inv_norm;
        }

        _.entries[_.count] = RoutableToolEntry{
            tool_id: tool_id,
            name: name,
            tag_mask: tag_mask,
            schema_tokens: schema_tokens,
            pinned: pinned,
            enabled: true,
        };
        _.count += 1;
        return true;
    }

    /// Dynamically enables or disables a registered tool by `tool_id`.
    /// Returns `false` if `tool_id` is not registered.
    fn setEnabled(*_, tool_id: u32, enabled: bool): bool {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].tool_id == tool_id) {
                _.entries[i].enabled = enabled;
                return true;
            }
        }
        return false;
    }

    /// Returns the number of currently registered tools.
    fn toolCount(): usize {
        return _.count;
    }

    /// Returns the maximum number of tools the router can hold.
    fn capacity(): usize {
        return _.entries.len;
    }

    /// Returns the configured embedding vector dimension.
    fn dimension(): usize {
        return _.dim;
    }

    /// Returns the entry at `idx`, or `null` if out of bounds.
    fn entryAt(idx: usize): RoutableToolEntry? {
        if (idx >= _.count) return null;
        return _.entries[idx];
    }

    /// Clears all registered tools for buffer reuse.
    fn clear(*_) {
        _.count = 0;
    }

    /// Routes the most relevant tools for `query_vec` under tag, score, count, and schema-token budgets:
    /// 1. Enabled `pinned` tools matching `required_tag_mask` are selected first (guaranteed inclusion
    ///    regardless of `min_score`, ordered by descending SIMD cosine score, bounded by budget).
    /// 2. Remaining enabled tools matching `required_tag_mask` and `score >= min_score` are greedily
    ///    selected in descending `@dot` cosine similarity order within `max_tools` and `max_schema_tokens`
    ///    (`max_schema_tokens == 0` disables the token budget cap).
    /// Returns `null` if `query_vec.len != dim`, `query_vec` is zero/non-finite, or `min_score` is NaN.
    fn routeTools(
        query_vec: []const f32,
        required_tag_mask: u64,
        max_tools: usize,
        max_schema_tokens: u32,
        min_score: f32,
        out_hits: []ToolRouteHit,
    ): ToolRouteReport? {
        if (query_vec.len != _.dim || !isFiniteF32(min_score)) {
            return null;
        }
        let d: usize = 0;
        while (d < _.dim) : (d += 1) {
            if (!isFiniteF32(query_vec[d])) {
                return null;
            }
        }
        const q_norm_sq: f32 = @dot(query_vec, query_vec);
        if (!isFiniteF32(q_norm_sq) || q_norm_sq <= 0.0) {
            return null;
        }
        const q_inv_norm: f32 = 1.0 / @sqrt(q_norm_sq);

        const max_cap = if (max_tools < out_hits.len) max_tools else out_hits.len;
        let report = ToolRouteReport{};

        // Count filtered-out tools (disabled, tag mismatch, or non-pinned below min_score)
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            const e = _.entries[i];
            if (!e.enabled || (e.tag_mask & required_tag_mask) != required_tag_mask) {
                report.filtered_out_count += 1;
                continue;
            }
            if (!e.pinned) {
                const base = i * _.dim;
                const unit_vec: []const f32 = _.vecs_flat[base .. base + _.dim];
                const score = clampCosine(@dot(query_vec, unit_vec) * q_inv_norm);
                if (score < min_score) {
                    report.filtered_out_count += 1;
                }
            }
        }

        // Phase 1: Select eligible pinned tools in descending score order (tie-break: lowest tool_id)
        while (true) {
            let best_found = false;
            let best_idx: usize = 0;
            let best_score: f32 = -2.0;
            let best_id: u32 = 0xFFFFFFFF;
            let saw_budget_skip = false;

            let idx: usize = 0;
            while (idx < _.count) : (idx += 1) {
                const e = _.entries[idx];
                if (!e.enabled || !e.pinned) continue;
                if ((e.tag_mask & required_tag_mask) != required_tag_mask) continue;

                // Skip if already selected
                let already = false;
                let s: usize = 0;
                while (s < report.selected_count) : (s += 1) {
                    if (out_hits[s].slot_index == idx) {
                        already = true;
                        break;
                    }
                }
                if (already) continue;

                if (report.selected_count >= max_cap) {
                    saw_budget_skip = true;
                    break;
                }
                if (max_schema_tokens > 0 && satAddU32(report.used_schema_tokens, e.schema_tokens) > max_schema_tokens) {
                    saw_budget_skip = true;
                    continue;
                }

                const base = idx * _.dim;
                const unit_vec: []const f32 = _.vecs_flat[base .. base + _.dim];
                const score = clampCosine(@dot(query_vec, unit_vec) * q_inv_norm);
                if (!best_found || score > best_score || (score == best_score && e.tool_id < best_id)) {
                    best_found = true;
                    best_idx = idx;
                    best_score = score;
                    best_id = e.tool_id;
                }
            }

            if (saw_budget_skip) {
                report.truncated_by_budget = true;
            }
            if (!best_found) break;

            const chosen = _.entries[best_idx];
            out_hits[report.selected_count] = ToolRouteHit{
                tool_id: chosen.tool_id,
                slot_index: best_idx,
                name: chosen.name,
                score: best_score,
                schema_tokens: chosen.schema_tokens,
                pinned: true,
            };
            report.selected_count += 1;
            report.pinned_count += 1;
            report.used_schema_tokens = satAddU32(report.used_schema_tokens, chosen.schema_tokens);
        }

        // Phase 2: Greedily select eligible non-pinned tools in descending SIMD cosine score order
        while (true) {
            let best_found = false;
            let best_idx: usize = 0;
            let best_score: f32 = -2.0;
            let best_id: u32 = 0xFFFFFFFF;
            let saw_budget_skip = false;

            let idx: usize = 0;
            while (idx < _.count) : (idx += 1) {
                const e = _.entries[idx];
                if (!e.enabled || e.pinned) continue;
                if ((e.tag_mask & required_tag_mask) != required_tag_mask) continue;

                let already = false;
                let s: usize = report.pinned_count;
                while (s < report.selected_count) : (s += 1) {
                    if (out_hits[s].slot_index == idx) {
                        already = true;
                        break;
                    }
                }
                if (already) continue;

                const base = idx * _.dim;
                const unit_vec: []const f32 = _.vecs_flat[base .. base + _.dim];
                const score = clampCosine(@dot(query_vec, unit_vec) * q_inv_norm);
                if (score < min_score) continue;

                if (report.selected_count >= max_cap) {
                    saw_budget_skip = true;
                    break;
                }
                if (max_schema_tokens > 0 && satAddU32(report.used_schema_tokens, e.schema_tokens) > max_schema_tokens) {
                    saw_budget_skip = true;
                    continue;
                }

                if (!best_found || score > best_score || (score == best_score && e.tool_id < best_id)) {
                    best_found = true;
                    best_idx = idx;
                    best_score = score;
                    best_id = e.tool_id;
                }
            }

            if (saw_budget_skip) {
                report.truncated_by_budget = true;
            }
            if (!best_found) break;

            const chosen = _.entries[best_idx];
            out_hits[report.selected_count] = ToolRouteHit{
                tool_id: chosen.tool_id,
                slot_index: best_idx,
                name: chosen.name,
                score: best_score,
                schema_tokens: chosen.schema_tokens,
                pinned: false,
            };
            report.selected_count += 1;
            report.used_schema_tokens = satAddU32(report.used_schema_tokens, chosen.schema_tokens);
        }

        return report;
    }
}
