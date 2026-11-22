// Origin: Pure-CP Stdlib (AI Prompt-Cache Breakpoint Planner & Prefix Stability Optimizer) | Created: 2026-11-22
// Zero-heap-allocation multi-turn LLM Prompt-Cache breakpoint planner and prefix stability analyzer
// modeled after Anthropic / OpenAI Prefix Caching economics (write overhead vs multi-turn read savings).

/// Semantic role of a prompt segment in an LLM request.
pub enum PromptBlockKind {
    SystemPrompt,
    ToolDefs,
    FewShot,
    DocContext,
    HistoryTurn,
    CurrentUser,
}

/// Metadata descriptor for a single ordered prompt block.
pub struct PromptBlockDesc {
    index: u16 = 0;
    kind: PromptBlockKind = PromptBlockKind.SystemPrompt;
    tokens: u32 = 0;
    content_hash: u64 = 0;
    volatile_flag: bool = false;
}

/// Economic and structural configuration for Prompt-Cache breakpoint planning.
pub struct CachePlanConfig {
    max_breakpoints: u8 = 4;
    min_cacheable_tokens: u32 = 1024;
    expected_future_turns: u32 = 3;
    write_overhead_pct: u32 = 25;
    read_discount_pct: u32 = 90;
}

/// Planned cache breakpoint summary and net token-equivalent savings estimate.
pub struct CachePlanResult {
    breakpoint_count: usize = 0;
    cached_prefix_tokens: u32 = 0;
    estimated_net_savings_tokens: i64 = 0;
    worth_caching: bool = false;
}

/// Identifies the longest stable non-volatile prefix in `blocks`, evaluates multi-turn caching
/// net savings against `cfg.write_overhead_pct` and `cfg.read_discount_pct`, and places up to
/// `min(cfg.max_breakpoints, out_breakpoint_indices.len)` semantic breakpoints into
/// `out_breakpoint_indices`.
/// Returns `null` if `cfg.max_breakpoints == 0` or `cfg.read_discount_pct > 100`.
pub fn planPromptCacheBreakpoints(
    blocks: []const PromptBlockDesc,
    cfg: CachePlanConfig,
    out_breakpoint_indices: *[]u16,
): CachePlanResult? {
    if (cfg.max_breakpoints == 0) return null;
    if (cfg.read_discount_pct > 100) return null;

    if (blocks.len == 0) {
        return CachePlanResult{
            breakpoint_count: 0,
            cached_prefix_tokens: 0,
            estimated_net_savings_tokens: 0,
            worth_caching: false,
        };
    }

    // 1. Identify longest contiguous stable prefix (stop at first volatile block or CurrentUser).
    let stable_end: usize = 0;
    let stable_tokens_u64: u64 = 0;
    while (stable_end < blocks.len) : (stable_end += 1) {
        const blk = blocks[stable_end];
        if (blk.volatile_flag || blk.kind == PromptBlockKind.CurrentUser) {
            break;
        }
        stable_tokens_u64 += u64(blk.tokens);
    }

    const stable_tokens: u32 = if (stable_tokens_u64 > 0xFFFFFFFF) 0xFFFFFFFF else u32(stable_tokens_u64);

    if (stable_end == 0 || stable_tokens < cfg.min_cacheable_tokens) {
        return CachePlanResult{
            breakpoint_count: 0,
            cached_prefix_tokens: 0,
            estimated_net_savings_tokens: 0,
            worth_caching: false,
        };
    }

    // 2. Evaluate net token-equivalent savings:
    //    net = (T * read_discount_pct * expected_future_turns) / 100 - (T * write_overhead_pct) / 100
    const t_i64: i64 = i64(stable_tokens);
    const write_extra: i64 = (t_i64 * i64(cfg.write_overhead_pct)) / 100;
    const read_savings: i64 = (t_i64 * i64(cfg.read_discount_pct) * i64(cfg.expected_future_turns)) / 100;
    const net_savings: i64 = read_savings - write_extra;

    if (net_savings <= 0) {
        return CachePlanResult{
            breakpoint_count: 0,
            cached_prefix_tokens: 0,
            estimated_net_savings_tokens: net_savings,
            worth_caching: false,
        };
    }

    // 3. Place semantic breakpoints up to max_breakpoints, always reserving the last slot for
    //    the final stable block so the entire stable prefix is cached.
    const cfg_cap: usize = usize(cfg.max_breakpoints);
    const cap: usize = if (cfg_cap < out_breakpoint_indices.len) cfg_cap else out_breakpoint_indices.len;

    if (cap == 0) {
        return CachePlanResult{
            breakpoint_count: 0,
            cached_prefix_tokens: 0,
            estimated_net_savings_tokens: net_savings,
            worth_caching: false,
        };
    }

    let bp_count: usize = 0;
    let cum_tokens: u64 = 0;
    let since_last_bp: u64 = 0;

    let i: usize = 0;
    while (i + 1 < stable_end) : (i += 1) {
        cum_tokens += u64(blocks[i].tokens);
        since_last_bp += u64(blocks[i].tokens);

        if (bp_count + 1 < cap && cum_tokens >= u64(cfg.min_cacheable_tokens)) {
            const kind_boundary = blocks[i].kind != blocks[i + 1].kind;
            const stride_boundary = since_last_bp >= u64(cfg.min_cacheable_tokens);
            if (kind_boundary || stride_boundary) {
                out_breakpoint_indices[bp_count] = blocks[i].index;
                bp_count += 1;
                since_last_bp = 0;
            }
        }
    }

    // Always place the final breakpoint at the end of the stable prefix.
    out_breakpoint_indices[bp_count] = blocks[stable_end - 1].index;
    bp_count += 1;

    return CachePlanResult{
        breakpoint_count: bp_count,
        cached_prefix_tokens: stable_tokens,
        estimated_net_savings_tokens: net_savings,
        worth_caching: true,
    };
}

/// Computes the exact matching prefix token count between two consecutive turns,
/// terminating immediately at the first block whose `content_hash`, `kind`, or `tokens` diverges.
pub fn computePrefixMatchTokens(
    prev_blocks: []const PromptBlockDesc,
    curr_blocks: []const PromptBlockDesc,
): u32 {
    const limit: usize = if (prev_blocks.len < curr_blocks.len) prev_blocks.len else curr_blocks.len;
    let matched: u64 = 0;
    let i: usize = 0;
    while (i < limit) : (i += 1) {
        const p = prev_blocks[i];
        const c = curr_blocks[i];
        if (p.content_hash != c.content_hash || p.kind != c.kind || p.tokens != c.tokens) {
            break;
        }
        matched += u64(c.tokens);
    }
    if (matched > 0xFFFFFFFF) return 0xFFFFFFFF;
    return u32(matched);
}
