// Origin: Pure-CP Stdlib (Zero-Alloc MCP Session/Tool Dual Token-Bucket Rate Limiter & Cost Quota Gate) | Created: 2026-11-23
// Pure CP zero-heap-allocation multi-tenant MCP rate-limit, concurrency, and cost-budget gate:
//   - Fixed 32-session / 64-tool-rule inline tables with zero dynamic heap allocation
//   - Millisecond-accurate fixed-point integer token buckets (1000 milli-tokens/token, zero float error)
//     at both session-global and per-tool layers with exact ceiling `retry_after_ms` calculation
//   - Strict atomic `acquire`: validates session existence, dual-layer concurrency caps, dual-layer
//     token buckets, and cost budget BEFORE mutating any state (zero side effects on any denial)
//   - Concurrency `release`, failed-call `refundCost`, telemetry `stats()`, and JSON-RPC `-32029`
//     error payload formatting (`formatQuotaErrorJson`)

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Fixed maximum number of concurrent MCP sessions tracked inline.
pub const MCP_QUOTA_MAX_SESSIONS: usize = 32;

/// Fixed maximum number of per-tool quota rules stored inline.
pub const MCP_QUOTA_MAX_RULES: usize = 64;

/// Maximum UTF-8 byte length of a session ID or tool name stored inline.
pub const MCP_QUOTA_ID_MAX_LEN: usize = 48;

/// Fixed-point milli-token scale (1 whole token = 1000 milli-tokens).
pub const MCP_QUOTA_MILLI_SCALE: u64 = 1000;

/// Standard JSON-RPC 2.0 server error code for rate/quota exhaustion.
pub const MCP_QUOTA_ERR_RATE_LIMITED: i32 = -32029;

/// Admission verdict returned by `McpRateQuotaEngine.acquire`.
pub enum McpQuotaVerdict {
    allowed,
    denied_rate_limit,
    denied_cost_budget,
    denied_concurrency,
    denied_unknown_session,
}

/// Per-tool rate limit, cost, and concurrency rule stored inline.
pub struct McpToolQuotaRule {
    active: bool = false;
    tool_name_buf: [MCP_QUOTA_ID_MAX_LEN]u8 = [0; MCP_QUOTA_ID_MAX_LEN];
    tool_name_len: u16 = 0;
    burst_capacity: u32 = 0;
    refill_per_sec: u32 = 0;
    cost_micros: u32 = 0;
    max_concurrent: u16 = 0;
    tokens_milli: u64 = 0;
    last_refill_ms: u64 = 0;
    bucket_initialized: bool = false;
    active_calls: u16 = 0;

    /// Return the registered tool name slice.
    fn tool_name(): []const u8 {
        return tool_name_buf[0..@as(usize, tool_name_len)];
    }
}

/// Per-session cost budget, global token bucket, and concurrency tracker.
pub struct McpSessionQuota {
    active: bool = false;
    session_id_buf: [MCP_QUOTA_ID_MAX_LEN]u8 = [0; MCP_QUOTA_ID_MAX_LEN];
    session_id_len: u16 = 0;
    max_cost_micros: u64 = 0;
    spent_cost_micros: u64 = 0;
    global_burst: u32 = 0;
    global_refill_per_sec: u32 = 0;
    global_tokens_milli: u64 = 0;
    last_refill_ms: u64 = 0;
    active_calls: u16 = 0;
    max_concurrent: u16 = 0;

    /// Return the registered session identifier slice.
    fn session_id(): []const u8 {
        return session_id_buf[0..@as(usize, session_id_len)];
    }

    /// Return remaining unspent cost budget in micro-dollars.
    fn remainingCostMicros(): u64 {
        if (max_cost_micros > spent_cost_micros) {
            return max_cost_micros - spent_cost_micros;
        }
        return 0;
    }
}

/// Detailed admission decision returned by `McpRateQuotaEngine.acquire`.
pub struct McpQuotaDecision {
    verdict: McpQuotaVerdict = McpQuotaVerdict.denied_unknown_session;
    retry_after_ms: u64 = 0;
    charged_cost_micros: u32 = 0;
    remaining_cost_micros: u64 = 0;
    remaining_tokens: u32 = 0;
}

/// Cumulative telemetry counters for `McpRateQuotaEngine`.
pub struct McpQuotaStats {
    total_checks: u64 = 0;
    allowed_count: u64 = 0;
    denied_rate_count: u64 = 0;
    denied_cost_count: u64 = 0;
    denied_concurrency_count: u64 = 0;
    total_cost_micros_charged: u64 = 0;
}

// ============================================================================
// 2. Pure Helper Functions
// ============================================================================

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn previewRefillMilli(
    current_milli: u64,
    last_refill_ms: u64,
    burst_capacity: u32,
    refill_per_sec: u32,
    now_ms: u64,
): u64 {
    const cap_milli: u64 = @as(u64, burst_capacity) * MCP_QUOTA_MILLI_SCALE;
    if (cap_milli == 0) return 0;
    let cur = if (current_milli > cap_milli) cap_milli else current_milli;
    if (now_ms > last_refill_ms && refill_per_sec > 0) {
        const elapsed_ms: u128 = @as(u128, now_ms - last_refill_ms);
        const added_milli: u128 = elapsed_ms * @as(u128, refill_per_sec);
        const total_milli: u128 = @as(u128, cur) + added_milli;
        if (total_milli >= @as(u128, cap_milli)) {
            cur = cap_milli;
        } else {
            cur = @as(u64, @intCast(total_milli));
        }
    }
    return cur;
}

fn computeRetryAfterMs(current_milli: u64, refill_per_sec: u32): u64 {
    if (current_milli >= MCP_QUOTA_MILLI_SCALE) return 0;
    if (refill_per_sec == 0) return 0;
    const deficit: u64 = MCP_QUOTA_MILLI_SCALE - current_milli;
    const rate: u64 = @as(u64, refill_per_sec);
    return (deficit + rate - 1) / rate;
}

fn appendBytes(out_buf: *[]u8, pos: *usize, s: []const u8): bool {
    if (pos + s.len > out_buf.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos + i] = s[i];
    }
    pos += s.len;
    return true;
}

fn appendU64(out_buf: *[]u8, pos: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) : (n += 1) {
            tmp[n] = @as(u8, @intCast('0' + (v % 10)));
            v = v / 10;
        }
    }
    if (pos + n > out_buf.len) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out_buf[pos + i] = tmp[n - 1 - i];
    }
    pos += n;
    return true;
}

fn verdictName(v: McpQuotaVerdict): []const u8 {
    if (v == McpQuotaVerdict.allowed) return "allowed";
    if (v == McpQuotaVerdict.denied_rate_limit) return "denied_rate_limit";
    if (v == McpQuotaVerdict.denied_cost_budget) return "denied_cost_budget";
    if (v == McpQuotaVerdict.denied_concurrency) return "denied_concurrency";
    return "denied_unknown_session";
}

fn verdictMessage(v: McpQuotaVerdict): []const u8 {
    if (v == McpQuotaVerdict.denied_rate_limit) return "rate limit exceeded";
    if (v == McpQuotaVerdict.denied_cost_budget) return "cost budget exceeded";
    if (v == McpQuotaVerdict.denied_concurrency) return "concurrency limit exceeded";
    if (v == McpQuotaVerdict.denied_unknown_session) return "unknown session";
    return "ok";
}

/// Format a standard JSON-RPC 2.0 `-32029` quota/rate-limit error object including `retryAfterMs`.
/// Returns bytes written, or `null` if `out_buf` is too small.
pub fn formatQuotaErrorJson(decision: McpQuotaDecision, out_buf: *[]u8): usize? {
    let pos: usize = 0;
    if (!appendBytes(out_buf, *pos, "{\"code\":-32029,\"message\":\"")) return null;
    if (!appendBytes(out_buf, *pos, verdictMessage(decision.verdict))) return null;
    if (!appendBytes(out_buf, *pos, "\",\"data\":{\"verdict\":\"")) return null;
    if (!appendBytes(out_buf, *pos, verdictName(decision.verdict))) return null;
    if (!appendBytes(out_buf, *pos, "\",\"retryAfterMs\":")) return null;
    if (!appendU64(out_buf, *pos, decision.retry_after_ms)) return null;
    if (!appendBytes(out_buf, *pos, ",\"remainingCostMicros\":")) return null;
    if (!appendU64(out_buf, *pos, decision.remaining_cost_micros)) return null;
    if (!appendBytes(out_buf, *pos, ",\"remainingTokens\":")) return null;
    if (!appendU64(out_buf, *pos, @as(u64, decision.remaining_tokens))) return null;
    if (!appendBytes(out_buf, *pos, "}}")) return null;
    return pos;
}

// ============================================================================
// 3. McpRateQuotaEngine — Dual-Layer Rate Limiter & Cost Quota Gate
// ============================================================================

/// Zero-heap-allocation dual-layer (session + tool) MCP rate limiter, concurrency gate,
/// and cost budget governor.
pub struct McpRateQuotaEngine {
    sessions: [MCP_QUOTA_MAX_SESSIONS]McpSessionQuota = [McpSessionQuota{}; MCP_QUOTA_MAX_SESSIONS];
    rules: [MCP_QUOTA_MAX_RULES]McpToolQuotaRule = [McpToolQuotaRule{}; MCP_QUOTA_MAX_RULES];
    total_checks: u64 = 0;
    allowed_count: u64 = 0;
    denied_rate_count: u64 = 0;
    denied_cost_count: u64 = 0;
    denied_concurrency_count: u64 = 0;
    total_cost_micros_charged: u64 = 0;

    /// Initialize an empty `McpRateQuotaEngine`.
    static fn init(): McpRateQuotaEngine {
        return McpRateQuotaEngine{};
    }

    fn findSessionIndex(session_id: []const u8): usize? {
        if (session_id.len == 0 || session_id.len > MCP_QUOTA_ID_MAX_LEN) return null;
        let i: usize = 0;
        while (i < MCP_QUOTA_MAX_SESSIONS) : (i += 1) {
            if (!sessions[i].active) continue;
            const cur: []const u8 = sessions[i].session_id_buf[0..@as(usize, sessions[i].session_id_len)];
            if (bytesEqual(cur, session_id)) return i;
        }
        return null;
    }

    fn findRuleIndex(tool_name: []const u8): usize? {
        if (tool_name.len == 0 || tool_name.len > MCP_QUOTA_ID_MAX_LEN) return null;
        let i: usize = 0;
        while (i < MCP_QUOTA_MAX_RULES) : (i += 1) {
            if (!rules[i].active) continue;
            const cur: []const u8 = rules[i].tool_name_buf[0..@as(usize, rules[i].tool_name_len)];
            if (bytesEqual(cur, tool_name)) return i;
        }
        return null;
    }

    /// Register or update a per-tool quota rule.
    /// Returns the rule slot index, or `null` if `tool_name` is empty / too long / table is full.
    fn registerToolRule(
        *_,
        tool_name: []const u8,
        burst_capacity: u32,
        refill_per_sec: u32,
        cost_micros: u32,
        max_concurrent: u16,
    ): usize? {
        if (tool_name.len == 0 || tool_name.len > MCP_QUOTA_ID_MAX_LEN) return null;

        if (existing := findRuleIndex(tool_name)) {
            rules[existing].burst_capacity = burst_capacity;
            rules[existing].refill_per_sec = refill_per_sec;
            rules[existing].cost_micros = cost_micros;
            rules[existing].max_concurrent = max_concurrent;
            rules[existing].tokens_milli = @as(u64, burst_capacity) * MCP_QUOTA_MILLI_SCALE;
            rules[existing].bucket_initialized = false;
            return existing;
        }

        let i: usize = 0;
        while (i < MCP_QUOTA_MAX_RULES) : (i += 1) {
            if (!rules[i].active) {
                let r = McpToolQuotaRule{};
                r.active = true;
                let k: usize = 0;
                while (k < tool_name.len) : (k += 1) {
                    r.tool_name_buf[k] = tool_name[k];
                }
                r.tool_name_len = @as(u16, @intCast(tool_name.len));
                r.burst_capacity = burst_capacity;
                r.refill_per_sec = refill_per_sec;
                r.cost_micros = cost_micros;
                r.max_concurrent = max_concurrent;
                r.tokens_milli = @as(u64, burst_capacity) * MCP_QUOTA_MILLI_SCALE;
                r.bucket_initialized = false;
                rules[i] = r;
                return i;
            }
        }
        return null;
    }

    /// Register or reset a session quota entry at `now_ms`.
    /// Returns the session slot index, or `null` if `session_id` is empty / too long / table is full.
    fn registerSession(
        *_,
        session_id: []const u8,
        max_cost_micros: u64,
        global_burst: u32,
        global_refill_per_sec: u32,
        max_concurrent: u16,
        now_ms: u64,
    ): usize? {
        if (session_id.len == 0 || session_id.len > MCP_QUOTA_ID_MAX_LEN) return null;

        if (existing := findSessionIndex(session_id)) {
            sessions[existing].max_cost_micros = max_cost_micros;
            sessions[existing].spent_cost_micros = 0;
            sessions[existing].global_burst = global_burst;
            sessions[existing].global_refill_per_sec = global_refill_per_sec;
            sessions[existing].global_tokens_milli = @as(u64, global_burst) * MCP_QUOTA_MILLI_SCALE;
            sessions[existing].last_refill_ms = now_ms;
            sessions[existing].active_calls = 0;
            sessions[existing].max_concurrent = max_concurrent;
            return existing;
        }

        let i: usize = 0;
        while (i < MCP_QUOTA_MAX_SESSIONS) : (i += 1) {
            if (!sessions[i].active) {
                let s = McpSessionQuota{};
                s.active = true;
                let k: usize = 0;
                while (k < session_id.len) : (k += 1) {
                    s.session_id_buf[k] = session_id[k];
                }
                s.session_id_len = @as(u16, @intCast(session_id.len));
                s.max_cost_micros = max_cost_micros;
                s.spent_cost_micros = 0;
                s.global_burst = global_burst;
                s.global_refill_per_sec = global_refill_per_sec;
                s.global_tokens_milli = @as(u64, global_burst) * MCP_QUOTA_MILLI_SCALE;
                s.last_refill_ms = now_ms;
                s.active_calls = 0;
                s.max_concurrent = max_concurrent;
                sessions[i] = s;
                return i;
            }
        }
        return null;
    }

    /// Look up a session snapshot by `session_id`.
    fn getSession(session_id: []const u8): McpSessionQuota? {
        const idx = findSessionIndex(session_id) ?? return null;
        return sessions[idx];
    }

    /// Look up a tool rule snapshot by `tool_name`.
    fn getToolRule(tool_name: []const u8): McpToolQuotaRule? {
        const idx = findRuleIndex(tool_name) ?? return null;
        return rules[idx];
    }

    /// Atomically evaluate and acquire a call permit for `(session_id, tool_name)` at `now_ms`.
    /// Validates in strict order:
    ///   1. Session existence
    ///   2. Session & tool concurrency limits
    ///   3. Session & tool millisecond token buckets (with exact `retry_after_ms`)
    ///   4. Session cost budget
    /// Any denial is guaranteed zero-side-effect (no tokens deducted, no cost charged, no active_calls incremented).
    fn acquire(*_, session_id: []const u8, tool_name: []const u8, now_ms: u64): McpQuotaDecision {
        total_checks += 1;

        const s_idx = findSessionIndex(session_id) ?? return McpQuotaDecision{
            verdict: McpQuotaVerdict.denied_unknown_session,
        };

        let has_rule = false;
        let r_idx: usize = 0;
        if (found_r := findRuleIndex(tool_name)) {
            has_rule = true;
            r_idx = found_r;
        }

        // Preview refilled token levels WITHOUT mutating state yet.
        const sess_milli: u64 = previewRefillMilli(
            sessions[s_idx].global_tokens_milli,
            sessions[s_idx].last_refill_ms,
            sessions[s_idx].global_burst,
            sessions[s_idx].global_refill_per_sec,
            now_ms,
        );
        let tool_milli: u64 = 0;
        if (has_rule && rules[r_idx].burst_capacity > 0) {
            const base_ts: u64 = if (rules[r_idx].bucket_initialized) rules[r_idx].last_refill_ms else now_ms;
            tool_milli = previewRefillMilli(
                rules[r_idx].tokens_milli,
                base_ts,
                rules[r_idx].burst_capacity,
                rules[r_idx].refill_per_sec,
                now_ms,
            );
        }

        const sess_rem_tok_preview: u32 = @as(u32, @intCast(sess_milli / MCP_QUOTA_MILLI_SCALE));

        // 1. Concurrency gate (session & tool)
        const sess_conc_full = sessions[s_idx].max_concurrent > 0 && sessions[s_idx].active_calls >= sessions[s_idx].max_concurrent;
        const tool_conc_full = has_rule && rules[r_idx].max_concurrent > 0 && rules[r_idx].active_calls >= rules[r_idx].max_concurrent;
        if (sess_conc_full || tool_conc_full) {
            denied_concurrency_count += 1;
            return McpQuotaDecision{
                verdict: McpQuotaVerdict.denied_concurrency,
                retry_after_ms: 0,
                charged_cost_micros: 0,
                remaining_cost_micros: sessions[s_idx].remainingCostMicros(),
                remaining_tokens: sess_rem_tok_preview,
            };
        }

        // 2. Dual token-bucket rate limit gate
        const sess_rate_denied = sessions[s_idx].global_burst > 0 && sess_milli < MCP_QUOTA_MILLI_SCALE;
        const tool_rate_denied = has_rule && rules[r_idx].burst_capacity > 0 && tool_milli < MCP_QUOTA_MILLI_SCALE;
        if (sess_rate_denied || tool_rate_denied) {
            let retry_ms: u64 = 0;
            if (sess_rate_denied) {
                retry_ms = computeRetryAfterMs(sess_milli, sessions[s_idx].global_refill_per_sec);
            }
            if (tool_rate_denied) {
                const tr = computeRetryAfterMs(tool_milli, rules[r_idx].refill_per_sec);
                if (tr > retry_ms) retry_ms = tr;
            }
            denied_rate_count += 1;
            return McpQuotaDecision{
                verdict: McpQuotaVerdict.denied_rate_limit,
                retry_after_ms: retry_ms,
                charged_cost_micros: 0,
                remaining_cost_micros: sessions[s_idx].remainingCostMicros(),
                remaining_tokens: 0,
            };
        }

        // 3. Cost budget gate
        const cost_u32: u32 = if (has_rule) rules[r_idx].cost_micros else 0;
        const cost_u64: u64 = @as(u64, cost_u32);
        if (sessions[s_idx].spent_cost_micros + cost_u64 > sessions[s_idx].max_cost_micros) {
            denied_cost_count += 1;
            return McpQuotaDecision{
                verdict: McpQuotaVerdict.denied_cost_budget,
                retry_after_ms: 0,
                charged_cost_micros: 0,
                remaining_cost_micros: sessions[s_idx].remainingCostMicros(),
                remaining_tokens: sess_rem_tok_preview,
            };
        }

        // 4. All checks passed -> Commit atomically!
        if (sessions[s_idx].global_burst > 0) {
            sessions[s_idx].global_tokens_milli = sess_milli - MCP_QUOTA_MILLI_SCALE;
            if (now_ms > sessions[s_idx].last_refill_ms) {
                sessions[s_idx].last_refill_ms = now_ms;
            }
        }
        sessions[s_idx].spent_cost_micros += cost_u64;
        sessions[s_idx].active_calls += 1;

        let rem_tokens: u32 = @as(u32, @intCast(sessions[s_idx].global_tokens_milli / MCP_QUOTA_MILLI_SCALE));
        if (has_rule) {
            if (rules[r_idx].burst_capacity > 0) {
                rules[r_idx].tokens_milli = tool_milli - MCP_QUOTA_MILLI_SCALE;
                if (!rules[r_idx].bucket_initialized || now_ms > rules[r_idx].last_refill_ms) {
                    rules[r_idx].last_refill_ms = now_ms;
                }
                rules[r_idx].bucket_initialized = true;
                const tool_rem: u32 = @as(u32, @intCast(rules[r_idx].tokens_milli / MCP_QUOTA_MILLI_SCALE));
                if (sessions[s_idx].global_burst == 0 || tool_rem < rem_tokens) {
                    rem_tokens = tool_rem;
                }
            }
            rules[r_idx].active_calls += 1;
        }

        allowed_count += 1;
        total_cost_micros_charged += cost_u64;

        return McpQuotaDecision{
            verdict: McpQuotaVerdict.allowed,
            retry_after_ms: 0,
            charged_cost_micros: cost_u32,
            remaining_cost_micros: sessions[s_idx].remainingCostMicros(),
            remaining_tokens: rem_tokens,
        };
    }

    /// Release an in-flight concurrency slot for `(session_id, tool_name)`.
    /// Returns `true` if the session was found and decremented.
    fn release(*_, session_id: []const u8, tool_name: []const u8): bool {
        const s_idx = findSessionIndex(session_id) ?? return false;
        if (sessions[s_idx].active_calls > 0) {
            sessions[s_idx].active_calls -= 1;
        }
        if (r_idx := findRuleIndex(tool_name)) {
            if (rules[r_idx].active_calls > 0) {
                rules[r_idx].active_calls -= 1;
            }
        }
        return true;
    }

    /// Refund previously charged cost (e.g. when a tool invocation fails before incurring upstream cost).
    /// Saturates cleanly at `0` spent cost. Returns `true` if `session_id` exists.
    fn refundCost(*_, session_id: []const u8, refund_micros: u64): bool {
        const s_idx = findSessionIndex(session_id) ?? return false;
        if (refund_micros >= sessions[s_idx].spent_cost_micros) {
            sessions[s_idx].spent_cost_micros = 0;
        } else {
            sessions[s_idx].spent_cost_micros -= refund_micros;
        }
        return true;
    }

    /// Return global rate-quota telemetry statistics.
    fn stats(): McpQuotaStats {
        return McpQuotaStats{
            total_checks: total_checks,
            allowed_count: allowed_count,
            denied_rate_count: denied_rate_count,
            denied_cost_count: denied_cost_count,
            denied_concurrency_count: denied_concurrency_count,
            total_cost_micros_charged: total_cost_micros_charged,
        };
    }
}
