// Origin: Pure-CP Stdlib (Zero-Alloc MCP RFC 5424 Logging Level Router & Notification Ring) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation MCP `logging/setLevel` & `notifications/message` engine:
//   - Full RFC 5424 8-level severity hierarchy (`debug < info < notice < warning < error < critical < alert < emergency`)
//     with bidirectional string codec (`parseMcpLogLevel` / `formatMcpLogLevel`).
//   - Global minimum severity (`setGlobalLevel` / `setGlobalLevelByName`) + longest-prefix logger override
//     rules (`setLoggerPrefixLevel`) with delimiter boundary safety (`.` / `/` / `:`).
//   - Per-logger window rate limiting to prevent log storms (`rate_limited_drops`) with monotonic clock guard.
//   - Fixed 64-entry FIFO notification ring buffer (`emitLog`, `drainNotifications`, `stats`) with newest-wins
//     overwrite accounting (`ring_overwrites`).
//   - Full boundary guards: invalid level names, empty/overlong logger or message, full rule table, clock rollback.

// ============================================================================
// Constants
// ============================================================================

pub const MCP_LOG_MAX_RULES: usize = 32;
pub const MCP_LOG_MAX_RATE_TRACKERS: usize = 32;
pub const MCP_LOG_RING_CAPACITY: usize = 64;
pub const MCP_LOG_LOGGER_MAX_LEN: usize = 64;
pub const MCP_LOG_MSG_MAX_LEN: usize = 256;

// ============================================================================
// Internal Helpers
// ============================================================================

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (asciiLower(a[i]) != asciiLower(b[i])) {
            return false;
        }
    }
    return true;
}

fn eqExact(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn isPrefixDelim(c: u8): bool {
    return c == '.' || c == '/' || c == ':';
}

fn matchesLoggerPrefix(logger: []const u8, prefix: []const u8): bool {
    if (prefix.len == 0 || logger.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (logger[i] != prefix[i]) {
            return false;
        }
    }
    if (logger.len == prefix.len) {
        return true;
    }
    return isPrefixDelim(logger[prefix.len]);
}

fn fnv1a64(bytes: []const u8): u64 {
    let h: u64 = 0xcbf29ce484222325;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        h = (h ^ (bytes[i] as u64)) *% 0x100000001b3;
    }
    return h;
}

// ============================================================================
// 1. RFC 5424 Log Levels & Codec
// ============================================================================

/// MCP RFC 5424 8-level logging severity enum (ordered lowest to highest severity).
pub enum McpLogLevel {
    Debug,
    Info,
    Notice,
    Warning,
    Error,
    Critical,
    Alert,
    Emergency,
}

/// Returns numeric severity rank `0..=7` (`Debug=0` .. `Emergency=7`).
pub fn mcpLogLevelRank(level: McpLogLevel): u8 {
    if (level == McpLogLevel.Debug) return 0;
    if (level == McpLogLevel.Info) return 1;
    if (level == McpLogLevel.Notice) return 2;
    if (level == McpLogLevel.Warning) return 3;
    if (level == McpLogLevel.Error) return 4;
    if (level == McpLogLevel.Critical) return 5;
    if (level == McpLogLevel.Alert) return 6;
    return 7;
}

/// Parses an RFC 5424 MCP log level string (ASCII case-insensitive). Returns `null` if unknown.
pub fn parseMcpLogLevel(name: []const u8): McpLogLevel? {
    if (eqIgnoreCase(name, "debug")) return McpLogLevel.Debug;
    if (eqIgnoreCase(name, "info")) return McpLogLevel.Info;
    if (eqIgnoreCase(name, "notice")) return McpLogLevel.Notice;
    if (eqIgnoreCase(name, "warning")) return McpLogLevel.Warning;
    if (eqIgnoreCase(name, "error")) return McpLogLevel.Error;
    if (eqIgnoreCase(name, "critical")) return McpLogLevel.Critical;
    if (eqIgnoreCase(name, "alert")) return McpLogLevel.Alert;
    if (eqIgnoreCase(name, "emergency")) return McpLogLevel.Emergency;
    return null;
}

/// Serializes an `McpLogLevel` to its canonical lowercase RFC 5424 MCP wire name.
pub fn formatMcpLogLevel(level: McpLogLevel): []const u8 {
    if (level == McpLogLevel.Debug) return "debug";
    if (level == McpLogLevel.Info) return "info";
    if (level == McpLogLevel.Notice) return "notice";
    if (level == McpLogLevel.Warning) return "warning";
    if (level == McpLogLevel.Error) return "error";
    if (level == McpLogLevel.Critical) return "critical";
    if (level == McpLogLevel.Alert) return "alert";
    return "emergency";
}

// ============================================================================
// 2. Data Models
// ============================================================================

/// Outcome status of `McpLogRouter.emitLog`.
pub enum McpLogEmitStatus {
    enqueued,
    filtered_level,
    rate_limited,
    invalid_input,
}

/// Per-logger fixed-window rate-limiting configuration (`max_per_window == 0` disables rate limiting).
pub struct McpLogRateLimitConfig {
    max_per_window: u32 = 0;
    window_ms: u64 = 1_000;

    static fn unlimited(): McpLogRateLimitConfig {
        return McpLogRateLimitConfig{ max_per_window: 0, window_ms: 1_000 };
    }

    fn isValid(): bool {
        if (_.max_per_window > 0 && _.window_ms == 0) {
            return false;
        }
        return true;
    }
}

/// Per-logger prefix level override entry.
pub struct McpLogPrefixRule {
    occupied: bool = false;
    prefix: [64]u8 = [0; 64];
    prefix_len: usize = 0;
    min_level: McpLogLevel = McpLogLevel.Info;
}

/// Per-logger rate-limit window state.
pub struct McpLogRateTracker {
    occupied: bool = false;
    logger_hash: u64 = 0;
    window_start_ms: u64 = 0;
    count_in_window: u32 = 0;
    last_seen_ms: u64 = 0;
}

/// Queued MCP `notifications/message` entry stored inline in the notification ring.
pub struct McpLogNotification {
    seq: u64 = 0;
    timestamp_ms: u64 = 0;
    level: McpLogLevel = McpLogLevel.Info;
    logger: [64]u8 = [0; 64];
    logger_len: usize = 0;
    message: [256]u8 = [0; 256];
    message_len: usize = 0;

    fn loggerSlice(): []const u8 {
        return _.logger[0.._.logger_len];
    }

    fn messageSlice(): []const u8 {
        return _.message[0.._.message_len];
    }
}

/// Aggregate telemetry snapshot for `McpLogRouter`.
pub struct McpLogRouterStats {
    global_level: McpLogLevel = McpLogLevel.Info;
    rule_count: usize = 0;
    queued_count: usize = 0;
    ring_capacity: usize = MCP_LOG_RING_CAPACITY;
    total_emitted: u64 = 0;
    filtered_drops: u64 = 0;
    rate_limited_drops: u64 = 0;
    ring_overwrites: u64 = 0;
    invalid_drops: u64 = 0;
}

// ============================================================================
// 3. MCP Logging Level Router & Notification Ring Engine
// ============================================================================

/// Zero-heap-allocation MCP RFC 5424 logging router, storm limiter, and notification ring buffer.
pub struct McpLogRouter {
    global_level: McpLogLevel = McpLogLevel.Info;
    rate_cfg: McpLogRateLimitConfig = McpLogRateLimitConfig{};
    rules: [32]McpLogPrefixRule = [McpLogPrefixRule{}; 32];
    rule_count: usize = 0;
    trackers: [32]McpLogRateTracker = [McpLogRateTracker{}; 32];
    ring: [64]McpLogNotification = [McpLogNotification{}; 64];
    ring_head: usize = 0;
    ring_tail: usize = 0;
    queued_count: usize = 0;
    next_seq: u64 = 1;
    last_now_ms: u64 = 0;
    total_emitted: u64 = 0;
    filtered_drops: u64 = 0;
    rate_limited_drops: u64 = 0;
    ring_overwrites: u64 = 0;
    invalid_drops: u64 = 0;

    /// Constructs a validated `McpLogRouter`. Returns `null` if `rate_cfg` is invalid.
    static fn init(global_level: McpLogLevel, rate_cfg: McpLogRateLimitConfig): McpLogRouter? {
        if (!rate_cfg.isValid()) {
            return null;
        }
        return McpLogRouter{
            global_level: global_level,
            rate_cfg: rate_cfg,
        };
    }

    /// Updates the global minimum log severity (MCP `logging/setLevel`).
    fn setGlobalLevel(*_, level: McpLogLevel) {
        _.global_level = level;
    }

    /// Parses `level_name` and updates the global minimum severity. Returns `false` if unknown.
    fn setGlobalLevelByName(*_, level_name: []const u8): bool {
        if (lvl := parseMcpLogLevel(level_name)) {
            _.global_level = lvl;
            return true;
        }
        return false;
    }

    /// Registers or updates a logger prefix severity override rule (longest-prefix match wins).
    /// Returns `false` if `prefix` is empty, exceeds `MCP_LOG_LOGGER_MAX_LEN`, or the 32-rule table is full.
    fn setLoggerPrefixLevel(*_, prefix: []const u8, min_level: McpLogLevel): bool {
        if (prefix.len == 0 || prefix.len > MCP_LOG_LOGGER_MAX_LEN) {
            return false;
        }
        let free_idx: usize = MCP_LOG_MAX_RULES;
        let i: usize = 0;
        while (i < MCP_LOG_MAX_RULES) : (i += 1) {
            if (_.rules[i].occupied) {
                if (eqExact(_.rules[i].prefix[0.._.rules[i].prefix_len], prefix)) {
                    _.rules[i].min_level = min_level;
                    return true;
                }
            } else if (free_idx == MCP_LOG_MAX_RULES) {
                free_idx = i;
            }
        }
        if (free_idx >= MCP_LOG_MAX_RULES) {
            return false;
        }
        let rule = McpLogPrefixRule{
            occupied: true,
            prefix_len: prefix.len,
            min_level: min_level,
        };
        let k: usize = 0;
        while (k < prefix.len) : (k += 1) {
            rule.prefix[k] = prefix[k];
        }
        _.rules[free_idx] = rule;
        _.rule_count += 1;
        return true;
    }

    /// Removes a logger prefix override rule. Returns `false` if not found.
    fn removeLoggerPrefixLevel(*_, prefix: []const u8): bool {
        let i: usize = 0;
        while (i < MCP_LOG_MAX_RULES) : (i += 1) {
            if (_.rules[i].occupied && eqExact(_.rules[i].prefix[0.._.rules[i].prefix_len], prefix)) {
                _.rules[i] = McpLogPrefixRule{};
                _.rule_count -= 1;
                return true;
            }
        }
        return false;
    }

    /// Resolves the effective minimum severity for `logger` via longest-prefix match, falling
    /// back to `global_level`.
    fn effectiveLevel(logger: []const u8): McpLogLevel {
        let best_len: usize = 0;
        let best_lvl = _.global_level;
        let i: usize = 0;
        while (i < MCP_LOG_MAX_RULES) : (i += 1) {
            if (_.rules[i].occupied) {
                const plen = _.rules[i].prefix_len;
                if (plen > best_len && matchesLoggerPrefix(logger, _.rules[i].prefix[0..plen])) {
                    best_len = plen;
                    best_lvl = _.rules[i].min_level;
                }
            }
        }
        return best_lvl;
    }

    /// Evaluates level filtering and per-logger rate limiting, and enqueues an accepted
    /// `notifications/message` entry into the 64-slot ring buffer.
    fn emitLog(
        *_,
        level: McpLogLevel,
        logger: []const u8,
        message: []const u8,
        now_ms: u64,
    ): McpLogEmitStatus {
        if (logger.len == 0 || logger.len > MCP_LOG_LOGGER_MAX_LEN ||
            message.len == 0 || message.len > MCP_LOG_MSG_MAX_LEN)
        {
            _.invalid_drops += 1;
            return McpLogEmitStatus.invalid_input;
        }

        const effective_now = _.clampNow(now_ms);

        // 1. Severity filter
        const min_lvl = _.effectiveLevel(logger);
        if (mcpLogLevelRank(level) < mcpLogLevelRank(min_lvl)) {
            _.filtered_drops += 1;
            return McpLogEmitStatus.filtered_level;
        }

        // 2. Per-logger window rate limiting
        if (_.rate_cfg.max_per_window > 0) {
            if (!_.admitRateLimit(logger, effective_now)) {
                _.rate_limited_drops += 1;
                return McpLogEmitStatus.rate_limited;
            }
        }

        // 3. Enqueue into notification ring (newest overwrites oldest if full)
        if (_.queued_count >= MCP_LOG_RING_CAPACITY) {
            _.ring_head = (_.ring_head + 1) % MCP_LOG_RING_CAPACITY;
            _.queued_count -= 1;
            _.ring_overwrites += 1;
        }

        let notif = McpLogNotification{
            seq: _.next_seq,
            timestamp_ms: effective_now,
            level: level,
            logger_len: logger.len,
            message_len: message.len,
        };
        let i: usize = 0;
        while (i < logger.len) : (i += 1) {
            notif.logger[i] = logger[i];
        }
        i = 0;
        while (i < message.len) : (i += 1) {
            notif.message[i] = message[i];
        }

        _.ring[_.ring_tail] = notif;
        _.ring_tail = (_.ring_tail + 1) % MCP_LOG_RING_CAPACITY;
        _.queued_count += 1;
        _.next_seq += 1;
        _.total_emitted += 1;
        return McpLogEmitStatus.enqueued;
    }

    /// Drains up to `min(queued_count, out_notifs.len)` queued notifications in FIFO order.
    fn drainNotifications(*_, out_notifs: *[]McpLogNotification): usize {
        let drained: usize = 0;
        while (drained < out_notifs.len && _.queued_count > 0) : (drained += 1) {
            out_notifs[drained] = _.ring[_.ring_head];
            _.ring_head = (_.ring_head + 1) % MCP_LOG_RING_CAPACITY;
            _.queued_count -= 1;
        }
        return drained;
    }

    fn pendingCount(): usize {
        return _.queued_count;
    }

    fn stats(): McpLogRouterStats {
        return McpLogRouterStats{
            global_level: _.global_level,
            rule_count: _.rule_count,
            queued_count: _.queued_count,
            ring_capacity: MCP_LOG_RING_CAPACITY,
            total_emitted: _.total_emitted,
            filtered_drops: _.filtered_drops,
            rate_limited_drops: _.rate_limited_drops,
            ring_overwrites: _.ring_overwrites,
            invalid_drops: _.invalid_drops,
        };
    }

    // ========================================================================
    // Internal Helpers
    // ========================================================================

    fn clampNow(*_, now_ms: u64): u64 {
        if (now_ms >= _.last_now_ms) {
            _.last_now_ms = now_ms;
            return now_ms;
        }
        return _.last_now_ms;
    }

    fn admitRateLimit(*_, logger: []const u8, effective_now: u64): bool {
        const lhash = fnv1a64(logger);
        let match_idx: usize = MCP_LOG_MAX_RATE_TRACKERS;
        let free_idx: usize = MCP_LOG_MAX_RATE_TRACKERS;
        let lru_idx: usize = 0;
        let lru_ts: u64 = 0xFFFFFFFFFFFFFFFF;

        let i: usize = 0;
        while (i < MCP_LOG_MAX_RATE_TRACKERS) : (i += 1) {
            if (_.trackers[i].occupied) {
                if (_.trackers[i].logger_hash == lhash) {
                    match_idx = i;
                    break;
                }
                if (_.trackers[i].last_seen_ms < lru_ts) {
                    lru_ts = _.trackers[i].last_seen_ms;
                    lru_idx = i;
                }
            } else if (free_idx == MCP_LOG_MAX_RATE_TRACKERS) {
                free_idx = i;
            }
        }

        if (match_idx < MCP_LOG_MAX_RATE_TRACKERS) {
            const elapsed = effective_now - _.trackers[match_idx].window_start_ms;
            if (elapsed >= _.rate_cfg.window_ms) {
                _.trackers[match_idx].window_start_ms = effective_now;
                _.trackers[match_idx].count_in_window = 1;
                _.trackers[match_idx].last_seen_ms = effective_now;
                return true;
            }
            if (_.trackers[match_idx].count_in_window >= _.rate_cfg.max_per_window) {
                _.trackers[match_idx].last_seen_ms = effective_now;
                return false;
            }
            _.trackers[match_idx].count_in_window += 1;
            _.trackers[match_idx].last_seen_ms = effective_now;
            return true;
        }

        const dst = if (free_idx < MCP_LOG_MAX_RATE_TRACKERS) free_idx else lru_idx;
        _.trackers[dst] = McpLogRateTracker{
            occupied: true,
            logger_hash: lhash,
            window_start_ms: effective_now,
            count_in_window: 1,
            last_seen_ms: effective_now,
        };
        return true;
    }
}
