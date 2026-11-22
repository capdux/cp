// Origin: Pure-CP Stdlib (MCP 2025-03-26 Progress Token & Cancellation Notification FSM) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation MCP progress & cancellation state machine:
//   - Bounded fixed-capacity progress token registry (integer or string ProgressToken, request_id binding)
//   - Strictly monotonic progress updates with configurable min_interval_ms emission throttling
//   - Race-safe cancellation (early-cancel tombstones before register, late-cancel idempotency after complete)
//   - Zero-alloc JSON-RPC 2.0 notifications/progress & notifications/cancelled encoder/parser helpers

const std = @import("std");

/// Maximum fixed number of tracked progress token slots.
pub const MCP_PROGRESS_MAX_SLOTS: usize = 64;

/// Maximum fixed number of early-cancellation tombstones (cancel arriving before register).
pub const MCP_PROGRESS_MAX_TOMBSTONES: usize = 64;

/// Maximum byte length of a string `ProgressToken`.
pub const MCP_PROGRESS_TOKEN_MAX_LEN: usize = 64;

/// Maximum byte length of an optional progress status `message`.
pub const MCP_PROGRESS_MSG_MAX_LEN: usize = 128;

/// Maximum byte length of a `notifications/cancelled` `reason`.
pub const MCP_CANCEL_REASON_MAX_LEN: usize = 128;

// ============================================================================
// Enums & Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// Discriminator for MCP `ProgressToken = integer | string`.
pub enum McpProgressTokenKind: u8 {
    int_token = 0,
    str_token = 1,
}

/// Lifecycle state of a tracked MCP progress token / request.
pub enum McpProgressState: u8 {
    active = 0,
    completed = 1,
    cancelled = 2,
    timed_out = 3,
}

/// Outcome of `McpProgressHub.reportProgress`.
pub enum McpProgressReportStatus: u8 {
    emitted = 0,
    throttled = 1,
    non_monotonic = 2,
    not_found = 3,
    already_terminal = 4,
    invalid_value = 5,
}

/// Outcome of `McpProgressHub.cancelByRequestId`.
pub enum McpCancelOutcome: u8 {
    cancelled_active = 0,
    recorded_early_cancel = 1,
    already_cancelled = 2,
    already_completed = 3,
    invalid_or_full = 4,
}

/// MCP 2025-03-26 `ProgressToken` supporting both integer and bounded string tokens.
pub struct McpProgressToken {
    kind: McpProgressTokenKind = McpProgressTokenKind.int_token;
    int_val: i64 = 0;
    str_buf: [MCP_PROGRESS_TOKEN_MAX_LEN]u8 = [0; MCP_PROGRESS_TOKEN_MAX_LEN];
    str_len: usize = 0;

    static fn fromInt(v: i64): McpProgressToken {
        return McpProgressToken{
            kind: McpProgressTokenKind.int_token,
            int_val: v,
            str_buf: [0; MCP_PROGRESS_TOKEN_MAX_LEN],
            str_len: 0,
        };
    }

    static fn fromStr(s: []const u8): McpProgressToken? {
        if (s.len == 0 || s.len > MCP_PROGRESS_TOKEN_MAX_LEN) {
            return null;
        }
        let tok = McpProgressToken{
            kind: McpProgressTokenKind.str_token,
            int_val: 0,
            str_buf: [0; MCP_PROGRESS_TOKEN_MAX_LEN],
            str_len: s.len,
        };
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            tok.str_buf[i] = s[i];
        }
        return tok;
    }

    fn strSlice(): []const u8 {
        const n: usize = if (_.str_len <= MCP_PROGRESS_TOKEN_MAX_LEN) _.str_len else MCP_PROGRESS_TOKEN_MAX_LEN;
        return _.str_buf[0..n];
    }

    fn equals(other: McpProgressToken): bool {
        if (_.kind != other.kind) return false;
        if (_.kind == McpProgressTokenKind.int_token) {
            return _.int_val == other.int_val;
        }
        if (_.str_len != other.str_len) return false;
        let i: usize = 0;
        while (i < _.str_len) : (i += 1) {
            if (_.str_buf[i] != other.str_buf[i]) return false;
        }
        return true;
    }
}

/// Fixed-capacity slot tracking one MCP progress token and its associated request.
pub struct McpProgressSlot {
    token: McpProgressToken = McpProgressToken.fromInt(0);
    request_id: u64 = 0;
    state: McpProgressState = McpProgressState.active;
    progress: f64 = 0.0;
    has_progress: bool = false;
    total: f64? = null;
    msg_buf: [MCP_PROGRESS_MSG_MAX_LEN]u8 = [0; MCP_PROGRESS_MSG_MAX_LEN];
    msg_len: usize = 0;
    cancel_reason_buf: [MCP_CANCEL_REASON_MAX_LEN]u8 = [0; MCP_CANCEL_REASON_MAX_LEN];
    cancel_reason_len: usize = 0;
    created_ms: u64 = 0;
    deadline_ms: u64 = 0;
    last_emit_ms: u64 = 0;
    has_emitted: bool = false;
    min_interval_ms: u64 = 0;
    occupied: bool = false;

    static fn empty(): McpProgressSlot {
        return McpProgressSlot{};
    }

    fn messageSlice(): []const u8 {
        const n: usize = if (_.msg_len <= MCP_PROGRESS_MSG_MAX_LEN) _.msg_len else MCP_PROGRESS_MSG_MAX_LEN;
        return _.msg_buf[0..n];
    }

    fn cancelReasonSlice(): []const u8 {
        const n: usize = if (_.cancel_reason_len <= MCP_CANCEL_REASON_MAX_LEN) _.cancel_reason_len else MCP_CANCEL_REASON_MAX_LEN;
        return _.cancel_reason_buf[0..n];
    }

    /// Returns completion percentage in `[0.0, 100.0]` when `total > 0` is known, else `null`.
    fn percent(): f64? {
        if (t := _.total) {
            if (t <= 0.0) return null;
            const pct: f64 = (_.progress / t) * 100.0;
            if (pct < 0.0) return 0.0;
            if (pct > 100.0) return 100.0;
            return pct;
        }
        return null;
    }
}

/// Telemetry snapshot for `McpProgressHub`.
pub struct McpProgressHubStats {
    active_count: usize = 0;
    completed_count: usize = 0;
    cancelled_count: usize = 0;
    timed_out_count: usize = 0;
    throttled_drops: u64 = 0;
    early_cancel_hits: u64 = 0;
    total_registered: u64 = 0;
    total_emitted: u64 = 0;
}

/// Parsed `notifications/progress` payload produced by `parseProgressNotification`.
pub struct McpParsedProgressNotif {
    token: McpProgressToken = McpProgressToken.fromInt(0);
    progress: f64 = 0.0;
    total: f64? = null;
    msg_buf: [MCP_PROGRESS_MSG_MAX_LEN]u8 = [0; MCP_PROGRESS_MSG_MAX_LEN];
    msg_len: usize = 0;

    fn messageSlice(): []const u8 {
        const n: usize = if (_.msg_len <= MCP_PROGRESS_MSG_MAX_LEN) _.msg_len else MCP_PROGRESS_MSG_MAX_LEN;
        return _.msg_buf[0..n];
    }
}

/// Parsed `notifications/cancelled` payload produced by `parseCancelledNotification`.
pub struct McpParsedCancelNotif {
    request_id: u64 = 0;
    reason_buf: [MCP_CANCEL_REASON_MAX_LEN]u8 = [0; MCP_CANCEL_REASON_MAX_LEN];
    reason_len: usize = 0;

    fn reasonSlice(): []const u8 {
        const n: usize = if (_.reason_len <= MCP_CANCEL_REASON_MAX_LEN) _.reason_len else MCP_CANCEL_REASON_MAX_LEN;
        return _.reason_buf[0..n];
    }
}

struct McpEarlyCancelEntry {
    request_id: u64 = 0;
    reason_buf: [MCP_CANCEL_REASON_MAX_LEN]u8 = [0; MCP_CANCEL_REASON_MAX_LEN];
    reason_len: usize = 0;
    cancelled_ms: u64 = 0;
    expires_ms: u64 = 0;
    occupied: bool = false;

    static fn empty(): McpEarlyCancelEntry {
        return McpEarlyCancelEntry{};
    }
}

// ============================================================================
// Internal Helpers
// ============================================================================

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn copyBoundedMsg(src: []const u8, dst: *[MCP_PROGRESS_MSG_MAX_LEN]u8): usize {
    const n: usize = if (src.len <= MCP_PROGRESS_MSG_MAX_LEN) src.len else MCP_PROGRESS_MSG_MAX_LEN;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n;
}

fn copyBoundedReason(src: []const u8, dst: *[MCP_CANCEL_REASON_MAX_LEN]u8): usize {
    const n: usize = if (src.len <= MCP_CANCEL_REASON_MAX_LEN) src.len else MCP_CANCEL_REASON_MAX_LEN;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n;
}

fn appendRaw(out: *[]u8, cursor: *usize, src: []const u8): bool {
    if (cursor.* + src.len > out.len) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out[cursor.* + i] = src[i];
    }
    cursor.* += src.len;
    return true;
}

fn appendJsonEscaped(out: *[]u8, cursor: *usize, src: []const u8): bool {
    if (!appendRaw(out, cursor, "\"")) return false;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        const c: u8 = src[i];
        if (c == '"') {
            if (!appendRaw(out, cursor, "\\\"")) return false;
        } else if (c == '\\') {
            if (!appendRaw(out, cursor, "\\\\")) return false;
        } else if (c == '\n') {
            if (!appendRaw(out, cursor, "\\n")) return false;
        } else if (c == '\r') {
            if (!appendRaw(out, cursor, "\\r")) return false;
        } else if (c == '\t') {
            if (!appendRaw(out, cursor, "\\t")) return false;
        } else if (c < 0x20) {
            if (!appendRaw(out, cursor, " ")) return false;
        } else {
            if (cursor.* >= out.len) return false;
            out[cursor.*] = c;
            cursor.* += 1;
        }
    }
    return appendRaw(out, cursor, "\"");
}

fn appendU64(out: *[]u8, cursor: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = ('0' as u64 + (v % 10)) as u8;
            v /= 10;
            n += 1;
        }
    }
    if (cursor.* + n > out.len) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[cursor.* + i] = tmp[n - 1 - i];
    }
    cursor.* += n;
    return true;
}

fn appendI64(out: *[]u8, cursor: *usize, val: i64): bool {
    if (val < 0) {
        if (!appendRaw(out, cursor, "-")) return false;
        const mag: u64 = (0 - (val as i128)) as u64;
        return appendU64(out, cursor, mag);
    }
    return appendU64(out, cursor, val as u64);
}

/// Appends a non-negative finite `f64` formatted cleanly (integer when exact, else up to 4 fractional digits).
fn appendF64(out: *[]u8, cursor: *usize, val: f64): bool {
    if (!isFiniteF64(val) || val < 0.0) return false;
    const scaled: u64 = ((val * 10000.0) + 0.5) as u64;
    const int_part: u64 = scaled / 10000;
    let frac: u64 = scaled % 10000;
    if (!appendU64(out, cursor, int_part)) return false;
    if (frac == 0) return true;
    if (!appendRaw(out, cursor, ".")) return false;
    // Emit 4 digits trimming trailing zeros
    let digits: [4]u8 = [
        ('0' as u64 + (frac / 1000)) as u8,
        ('0' as u64 + ((frac / 100) % 10)) as u8,
        ('0' as u64 + ((frac / 10) % 10)) as u8,
        ('0' as u64 + (frac % 10)) as u8,
    ];
    let end: usize = 4;
    while (end > 1 && digits[end - 1] == '0') : (end -= 1) {}
    return appendRaw(out, cursor, digits[0..end]);
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Finds the value slice start right after `"key":` inside a JSON object slice.
fn findJsonFieldValueStart(json: []const u8, key: []const u8): usize? {
    let i: usize = 0;
    while (i < json.len) : (i += 1) {
        if (json[i] != '"') continue;
        const k_start: usize = i + 1;
        let k_end: usize = k_start;
        while (k_end < json.len) : (k_end += 1) {
            if (json[k_end] == '\\') {
                k_end += 1;
                continue;
            }
            if (json[k_end] == '"') break;
        }
        if (k_end >= json.len) return null;
        const found_key: []const u8 = json[k_start..k_end];
        i = k_end;
        if (!bytesEqual(found_key, key)) continue;

        let p: usize = k_end + 1;
        while (p < json.len && (json[p] == ' ' || json[p] == '\t' || json[p] == '\r' || json[p] == '\n')) : (p += 1) {}
        if (p < json.len && json[p] == ':') {
            p += 1;
            while (p < json.len && (json[p] == ' ' || json[p] == '\t' || json[p] == '\r' || json[p] == '\n')) : (p += 1) {}
            if (p < json.len) return p;
        }
    }
    return null;
}

fn parseJsonStringAt(json: []const u8, start: usize, out: *[]u8): usize? {
    if (start >= json.len || json[start] != '"') return null;
    let p: usize = start + 1;
    let written: usize = 0;
    while (p < json.len) : (p += 1) {
        const c: u8 = json[p];
        if (c == '"') {
            return written;
        }
        if (c == '\\') {
            p += 1;
            if (p >= json.len) return null;
            const esc: u8 = json[p];
            let ch: u8 = esc;
            if (esc == 'n') ch = '\n' as u8;
            else if (esc == 'r') ch = '\r' as u8;
            else if (esc == 't') ch = '\t' as u8;
            else if (esc == '"' || esc == '\\' || esc == '/') ch = esc;
            else return null;
            if (written >= out.len) return null;
            out[written] = ch;
            written += 1;
        } else {
            if (written >= out.len) return null;
            out[written] = c;
            written += 1;
        }
    }
    return null;
}

fn parseJsonI64At(json: []const u8, start: usize): i64? {
    if (start >= json.len) return null;
    let p: usize = start;
    let neg: bool = false;
    if (json[p] == '-') {
        neg = true;
        p += 1;
    }
    if (p >= json.len || json[p] < '0' || json[p] > '9') return null;
    let acc: u64 = 0;
    const max_i64: u64 = std.math.maxInt(i64) as u64;
    while (p < json.len && json[p] >= '0' && json[p] <= '9') : (p += 1) {
        const d: u64 = (json[p] - '0') as u64;
        if (acc > (max_i64 - d) / 10) return null;
        acc = acc * 10 + d;
    }
    if (neg) return 0 - (acc as i64);
    return acc as i64;
}

fn parseJsonF64At(json: []const u8, start: usize): f64? {
    if (start >= json.len) return null;
    let p: usize = start;
    let neg: bool = false;
    if (json[p] == '-') {
        neg = true;
        p += 1;
    }
    if (p >= json.len || json[p] < '0' || json[p] > '9') return null;
    let int_val: f64 = 0.0;
    while (p < json.len && json[p] >= '0' && json[p] <= '9') : (p += 1) {
        int_val = int_val * 10.0 + ((json[p] - '0') as f64);
    }
    let frac_val: f64 = 0.0;
    if (p < json.len && json[p] == '.') {
        p += 1;
        if (p >= json.len || json[p] < '0' || json[p] > '9') return null;
        let place: f64 = 0.1;
        while (p < json.len && json[p] >= '0' && json[p] <= '9') : (p += 1) {
            frac_val += ((json[p] - '0') as f64) * place;
            place *= 0.1;
        }
    }
    const res: f64 = int_val + frac_val;
    if (!isFiniteF64(res)) return null;
    return if (neg) (0.0 - res) else res;
}

// ============================================================================
// 1. Zero-Alloc JSON-RPC 2.0 Progress & Cancellation Wire Codec
// ============================================================================

/// Encodes a JSON-RPC 2.0 `notifications/progress` message into caller-provided `out_buf`.
/// Returns bytes written, or `null` if `out_buf` is undersized or numeric inputs are invalid.
pub fn encodeProgressNotification(
    token: McpProgressToken,
    progress: f64,
    total: f64?,
    message: []const u8,
    out_buf: *[]u8,
): usize? {
    if (!isFiniteF64(progress) || progress < 0.0) return null;
    if (t := total) {
        if (!isFiniteF64(t) || t <= 0.0) return null;
    }
    if (message.len > MCP_PROGRESS_MSG_MAX_LEN) return null;

    let cur: usize = 0;
    if (!appendRaw(out_buf, *cur, "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/progress\",\"params\":{\"progressToken\":")) return null;
    if (token.kind == McpProgressTokenKind.int_token) {
        if (!appendI64(out_buf, *cur, token.int_val)) return null;
    } else {
        if (!appendJsonEscaped(out_buf, *cur, token.strSlice())) return null;
    }
    if (!appendRaw(out_buf, *cur, ",\"progress\":")) return null;
    if (!appendF64(out_buf, *cur, progress)) return null;
    if (t := total) {
        if (!appendRaw(out_buf, *cur, ",\"total\":")) return null;
        if (!appendF64(out_buf, *cur, t)) return null;
    }
    if (message.len > 0) {
        if (!appendRaw(out_buf, *cur, ",\"message\":")) return null;
        if (!appendJsonEscaped(out_buf, *cur, message)) return null;
    }
    if (!appendRaw(out_buf, *cur, "}}")) return null;
    return cur;
}

/// Parses a JSON-RPC 2.0 `notifications/progress` message without heap allocation.
pub fn parseProgressNotification(json: []const u8): McpParsedProgressNotif? {
    const m_pos: usize = findJsonFieldValueStart(json, "method") ?? return null;
    let m_buf: [32]u8 = [0; 32];
    let m_s: []u8 = m_buf[0..];
    const m_len: usize = parseJsonStringAt(json, m_pos, *m_s) ?? return null;
    if (!bytesEqual(m_buf[0..m_len], "notifications/progress")) return null;

    const tok_pos: usize = findJsonFieldValueStart(json, "progressToken") ?? return null;
    let parsed = McpParsedProgressNotif{};
    if (json[tok_pos] == '"') {
        let t_buf: [MCP_PROGRESS_TOKEN_MAX_LEN]u8 = [0; MCP_PROGRESS_TOKEN_MAX_LEN];
        let t_s: []u8 = t_buf[0..];
        const t_len: usize = parseJsonStringAt(json, tok_pos, *t_s) ?? return null;
        parsed.token = McpProgressToken.fromStr(t_buf[0..t_len]) ?? return null;
    } else {
        const iv: i64 = parseJsonI64At(json, tok_pos) ?? return null;
        parsed.token = McpProgressToken.fromInt(iv);
    }

    const prog_pos: usize = findJsonFieldValueStart(json, "progress") ?? return null;
    const prog_val: f64 = parseJsonF64At(json, prog_pos) ?? return null;
    if (prog_val < 0.0) return null;
    parsed.progress = prog_val;

    if (tot_pos := findJsonFieldValueStart(json, "total")) {
        const tot_val: f64 = parseJsonF64At(json, tot_pos) ?? return null;
        if (tot_val <= 0.0) return null;
        parsed.total = tot_val;
    }

    if (msg_pos := findJsonFieldValueStart(json, "message")) {
        let msg_s: []u8 = parsed.msg_buf[0..];
        const msg_len: usize = parseJsonStringAt(json, msg_pos, *msg_s) ?? return null;
        parsed.msg_len = msg_len;
    }

    return parsed;
}

/// Encodes a JSON-RPC 2.0 `notifications/cancelled` message into caller-provided `out_buf`.
/// Returns bytes written, or `null` on `request_id == 0`, oversized reason, or undersized buffer.
pub fn encodeCancelledNotification(
    request_id: u64,
    reason: []const u8,
    out_buf: *[]u8,
): usize? {
    if (request_id == 0 || reason.len > MCP_CANCEL_REASON_MAX_LEN) return null;
    let cur: usize = 0;
    if (!appendRaw(out_buf, *cur, "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/cancelled\",\"params\":{\"requestId\":")) return null;
    if (!appendU64(out_buf, *cur, request_id)) return null;
    if (reason.len > 0) {
        if (!appendRaw(out_buf, *cur, ",\"reason\":")) return null;
        if (!appendJsonEscaped(out_buf, *cur, reason)) return null;
    }
    if (!appendRaw(out_buf, *cur, "}}")) return null;
    return cur;
}

/// Parses a JSON-RPC 2.0 `notifications/cancelled` message without heap allocation.
pub fn parseCancelledNotification(json: []const u8): McpParsedCancelNotif? {
    const m_pos: usize = findJsonFieldValueStart(json, "method") ?? return null;
    let m_buf: [32]u8 = [0; 32];
    let m_s: []u8 = m_buf[0..];
    const m_len: usize = parseJsonStringAt(json, m_pos, *m_s) ?? return null;
    if (!bytesEqual(m_buf[0..m_len], "notifications/cancelled")) return null;

    const id_pos: usize = findJsonFieldValueStart(json, "requestId") ?? return null;
    const raw_id: i64 = parseJsonI64At(json, id_pos) ?? return null;
    if (raw_id <= 0) return null;

    let parsed = McpParsedCancelNotif{ request_id: raw_id as u64 };
    if (r_pos := findJsonFieldValueStart(json, "reason")) {
        let r_s: []u8 = parsed.reason_buf[0..];
        const r_len: usize = parseJsonStringAt(json, r_pos, *r_s) ?? return null;
        parsed.reason_len = r_len;
    }
    return parsed;
}

// ============================================================================
// 2. Core MCP Progress & Race-Safe Cancellation State Machine
// ============================================================================

/// Zero-heap-allocation MCP 2025-03-26 progress token registry & race-safe cancellation manager.
pub struct McpProgressHub {
    slots: [MCP_PROGRESS_MAX_SLOTS]McpProgressSlot = [McpProgressSlot.empty(); MCP_PROGRESS_MAX_SLOTS];
    early_cancels: [MCP_PROGRESS_MAX_TOMBSTONES]McpEarlyCancelEntry = [McpEarlyCancelEntry.empty(); MCP_PROGRESS_MAX_TOMBSTONES];
    default_min_interval_ms: u64 = 0;
    early_cancel_ttl_ms: u64 = 30000;
    throttled_drops: u64 = 0;
    early_cancel_hits: u64 = 0;
    total_registered: u64 = 0;
    total_emitted: u64 = 0;

    /// Initializes a new `McpProgressHub` with default throttling interval and early-cancel tombstone TTL.
    static fn init(default_min_interval_ms: u64, early_cancel_ttl_ms: u64): McpProgressHub {
        return McpProgressHub{
            slots: [McpProgressSlot.empty(); MCP_PROGRESS_MAX_SLOTS],
            early_cancels: [McpEarlyCancelEntry.empty(); MCP_PROGRESS_MAX_TOMBSTONES],
            default_min_interval_ms: default_min_interval_ms,
            early_cancel_ttl_ms: early_cancel_ttl_ms,
            throttled_drops: 0,
            early_cancel_hits: 0,
            total_registered: 0,
            total_emitted: 0,
        };
    }

    /// Registers a progress token bound to `request_id`.
    /// - If a `notifications/cancelled` tombstone for `request_id` arrived early (before registration),
    ///   consumes the tombstone and immediately registers the slot in `McpProgressState.cancelled`.
    /// - Returns `null` if `request_id == 0`, `total` is invalid, token/request_id duplicates an
    ///   existing occupied slot, or the slot table is full.
    fn register(
        *_,
        token: McpProgressToken,
        request_id: u64,
        total: f64?,
        min_interval_ms: u64?,
        timeout_ms: u64,
        now_ms: u64,
    ): McpProgressState? {
        if (request_id == 0) return null;
        if (t := total) {
            if (!isFiniteF64(t) || t <= 0.0) return null;
        }
        if (token.kind == McpProgressTokenKind.str_token && (token.str_len == 0 || token.str_len > MCP_PROGRESS_TOKEN_MAX_LEN)) {
            return null;
        }

        let free_idx: usize = MCP_PROGRESS_MAX_SLOTS;
        let i: usize = 0;
        while (i < MCP_PROGRESS_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].occupied) {
                if (_.slots[i].request_id == request_id || _.slots[i].token.equals(token)) {
                    return null;
                }
            } else if (free_idx == MCP_PROGRESS_MAX_SLOTS) {
                free_idx = i;
            }
        }
        if (free_idx == MCP_PROGRESS_MAX_SLOTS) {
            return null;
        }

        let slot = McpProgressSlot.empty();
        slot.token = token;
        slot.request_id = request_id;
        slot.total = total;
        slot.created_ms = now_ms;
        slot.deadline_ms = if (timeout_ms > 0) (now_ms + timeout_ms) else 0;
        slot.min_interval_ms = min_interval_ms ?? _.default_min_interval_ms;
        slot.state = McpProgressState.active;
        slot.occupied = true;

        // Check early-cancel tombstone table for race-safe pre-registration cancellation
        let t_idx: usize = 0;
        while (t_idx < MCP_PROGRESS_MAX_TOMBSTONES) : (t_idx += 1) {
            const tomb = _.early_cancels[t_idx];
            if (!tomb.occupied) continue;
            if (tomb.expires_ms > 0 && now_ms >= tomb.expires_ms) {
                _.early_cancels[t_idx] = McpEarlyCancelEntry.empty();
                continue;
            }
            if (tomb.request_id == request_id) {
                slot.state = McpProgressState.cancelled;
                slot.cancel_reason_len = copyBoundedReason(tomb.reason_buf[0..tomb.reason_len], *slot.cancel_reason_buf);
                _.early_cancels[t_idx] = McpEarlyCancelEntry.empty();
                _.early_cancel_hits += 1;
                break;
            }
        }

        _.slots[free_idx] = slot;
        _.total_registered += 1;
        return slot.state;
    }

    /// Reports a monotonic progress update for `token` at `now_ms`.
    /// Enforces monotonic increase, timeout check, and `min_interval_ms` emission throttling
    /// (allowing final completion `progress >= total` to bypass throttling).
    fn reportProgress(
        *_,
        token: McpProgressToken,
        progress: f64,
        total: f64?,
        message: []const u8,
        now_ms: u64,
    ): McpProgressReportStatus {
        if (!isFiniteF64(progress) || progress < 0.0 || message.len > MCP_PROGRESS_MSG_MAX_LEN) {
            return McpProgressReportStatus.invalid_value;
        }
        if (t := total) {
            if (!isFiniteF64(t) || t <= 0.0) {
                return McpProgressReportStatus.invalid_value;
            }
        }

        let i: usize = 0;
        while (i < MCP_PROGRESS_MAX_SLOTS) : (i += 1) {
            if (!_.slots[i].occupied || !_.slots[i].token.equals(token)) continue;

            if (_.slots[i].state != McpProgressState.active) {
                return McpProgressReportStatus.already_terminal;
            }
            if (_.slots[i].deadline_ms > 0 && now_ms >= _.slots[i].deadline_ms) {
                _.slots[i].state = McpProgressState.timed_out;
                return McpProgressReportStatus.already_terminal;
            }
            if (_.slots[i].has_progress && progress <= _.slots[i].progress) {
                return McpProgressReportStatus.non_monotonic;
            }

            const eff_total: f64? = if (total != null) total else _.slots[i].total;
            let is_final_reach: bool = false;
            if (et := eff_total) {
                if (progress >= et) {
                    is_final_reach = true;
                }
            }

            if (_.slots[i].has_emitted && _.slots[i].min_interval_ms > 0 && !is_final_reach) {
                const elapsed: u64 = if (now_ms > _.slots[i].last_emit_ms) (now_ms - _.slots[i].last_emit_ms) else 0;
                if (elapsed < _.slots[i].min_interval_ms) {
                    _.throttled_drops += 1;
                    return McpProgressReportStatus.throttled;
                }
            }

            _.slots[i].progress = progress;
            _.slots[i].has_progress = true;
            if (total != null) {
                _.slots[i].total = total;
            }
            _.slots[i].msg_len = copyBoundedMsg(message, *_.slots[i].msg_buf);
            _.slots[i].last_emit_ms = now_ms;
            _.slots[i].has_emitted = true;
            _.total_emitted += 1;
            return McpProgressReportStatus.emitted;
        }
        return McpProgressReportStatus.not_found;
    }

    /// Marks the active request `request_id` as completed. Returns `false` if absent or non-active.
    fn complete(*_, request_id: u64): bool {
        if (request_id == 0) return false;
        let i: usize = 0;
        while (i < MCP_PROGRESS_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].request_id == request_id) {
                if (_.slots[i].state != McpProgressState.active) return false;
                _.slots[i].state = McpProgressState.completed;
                return true;
            }
        }
        return false;
    }

    /// Processes a `notifications/cancelled` event for `request_id`.
    /// - Active request -> transitions to `cancelled` and records `reason`.
    /// - Already cancelled -> idempotent `already_cancelled`.
    /// - Already completed/timed_out -> idempotent `already_completed` (never overwrites terminal state).
    /// - Not yet registered -> stores race-safe early-cancel tombstone (`recorded_early_cancel`).
    fn cancelByRequestId(*_, request_id: u64, reason: []const u8, now_ms: u64): McpCancelOutcome {
        if (request_id == 0 || reason.len > MCP_CANCEL_REASON_MAX_LEN) {
            return McpCancelOutcome.invalid_or_full;
        }

        let i: usize = 0;
        while (i < MCP_PROGRESS_MAX_SLOTS) : (i += 1) {
            if (!_.slots[i].occupied || _.slots[i].request_id != request_id) continue;
            if (_.slots[i].state == McpProgressState.active) {
                _.slots[i].state = McpProgressState.cancelled;
                _.slots[i].cancel_reason_len = copyBoundedReason(reason, *_.slots[i].cancel_reason_buf);
                return McpCancelOutcome.cancelled_active;
            }
            if (_.slots[i].state == McpProgressState.cancelled) {
                return McpCancelOutcome.already_cancelled;
            }
            return McpCancelOutcome.already_completed;
        }

        // Not registered yet -> check/insert early-cancel tombstone
        let free_t: usize = MCP_PROGRESS_MAX_TOMBSTONES;
        let t: usize = 0;
        while (t < MCP_PROGRESS_MAX_TOMBSTONES) : (t += 1) {
            if (_.early_cancels[t].occupied) {
                if (_.early_cancels[t].expires_ms > 0 && now_ms >= _.early_cancels[t].expires_ms) {
                    _.early_cancels[t] = McpEarlyCancelEntry.empty();
                    if (free_t == MCP_PROGRESS_MAX_TOMBSTONES) free_t = t;
                    continue;
                }
                if (_.early_cancels[t].request_id == request_id) {
                    return McpCancelOutcome.already_cancelled;
                }
            } else if (free_t == MCP_PROGRESS_MAX_TOMBSTONES) {
                free_t = t;
            }
        }

        if (free_t == MCP_PROGRESS_MAX_TOMBSTONES) {
            return McpCancelOutcome.invalid_or_full;
        }

        let tomb = McpEarlyCancelEntry.empty();
        tomb.request_id = request_id;
        tomb.reason_len = copyBoundedReason(reason, *tomb.reason_buf);
        tomb.cancelled_ms = now_ms;
        tomb.expires_ms = if (_.early_cancel_ttl_ms > 0) (now_ms + _.early_cancel_ttl_ms) else 0;
        tomb.occupied = true;
        _.early_cancels[free_t] = tomb;
        return McpCancelOutcome.recorded_early_cancel;
    }

    /// Returns `true` if `request_id` is currently marked cancelled (either in slot or active tombstone).
    fn isCancelled(request_id: u64): bool {
        if (request_id == 0) return false;
        let i: usize = 0;
        while (i < MCP_PROGRESS_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].request_id == request_id) {
                return _.slots[i].state == McpProgressState.cancelled;
            }
        }
        let t: usize = 0;
        while (t < MCP_PROGRESS_MAX_TOMBSTONES) : (t += 1) {
            if (_.early_cancels[t].occupied && _.early_cancels[t].request_id == request_id) {
                return true;
            }
        }
        return false;
    }

    /// Marks timed-out active slots (`deadline_ms > 0 && now_ms >= deadline_ms`) as `timed_out`
    /// and cleans expired early-cancel tombstones. Returns newly timed-out slot count.
    fn reapExpired(*_, now_ms: u64): usize {
        let expired_slots: usize = 0;
        let i: usize = 0;
        while (i < MCP_PROGRESS_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].state == McpProgressState.active) {
                if (_.slots[i].deadline_ms > 0 && now_ms >= _.slots[i].deadline_ms) {
                    _.slots[i].state = McpProgressState.timed_out;
                    expired_slots += 1;
                }
            }
        }
        let t: usize = 0;
        while (t < MCP_PROGRESS_MAX_TOMBSTONES) : (t += 1) {
            if (_.early_cancels[t].occupied && _.early_cancels[t].expires_ms > 0 && now_ms >= _.early_cancels[t].expires_ms) {
                _.early_cancels[t] = McpEarlyCancelEntry.empty();
            }
        }
        return expired_slots;
    }

    /// Reclaims all terminal (`completed`, `cancelled`, `timed_out`) slots for reuse.
    fn purgeTerminal(*_): usize {
        let purged: usize = 0;
        let i: usize = 0;
        while (i < MCP_PROGRESS_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].state != McpProgressState.active) {
                _.slots[i] = McpProgressSlot.empty();
                purged += 1;
            }
        }
        return purged;
    }

    /// Looks up a progress slot snapshot by `token`.
    fn getByToken(token: McpProgressToken): McpProgressSlot? {
        let i: usize = 0;
        while (i < MCP_PROGRESS_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].token.equals(token)) {
                return _.slots[i];
            }
        }
        return null;
    }

    /// Looks up a progress slot snapshot by `request_id`.
    fn getByRequestId(request_id: u64): McpProgressSlot? {
        if (request_id == 0) return null;
        let i: usize = 0;
        while (i < MCP_PROGRESS_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].request_id == request_id) {
                return _.slots[i];
            }
        }
        return null;
    }

    /// Returns a full telemetry snapshot across all slots and counters.
    fn stats(): McpProgressHubStats {
        let st = McpProgressHubStats{
            throttled_drops: _.throttled_drops,
            early_cancel_hits: _.early_cancel_hits,
            total_registered: _.total_registered,
            total_emitted: _.total_emitted,
        };
        let i: usize = 0;
        while (i < MCP_PROGRESS_MAX_SLOTS) : (i += 1) {
            if (!_.slots[i].occupied) continue;
            if (_.slots[i].state == McpProgressState.active) st.active_count += 1;
            else if (_.slots[i].state == McpProgressState.completed) st.completed_count += 1;
            else if (_.slots[i].state == McpProgressState.cancelled) st.cancelled_count += 1;
            else st.timed_out_count += 1;
        }
        return st;
    }
}
