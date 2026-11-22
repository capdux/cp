// Origin: Pure-CP Stdlib (AI Agent Zero-Alloc Dynamic Tool Router & Token Budget Trimmer) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation dynamic tool routing & schema token budget trimmer:
//   - Fixed-capacity tool catalog (`TOOL_ROUTER_MAX_TOOLS = 64`) storing tool_id, name,
//     64-bit capability `domain_mask`, `risk_tier`, `schema_tokens`, `priority`, `pinned`,
//     and up to 8 short trigger keywords per tool
//   - Two-phase deterministic routing (`routeTools`):
//       Phase 1: Locks risk-compliant `pinned` core tools first and deducts their token cost
//       Phase 2: Scores remaining risk-compliant tools using capability domain overlap (`@popCount`),
//                case-insensitive query keyword hit rate, direct tool-name match bonus, and priority,
//                then greedily packs in descending score order within `token_budget` & `max_tools`
//   - Produces complete `ToolRouteReport` (selected/pinned counts, consumed/saved tokens,
//     savings permille, and risk/budget/limit/threshold skip counters) + cumulative `ToolRouterStats`

/// Maximum number of tools stored in a single `ToolRouter` catalog (zero dynamic heap).
pub const TOOL_ROUTER_MAX_TOOLS: usize = 64;

/// Maximum byte length of a registered tool name.
pub const TOOL_ROUTER_NAME_MAX_LEN: usize = 64;

/// Maximum number of short trigger keywords stored per tool.
pub const TOOL_ROUTER_MAX_KEYWORDS: usize = 8;

/// Maximum byte length of a single trigger keyword.
pub const TOOL_ROUTER_KEYWORD_MAX_LEN: usize = 32;

const TOOL_ROUTER_KW_FLAT_LEN: usize = TOOL_ROUTER_MAX_KEYWORDS * TOOL_ROUTER_KEYWORD_MAX_LEN;

// ============================================================================
// 1. Internal Helpers (Zero-Alloc Pure CP)
// ============================================================================

fn satAddU32(a: u32, b: u32): u32 {
    if (a > 0xFFFFFFFF - b) {
        return 0xFFFFFFFF;
    }
    return a + b;
}

fn satMulU32(a: u32, b: u32): u32 {
    if (a == 0 || b == 0) {
        return 0;
    }
    if (a > 0xFFFFFFFF / b) {
        return 0xFFFFFFFF;
    }
    return a * b;
}

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn equalsIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (asciiLower(a[i]) != asciiLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn containsIgnoreCaseAscii(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0 || haystack.len < needle.len) {
        return false;
    }
    const max_start: usize = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= max_start) {
        let j: usize = 0;
        let matched: bool = true;
        while (j < needle.len) {
            if (asciiLower(haystack[i + j]) != asciiLower(needle[j])) {
                matched = false;
                break;
            }
            j += 1;
        }
        if (matched) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn copyBytes(dst: *[]u8, src: []const u8): usize {
    const n: usize = if (src.len <= dst.len) src.len else dst.len;
    let i: usize = 0;
    while (i < n) {
        dst[i] = src[i];
        i += 1;
    }
    return n;
}

// ============================================================================
// 2. Public Enums & Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// Risk classification tier for a registered AI Agent tool.
pub enum ToolRiskTier: u8 {
    safe = 0,
    low = 1,
    medium = 2,
    high = 3,
    critical = 4,
}

/// Converts `ToolRiskTier` to its monotonic numeric severity rank (`0..=4`).
pub fn toolRiskRank(tier: ToolRiskTier): u8 {
    if (tier == ToolRiskTier.safe) {
        return 0;
    }
    if (tier == ToolRiskTier.low) {
        return 1;
    }
    if (tier == ToolRiskTier.medium) {
        return 2;
    }
    if (tier == ToolRiskTier.high) {
        return 3;
    }
    return 4;
}

/// Multi-signal scoring weights used during tool routing.
pub struct ToolRouterWeights {
    /// Score awarded per overlapping capability bit (`@popCount(tool.domain_mask & required_domain_mask)`).
    domain_hit_weight: u32 = 400;
    /// Base score scaled by keyword hit ratio (`matched_keywords / total_keywords`).
    keyword_hit_weight: u32 = 300;
    /// Additional flat score per matched keyword to differentiate richer matches.
    keyword_per_hit_bonus: u32 = 60;
    /// Bonus awarded when the tool name appears directly (case-insensitive) in the query.
    name_match_bonus: u32 = 500;
    /// Multiplier applied to `entry.priority`.
    priority_weight: u32 = 10;
    /// Optional minimum relevance score required for non-pinned tools (`0` = no threshold).
    min_score_threshold: u32 = 0;

    /// Returns balanced production default scoring weights.
    static fn defaults(): ToolRouterWeights {
        return ToolRouterWeights{};
    }
}

/// Single fixed-capacity tool catalog entry stored inline inside `ToolRouter`.
pub struct ToolSpecEntry {
    active: bool = false;
    tool_id: u32 = 0;
    name_buf: [TOOL_ROUTER_NAME_MAX_LEN]u8 = [0; TOOL_ROUTER_NAME_MAX_LEN];
    name_len: usize = 0;
    domain_mask: u64 = 0;
    risk_tier: ToolRiskTier = ToolRiskTier.safe;
    schema_tokens: u32 = 0;
    priority: u16 = 0;
    pinned: bool = false;
    kw_buf: [TOOL_ROUTER_KW_FLAT_LEN]u8 = [0; TOOL_ROUTER_KW_FLAT_LEN];
    kw_lens: [TOOL_ROUTER_MAX_KEYWORDS]usize = [0; TOOL_ROUTER_MAX_KEYWORDS];
    kw_count: usize = 0;

    /// Returns the registered tool name as a zero-copy read-only byte slice.
    fn nameSlice(): []const u8 {
        const n: usize = if (_.name_len <= TOOL_ROUTER_NAME_MAX_LEN) _.name_len else TOOL_ROUTER_NAME_MAX_LEN;
        return _.name_buf[0..n];
    }

    /// Returns the number of trigger keywords registered on this tool (`0..=8`).
    fn keywordCount(): usize {
        return _.kw_count;
    }

    /// Returns the `idx`-th keyword as a zero-copy read-only byte slice, or `""` if out of range.
    fn keywordSlice(idx: usize): []const u8 {
        if (idx >= _.kw_count || idx >= TOOL_ROUTER_MAX_KEYWORDS) {
            return "";
        }
        const len: usize = if (_.kw_lens[idx] <= TOOL_ROUTER_KEYWORD_MAX_LEN) _.kw_lens[idx] else TOOL_ROUTER_KEYWORD_MAX_LEN;
        const base: usize = idx * TOOL_ROUTER_KEYWORD_MAX_LEN;
        return _.kw_buf[base..(base + len)];
    }
}

/// Single routed tool selection emitted by `ToolRouter.routeTools`.
pub struct RoutedToolPick {
    tool_id: u32 = 0;
    slot_index: usize = 0;
    score: u32 = 0;
    schema_tokens: u32 = 0;
    domain_overlap: u32 = 0;
    keyword_hits: u32 = 0;
    pinned: bool = false;
    risk_tier: ToolRiskTier = ToolRiskTier.safe;
}

/// Complete report returned by a single `ToolRouter.routeTools` invocation.
pub struct ToolRouteReport {
    catalog_total_tools: usize = 0;
    catalog_total_tokens: u64 = 0;
    selected_count: usize = 0;
    pinned_selected_count: usize = 0;
    consumed_tokens: u64 = 0;
    saved_tokens: u64 = 0;
    /// Token savings ratio in permille (`0..=1000`, e.g. `750` = 75.0% tokens saved).
    savings_permille: u32 = 0;
    skipped_by_risk: usize = 0;
    skipped_by_budget: usize = 0;
    skipped_by_limit: usize = 0;
    skipped_by_threshold: usize = 0;
    pinned_budget_overflow: bool = false;
}

/// Cumulative zero-heap telemetry snapshot across the lifetime of a `ToolRouter`.
pub struct ToolRouterStats {
    total_routes: u64 = 0;
    total_registered: u64 = 0;
    total_removed: u64 = 0;
    total_picks_emitted: u64 = 0;
    total_tokens_consumed: u64 = 0;
    total_tokens_saved: u64 = 0;
    total_risk_blocked: u64 = 0;
    total_budget_blocked: u64 = 0;
}

// Internal candidate descriptor for stack-local deterministic sorting.
struct ToolCandidate {
    slot_idx: usize = 0;
    tool_id: u32 = 0;
    score: u32 = 0;
    domain_overlap: u32 = 0;
    keyword_hits: u32 = 0;
    priority: u16 = 0;
    schema_tokens: u32 = 0;
}

fn pinnedCandidateBetter(a: ToolCandidate, b: ToolCandidate): bool {
    if (a.priority != b.priority) {
        return a.priority > b.priority;
    }
    if (a.schema_tokens != b.schema_tokens) {
        return a.schema_tokens < b.schema_tokens;
    }
    return a.tool_id < b.tool_id;
}

fn scoredCandidateBetter(a: ToolCandidate, b: ToolCandidate): bool {
    if (a.score != b.score) {
        return a.score > b.score;
    }
    if (a.domain_overlap != b.domain_overlap) {
        return a.domain_overlap > b.domain_overlap;
    }
    if (a.keyword_hits != b.keyword_hits) {
        return a.keyword_hits > b.keyword_hits;
    }
    if (a.priority != b.priority) {
        return a.priority > b.priority;
    }
    if (a.schema_tokens != b.schema_tokens) {
        return a.schema_tokens < b.schema_tokens;
    }
    return a.tool_id < b.tool_id;
}

fn scoreEntry(
    entry: ToolSpecEntry,
    query: []const u8,
    required_domain_mask: u64,
    weights: ToolRouterWeights,
): ToolCandidate {
    const overlap_bits: u64 = entry.domain_mask & required_domain_mask;
    const domain_overlap: u32 = @as(u32, @popCount(overlap_bits));
    const domain_score: u32 = satMulU32(domain_overlap, weights.domain_hit_weight);

    let kw_hits: u32 = 0;
    if (query.len > 0 && entry.kw_count > 0) {
        let k: usize = 0;
        while (k < entry.kw_count && k < TOOL_ROUTER_MAX_KEYWORDS) {
            const kw = entry.keywordSlice(k);
            if (kw.len > 0 && containsIgnoreCaseAscii(query, kw)) {
                kw_hits += 1;
            }
            k += 1;
        }
    }

    let kw_score: u32 = 0;
    if (kw_hits > 0 && entry.kw_count > 0) {
        const ratio_part: u32 = satMulU32(kw_hits, weights.keyword_hit_weight) / @as(u32, @intCast(entry.kw_count));
        const bonus_part: u32 = satMulU32(kw_hits, weights.keyword_per_hit_bonus);
        kw_score = satAddU32(ratio_part, bonus_part);
    }

    let name_score: u32 = 0;
    if (query.len > 0) {
        const name = entry.nameSlice();
        if (name.len > 0 && containsIgnoreCaseAscii(query, name)) {
            name_score = weights.name_match_bonus;
        }
    }

    const prio_score: u32 = satMulU32(@as(u32, entry.priority), weights.priority_weight);

    let total: u32 = satAddU32(domain_score, kw_score);
    total = satAddU32(total, name_score);
    total = satAddU32(total, prio_score);

    return ToolCandidate{
        slot_idx: 0,
        tool_id: entry.tool_id,
        score: total,
        domain_overlap: domain_overlap,
        keyword_hits: kw_hits,
        priority: entry.priority,
        schema_tokens: entry.schema_tokens,
    };
}

// ============================================================================
// 3. ToolRouter Core Engine
// ============================================================================

/// Zero-heap-allocation dynamic tool catalog, relevance ranker, and token budget trimmer.
pub struct ToolRouter {
    entries: [TOOL_ROUTER_MAX_TOOLS]ToolSpecEntry = [ToolSpecEntry{}; TOOL_ROUTER_MAX_TOOLS];
    active_count: usize = 0;
    weights: ToolRouterWeights = ToolRouterWeights.defaults();
    telemetry: ToolRouterStats = ToolRouterStats{};

    /// Creates an empty `ToolRouter` with default scoring weights.
    static fn init(): ToolRouter {
        return ToolRouter{};
    }

    /// Creates an empty `ToolRouter` with custom scoring weights.
    static fn initWithWeights(weights: ToolRouterWeights): ToolRouter {
        return ToolRouter{
            weights: weights,
        };
    }

    /// Updates scoring weights for subsequent `routeTools` calls.
    fn setWeights(*_, weights: ToolRouterWeights) {
        _.weights = weights;
    }

    /// Removes all tools from the catalog while preserving cumulative telemetry.
    fn clear(*_) {
        let i: usize = 0;
        while (i < TOOL_ROUTER_MAX_TOOLS) {
            _.entries[i] = ToolSpecEntry{};
            i += 1;
        }
        _.active_count = 0;
    }

    /// Removes all tools and resets cumulative telemetry counters to zero.
    fn resetAll(*_) {
        _.clear();
        _.telemetry = ToolRouterStats{};
    }

    /// Returns the number of currently active tools in the catalog.
    fn count(): usize {
        return _.active_count;
    }

    /// Returns the sum of `schema_tokens` across all active tools in the catalog.
    fn totalCatalogTokens(): u64 {
        let total: u64 = 0;
        let i: usize = 0;
        while (i < TOOL_ROUTER_MAX_TOOLS) {
            if (_.entries[i].active) {
                total += @as(u64, _.entries[i].schema_tokens);
            }
            i += 1;
        }
        return total;
    }

    /// Returns a copy of cumulative zero-heap telemetry statistics.
    fn stats(): ToolRouterStats {
        return _.telemetry;
    }

    /// Looks up the slot index of an active tool by `tool_id`, or `null` if not found.
    fn findById(tool_id: u32): usize? {
        let i: usize = 0;
        while (i < TOOL_ROUTER_MAX_TOOLS) {
            if (_.entries[i].active && _.entries[i].tool_id == tool_id) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// Returns a value copy of the active tool entry at `slot_idx`, or `null` if invalid/inactive.
    fn getEntry(slot_idx: usize): ToolSpecEntry? {
        if (slot_idx >= TOOL_ROUTER_MAX_TOOLS || !_.entries[slot_idx].active) {
            return null;
        }
        return _.entries[slot_idx];
    }

    /// Registers a new tool into the fixed catalog.
    /// Returns the assigned slot index, or `null` on invalid name, duplicate `tool_id`, or full catalog.
    fn registerTool(
        *_,
        tool_id: u32,
        name: []const u8,
        domain_mask: u64,
        risk_tier: ToolRiskTier,
        schema_tokens: u32,
        priority: u16,
        pinned: bool,
    ): usize? {
        if (name.len == 0 || name.len > TOOL_ROUTER_NAME_MAX_LEN) {
            return null;
        }
        if (_.active_count >= TOOL_ROUTER_MAX_TOOLS) {
            return null;
        }
        if (_ := _.findById(tool_id)) {
            return null;
        }

        let free_slot: usize = TOOL_ROUTER_MAX_TOOLS;
        let i: usize = 0;
        while (i < TOOL_ROUTER_MAX_TOOLS) {
            if (!_.entries[i].active) {
                free_slot = i;
                break;
            }
            i += 1;
        }
        if (free_slot >= TOOL_ROUTER_MAX_TOOLS) {
            return null;
        }

        let entry = ToolSpecEntry{
            active: true,
            tool_id: tool_id,
            domain_mask: domain_mask,
            risk_tier: risk_tier,
            schema_tokens: schema_tokens,
            priority: priority,
            pinned: pinned,
        };
        let dst: []u8 = entry.name_buf[0..];
        entry.name_len = copyBytes(*dst, name);

        _.entries[free_slot] = entry;
        _.active_count += 1;
        _.telemetry.total_registered += 1;
        return free_slot;
    }

    /// Appends a case-insensitive deduplicated trigger keyword to an existing tool (up to 8 keywords).
    /// Returns `true` if added, `false` if tool not found, invalid length, duplicate, or keyword slots full.
    fn addKeyword(*_, tool_id: u32, keyword: []const u8): bool {
        if (keyword.len == 0 || keyword.len > TOOL_ROUTER_KEYWORD_MAX_LEN) {
            return false;
        }
        const slot = _.findById(tool_id) ?? return false;
        if (_.entries[slot].kw_count >= TOOL_ROUTER_MAX_KEYWORDS) {
            return false;
        }

        // Deduplicate case-insensitively against existing keywords on this tool.
        let k: usize = 0;
        while (k < _.entries[slot].kw_count) {
            if (equalsIgnoreCaseAscii(_.entries[slot].keywordSlice(k), keyword)) {
                return false;
            }
            k += 1;
        }

        const idx: usize = _.entries[slot].kw_count;
        const base: usize = idx * TOOL_ROUTER_KEYWORD_MAX_LEN;
        let dst: []u8 = _.entries[slot].kw_buf[base..(base + TOOL_ROUTER_KEYWORD_MAX_LEN)];
        _.entries[slot].kw_lens[idx] = copyBytes(*dst, keyword);
        _.entries[slot].kw_count = idx + 1;
        return true;
    }

    /// Registers a tool and attaches up to `TOOL_ROUTER_MAX_KEYWORDS` valid trigger keywords in one call.
    fn registerToolWithKeywords(
        *_,
        tool_id: u32,
        name: []const u8,
        domain_mask: u64,
        risk_tier: ToolRiskTier,
        schema_tokens: u32,
        priority: u16,
        pinned: bool,
        keywords: []const []const u8,
    ): usize? {
        const slot = _.registerTool(tool_id, name, domain_mask, risk_tier, schema_tokens, priority, pinned) ?? return null;
        let i: usize = 0;
        while (i < keywords.len && _.entries[slot].kw_count < TOOL_ROUTER_MAX_KEYWORDS) {
            * = _.addKeyword(tool_id, keywords[i]);
            i += 1;
        }
        return slot;
    }

    /// Removes a tool by `tool_id`. Returns `true` if found and removed.
    fn removeTool(*_, tool_id: u32): bool {
        const slot = _.findById(tool_id) ?? return false;
        _.entries[slot] = ToolSpecEntry{};
        if (_.active_count > 0) {
            _.active_count -= 1;
        }
        _.telemetry.total_removed += 1;
        return true;
    }

    /// Updates the `pinned` status of an existing tool by `tool_id`.
    fn setPinned(*_, tool_id: u32, pinned: bool): bool {
        const slot = _.findById(tool_id) ?? return false;
        _.entries[slot].pinned = pinned;
        return true;
    }

    /// Routes tools for a query under capability domain mask, risk ceiling, token budget, and tool count cap.
    ///
    /// Phase 1: Packs risk-compliant `pinned` tools first (ordered by priority desc, schema_tokens asc, tool_id asc).
    /// Phase 2: Scores remaining risk-compliant tools (domain `@popCount` + case-insensitive keyword hit rate
    ///          + direct name match + priority) and greedily packs in descending score order within remaining budget.
    fn routeTools(
        *_,
        query: []const u8,
        required_domain_mask: u64,
        max_risk_tier: ToolRiskTier,
        token_budget: u32,
        max_tools: usize,
        out_picks: *[]RoutedToolPick,
    ): ToolRouteReport {
        let report = ToolRouteReport{};
        const max_risk_rank: u8 = toolRiskRank(max_risk_tier);
        const eff_max_tools: usize = if (max_tools < out_picks.len) max_tools else out_picks.len;
        const budget_u64: u64 = @as(u64, token_budget);

        let pinned_cands: [TOOL_ROUTER_MAX_TOOLS]ToolCandidate = [ToolCandidate{}; TOOL_ROUTER_MAX_TOOLS];
        let pinned_count: usize = 0;

        let normal_cands: [TOOL_ROUTER_MAX_TOOLS]ToolCandidate = [ToolCandidate{}; TOOL_ROUTER_MAX_TOOLS];
        let normal_count: usize = 0;

        // Partition active catalog entries into risk-blocked, pinned, and scored candidates.
        let i: usize = 0;
        while (i < TOOL_ROUTER_MAX_TOOLS) {
            if (_.entries[i].active) {
                report.catalog_total_tools += 1;
                report.catalog_total_tokens += @as(u64, _.entries[i].schema_tokens);

                if (toolRiskRank(_.entries[i].risk_tier) > max_risk_rank) {
                    report.skipped_by_risk += 1;
                } else {
                    let cand = scoreEntry(_.entries[i], query, required_domain_mask, _.weights);
                    cand.slot_idx = i;

                    if (_.entries[i].pinned) {
                        pinned_cands[pinned_count] = cand;
                        pinned_count += 1;
                    } else {
                        if (_.weights.min_score_threshold > 0 && cand.score < _.weights.min_score_threshold) {
                            report.skipped_by_threshold += 1;
                        } else {
                            normal_cands[normal_count] = cand;
                            normal_count += 1;
                        }
                    }
                }
            }
            i += 1;
        }

        // Deterministic insertion sort for pinned candidates.
        let p: usize = 1;
        while (p < pinned_count) {
            const key = pinned_cands[p];
            let j: usize = p;
            while (j > 0 && pinnedCandidateBetter(key, pinned_cands[j - 1])) {
                pinned_cands[j] = pinned_cands[j - 1];
                j -= 1;
            }
            pinned_cands[j] = key;
            p += 1;
        }

        // Phase 1: Lock pinned core tools first within budget & max_tools cap.
        let pi: usize = 0;
        while (pi < pinned_count) {
            const cand = pinned_cands[pi];
            const cost: u64 = @as(u64, cand.schema_tokens);
            if (report.selected_count >= eff_max_tools) {
                report.skipped_by_limit += 1;
            } else if (report.consumed_tokens + cost > budget_u64) {
                report.pinned_budget_overflow = true;
                report.skipped_by_budget += 1;
            } else {
                out_picks[report.selected_count] = RoutedToolPick{
                    tool_id: cand.tool_id,
                    slot_index: cand.slot_idx,
                    score: cand.score,
                    schema_tokens: cand.schema_tokens,
                    domain_overlap: cand.domain_overlap,
                    keyword_hits: cand.keyword_hits,
                    pinned: true,
                    risk_tier: _.entries[cand.slot_idx].risk_tier,
                };
                report.consumed_tokens += cost;
                report.selected_count += 1;
                report.pinned_selected_count += 1;
            }
            pi += 1;
        }

        // Deterministic insertion sort for non-pinned scored candidates.
        let s: usize = 1;
        while (s < normal_count) {
            const key = normal_cands[s];
            let j: usize = s;
            while (j > 0 && scoredCandidateBetter(key, normal_cands[j - 1])) {
                normal_cands[j] = normal_cands[j - 1];
                j -= 1;
            }
            normal_cands[j] = key;
            s += 1;
        }

        // Phase 2: Greedily pack remaining scored candidates within remaining token_budget.
        let ni: usize = 0;
        while (ni < normal_count) {
            const cand = normal_cands[ni];
            const cost: u64 = @as(u64, cand.schema_tokens);
            if (report.selected_count >= eff_max_tools) {
                report.skipped_by_limit += 1;
            } else if (report.consumed_tokens + cost > budget_u64) {
                report.skipped_by_budget += 1;
            } else {
                out_picks[report.selected_count] = RoutedToolPick{
                    tool_id: cand.tool_id,
                    slot_index: cand.slot_idx,
                    score: cand.score,
                    schema_tokens: cand.schema_tokens,
                    domain_overlap: cand.domain_overlap,
                    keyword_hits: cand.keyword_hits,
                    pinned: false,
                    risk_tier: _.entries[cand.slot_idx].risk_tier,
                };
                report.consumed_tokens += cost;
                report.selected_count += 1;
            }
            ni += 1;
        }

        // Compute token savings & permille ratio.
        if (report.catalog_total_tokens >= report.consumed_tokens) {
            report.saved_tokens = report.catalog_total_tokens - report.consumed_tokens;
        } else {
            report.saved_tokens = 0;
        }
        if (report.catalog_total_tokens > 0) {
            report.savings_permille = @as(u32, @intCast((report.saved_tokens * 1000) / report.catalog_total_tokens));
        } else {
            report.savings_permille = 0;
        }

        // Update cumulative router telemetry.
        _.telemetry.total_routes += 1;
        _.telemetry.total_picks_emitted += @as(u64, report.selected_count);
        _.telemetry.total_tokens_consumed += report.consumed_tokens;
        _.telemetry.total_tokens_saved += report.saved_tokens;
        _.telemetry.total_risk_blocked += @as(u64, report.skipped_by_risk);
        _.telemetry.total_budget_blocked += @as(u64, report.skipped_by_budget);

        return report;
    }
}
