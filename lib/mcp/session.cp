// Origin: Pure-CP Stdlib (MCP Session Lifecycle, Capability Negotiation & Request Tracker) | Created: 2026-11-22
// Zero-heap-allocation Model Context Protocol (MCP) session lifecycle state machine,
// protocol version & capability bitmask negotiation, and fixed-slot JSON-RPC 2.0 inflight request tracker.

const std = @import("std");

/// Supported Model Context Protocol (MCP) specification versions.
pub enum McpProtocolVersion {
    V2024_11_05,
    V2025_03_26,
    Unknown,
}

fn versionRank(ver: McpProtocolVersion): u8 {
    return switch (ver) {
        McpProtocolVersion.V2024_11_05 => 1,
        McpProtocolVersion.V2025_03_26 => 2,
        McpProtocolVersion.Unknown => 0,
    };
}

/// Parses an MCP protocol version string (e.g. `"2025-03-26"`) into `McpProtocolVersion`.
pub fn parseMcpVersion(raw: string): McpProtocolVersion {
    if (std.mem.eql(u8, raw, "2024-11-05")) {
        return McpProtocolVersion.V2024_11_05;
    }
    if (std.mem.eql(u8, raw, "2025-03-26")) {
        return McpProtocolVersion.V2025_03_26;
    }
    return McpProtocolVersion.Unknown;
}

/// Formats `McpProtocolVersion` into its canonical protocol date string.
pub fn formatMcpVersion(ver: McpProtocolVersion): string {
    return switch (ver) {
        McpProtocolVersion.V2024_11_05 => "2024-11-05",
        McpProtocolVersion.V2025_03_26 => "2025-03-26",
        McpProtocolVersion.Unknown => "unknown",
    };
}

/// Negotiates the highest mutually supported MCP protocol version.
/// Returns `null` if either side reports `Unknown`.
pub fn negotiateMcpVersion(
    client_ver: McpProtocolVersion,
    server_max_ver: McpProtocolVersion,
): McpProtocolVersion? {
    const c_rank = versionRank(client_ver);
    const s_rank = versionRank(server_max_ver);
    if (c_rank == 0 || s_rank == 0) {
        return null;
    }
    if (c_rank <= s_rank) {
        return client_ver;
    }
    return server_max_ver;
}

/// Bitmask wrapper for MCP client/server capability flags.
pub struct McpCapabilityFlags {
    bits: u32 = 0;

    pub static const NONE: u32 = 0;
    pub static const TOOLS: u32 = 1 << 0;
    pub static const RESOURCES: u32 = 1 << 1;
    pub static const PROMPTS: u32 = 1 << 2;
    pub static const LOGGING: u32 = 1 << 3;
    pub static const SAMPLING: u32 = 1 << 4;
    pub static const ROOTS: u32 = 1 << 5;
    pub static const SUBSCRIPTIONS: u32 = 1 << 6;
    pub static const LIST_CHANGED: u32 = 1 << 7;
    pub static const ALL_KNOWN: u32 = 0xFF;

    /// Constructs capability flags from a raw bitmask, masking out unknown high bits.
    static fn fromBits(raw: u32): McpCapabilityFlags {
        return McpCapabilityFlags{
            bits: raw & McpCapabilityFlags.ALL_KNOWN,
        };
    }

    /// Returns `true` if all bits in `mask` are set.
    fn has(mask: u32): bool {
        return (_.bits & mask) == mask;
    }

    /// Returns `true` if at least one bit in `mask` is set.
    fn hasAny(mask: u32): bool {
        return (_.bits & mask) != 0;
    }

    /// Returns a new flag set with `mask` bits enabled.
    fn with(mask: u32): McpCapabilityFlags {
        return McpCapabilityFlags{
            bits: (_.bits | mask) & McpCapabilityFlags.ALL_KNOWN,
        };
    }

    /// Returns a new flag set with `mask` bits cleared.
    fn without(mask: u32): McpCapabilityFlags {
        return McpCapabilityFlags{
            bits: _.bits & (~mask) & McpCapabilityFlags.ALL_KNOWN,
        };
    }

    /// Returns `true` when no capability bits are enabled.
    fn isEmpty(): bool {
        return _.bits == 0;
    }
}

/// Computes the negotiated capability intersection between client and server.
pub fn negotiateCapabilities(
    client_caps: McpCapabilityFlags,
    server_caps: McpCapabilityFlags,
): McpCapabilityFlags {
    return McpCapabilityFlags.fromBits(client_caps.bits & server_caps.bits);
}

/// Strict lifecycle state of an MCP session.
pub enum McpSessionState {
    Disconnected,
    Initializing,
    Ready,
    Closing,
    Closed,
}

/// Deterministic error codes for MCP session lifecycle and request tracking operations.
pub enum McpSessionOpError {
    InvalidState,
    IncompatibleVersion,
    DuplicateRequestId,
    InflightPoolFull,
    RequestNotFound,
    InvalidRequestId,
    CapabilityUnsupported,
}

/// Fixed-slot tracking record for a single inflight JSON-RPC 2.0 request.
pub struct McpInflightRequest {
    active: bool = false;
    request_id: u64 = 0;
    method_hash: u64 = 0;
    sent_ms: i64 = 0;
    timeout_ms: i64 = 0;
    cancelled: bool = false;
}

/// Cumulative operational telemetry for an MCP session.
pub struct McpSessionStats {
    total_started: u64 = 0;
    total_completed: u64 = 0;
    total_cancelled: u64 = 0;
    total_timed_out: u64 = 0;
    total_rejected: u64 = 0;
    peak_inflight: usize = 0;
}

/// Computes a deterministic 64-bit FNV-1a hash for an MCP JSON-RPC method name.
pub fn hashMethodName(method: string): u64 {
    let h: u64 = (u64(0xcbf29ce4) << 32) | u64(0x84222325);
    let i: usize = 0;
    while (i < method.len) : (i += 1) {
        h = h ^ u64(method[i]);
        h = h *% 0x100000001b3;
    }
    return h;
}

/// Zero-heap-allocation MCP session lifecycle state machine and inflight request tracker.
pub struct McpSession {
    state: McpSessionState = McpSessionState.Disconnected;
    protocol_version: McpProtocolVersion = McpProtocolVersion.Unknown;
    client_caps: McpCapabilityFlags = McpCapabilityFlags{};
    server_caps: McpCapabilityFlags = McpCapabilityFlags{};
    negotiated_caps: McpCapabilityFlags = McpCapabilityFlags{};
    slots: []McpInflightRequest;
    active_count: usize = 0;
    stats: McpSessionStats = McpSessionStats{};

    /// Initializes a new disconnected session backed by caller-owned `slots` buffer.
    static fn init(slots: *[]McpInflightRequest): McpSession {
        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i] = McpInflightRequest{};
        }
        return McpSession{
            state: McpSessionState.Disconnected,
            protocol_version: McpProtocolVersion.Unknown,
            client_caps: McpCapabilityFlags{},
            server_caps: McpCapabilityFlags{},
            negotiated_caps: McpCapabilityFlags{},
            slots: slots,
            active_count: 0,
            stats: McpSessionStats{},
        };
    }

    /// Returns the current lifecycle state.
    fn getState(): McpSessionState {
        return _.state;
    }

    /// Returns `true` only when the session has completed handshake and is `Ready`.
    fn isReady(): bool {
        return _.state == McpSessionState.Ready;
    }

    /// Returns the number of currently tracked inflight requests (including cancelled pending drain).
    fn inflightCount(): usize {
        return _.active_count;
    }

    /// Returns the maximum number of concurrent inflight requests supported by the slot pool.
    fn capacity(): usize {
        return _.slots.len;
    }

    /// Returns a snapshot copy of cumulative session statistics.
    fn getStats(): McpSessionStats {
        return _.stats;
    }

    /// Returns the negotiated MCP protocol version (`Unknown` prior to initialization).
    fn getNegotiatedVersion(): McpProtocolVersion {
        return _.protocol_version;
    }

    /// Returns the negotiated capability intersection bitmask.
    fn getNegotiatedCapabilities(): McpCapabilityFlags {
        return _.negotiated_caps;
    }

    /// Transitions `Disconnected -> Initializing` after validating protocol version compatibility
    /// and computing the client/server capability intersection.
    /// Returns `null` on success, or an `McpSessionOpError` on failure.
    fn beginInitialize(
        *_,
        client_ver: McpProtocolVersion,
        server_max_ver: McpProtocolVersion,
        client_caps: McpCapabilityFlags,
        server_caps: McpCapabilityFlags,
    ): McpSessionOpError? {
        if (_.state != McpSessionState.Disconnected) {
            _.stats.total_rejected += 1;
            return McpSessionOpError.InvalidState;
        }
        if (ver := negotiateMcpVersion(client_ver, server_max_ver)) {
            _.protocol_version = ver;
            _.client_caps = McpCapabilityFlags.fromBits(client_caps.bits);
            _.server_caps = McpCapabilityFlags.fromBits(server_caps.bits);
            _.negotiated_caps = negotiateCapabilities(_.client_caps, _.server_caps);
            _.state = McpSessionState.Initializing;
            return null;
        }
        _.stats.total_rejected += 1;
        return McpSessionOpError.IncompatibleVersion;
    }

    /// Transitions `Initializing -> Ready` upon receiving `notifications/initialized`.
    /// Returns `null` on success, or `InvalidState` if not currently `Initializing`.
    fn markReady(*_): McpSessionOpError? {
        if (_.state != McpSessionState.Initializing) {
            _.stats.total_rejected += 1;
            return McpSessionOpError.InvalidState;
        }
        _.state = McpSessionState.Ready;
        return null;
    }

    /// Transitions `Initializing` or `Ready` -> `Closing` to begin graceful drain.
    fn beginClose(*_): McpSessionOpError? {
        if (_.state != McpSessionState.Ready && _.state != McpSessionState.Initializing) {
            _.stats.total_rejected += 1;
            return McpSessionOpError.InvalidState;
        }
        _.state = McpSessionState.Closing;
        return null;
    }

    /// Transitions `Closing -> Closed` and clears any remaining inflight slots.
    fn finishClose(*_): McpSessionOpError? {
        if (_.state != McpSessionState.Closing) {
            _.stats.total_rejected += 1;
            return McpSessionOpError.InvalidState;
        }
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].active) {
                if (!_.slots[i].cancelled) {
                    _.stats.total_cancelled += 1;
                }
                _.slots[i] = McpInflightRequest{};
            }
        }
        _.active_count = 0;
        _.state = McpSessionState.Closed;
        return null;
    }

    /// Immediately transitions any non-`Closed` state to `Closed`, cancelling all active inflight requests.
    /// Returns the number of newly cancelled inflight requests.
    fn forceClose(*_): usize {
        let newly_cancelled: usize = 0;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].active) {
                if (!_.slots[i].cancelled) {
                    _.stats.total_cancelled += 1;
                    newly_cancelled += 1;
                }
                _.slots[i] = McpInflightRequest{};
            }
        }
        _.active_count = 0;
        _.state = McpSessionState.Closed;
        return newly_cancelled;
    }

    /// Resets a `Closed` or `Disconnected` session back to a clean `Disconnected` state.
    /// Returns `InvalidState` if the session is currently active (`Initializing`/`Ready`/`Closing`).
    fn reset(*_): McpSessionOpError? {
        if (_.state != McpSessionState.Closed && _.state != McpSessionState.Disconnected) {
            _.stats.total_rejected += 1;
            return McpSessionOpError.InvalidState;
        }
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            _.slots[i] = McpInflightRequest{};
        }
        _.active_count = 0;
        _.state = McpSessionState.Disconnected;
        _.protocol_version = McpProtocolVersion.Unknown;
        _.client_caps = McpCapabilityFlags{};
        _.server_caps = McpCapabilityFlags{};
        _.negotiated_caps = McpCapabilityFlags{};
        _.stats = McpSessionStats{};
        return null;
    }

    /// Evaluates whether a JSON-RPC `method` is permitted in the current lifecycle state
    /// and under the negotiated capability flags. Returns `null` if allowed.
    fn checkMethodAllowed(method: string): McpSessionOpError? {
        if (method.len == 0) {
            return McpSessionOpError.InvalidState;
        }
        if (_.state == McpSessionState.Disconnected) {
            if (std.mem.eql(u8, method, "initialize")) {
                return null;
            }
            return McpSessionOpError.InvalidState;
        }
        if (_.state == McpSessionState.Initializing) {
            if (std.mem.eql(u8, method, "notifications/initialized") || std.mem.eql(u8, method, "ping")) {
                return null;
            }
            return McpSessionOpError.InvalidState;
        }
        if (_.state != McpSessionState.Ready) {
            return McpSessionOpError.InvalidState;
        }

        // Session is Ready: enforce negotiated capability gating for standard MCP namespaces.
        if (std.mem.eql(u8, method, "ping")) {
            return null;
        }
        if (std.mem.startsWith(u8, method, "tools/")) {
            if (!_.negotiated_caps.has(McpCapabilityFlags.TOOLS)) {
                return McpSessionOpError.CapabilityUnsupported;
            }
            return null;
        }
        if (std.mem.eql(u8, method, "resources/subscribe") || std.mem.eql(u8, method, "resources/unsubscribe")) {
            const req_mask = McpCapabilityFlags.RESOURCES | McpCapabilityFlags.SUBSCRIPTIONS;
            if (!_.negotiated_caps.has(req_mask)) {
                return McpSessionOpError.CapabilityUnsupported;
            }
            return null;
        }
        if (std.mem.startsWith(u8, method, "resources/")) {
            if (!_.negotiated_caps.has(McpCapabilityFlags.RESOURCES)) {
                return McpSessionOpError.CapabilityUnsupported;
            }
            return null;
        }
        if (std.mem.startsWith(u8, method, "prompts/")) {
            if (!_.negotiated_caps.has(McpCapabilityFlags.PROMPTS)) {
                return McpSessionOpError.CapabilityUnsupported;
            }
            return null;
        }
        if (std.mem.startsWith(u8, method, "logging/")) {
            if (!_.negotiated_caps.has(McpCapabilityFlags.LOGGING)) {
                return McpSessionOpError.CapabilityUnsupported;
            }
            return null;
        }
        if (std.mem.startsWith(u8, method, "sampling/")) {
            if (!_.negotiated_caps.has(McpCapabilityFlags.SAMPLING)) {
                return McpSessionOpError.CapabilityUnsupported;
            }
            return null;
        }
        if (std.mem.startsWith(u8, method, "roots/")) {
            if (!_.negotiated_caps.has(McpCapabilityFlags.ROOTS)) {
                return McpSessionOpError.CapabilityUnsupported;
            }
            return null;
        }
        return null;
    }

    /// Convenience boolean wrapper around `checkMethodAllowed`.
    fn canDispatchMethod(method: string): bool {
        return _.checkMethodAllowed(method) == null;
    }

    /// Finds the backing slot index of an active inflight request by `request_id`.
    fn findInflightIndex(request_id: u64): usize? {
        if (request_id == 0) return null;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].active && _.slots[i].request_id == request_id) {
                return i;
            }
        }
        return null;
    }

    /// Returns a copy of an active inflight request by `request_id`, or `null` if absent.
    fn getInflight(request_id: u64): McpInflightRequest? {
        if (idx := _.findInflightIndex(request_id)) {
            return _.slots[idx];
        }
        return null;
    }

    /// Registers a new inflight JSON-RPC request in the fixed slot pool.
    /// Requires `Ready` state, non-zero `request_id`, unique active `request_id`, and a free slot.
    /// `timeout_ms <= 0` disables timeout expiration for this request; negative `sent_ms` is clamped to `0`.
    fn beginRequest(
        *_,
        request_id: u64,
        method_hash: u64,
        sent_ms: i64,
        timeout_ms: i64,
    ): McpSessionOpError? {
        if (_.state != McpSessionState.Ready) {
            _.stats.total_rejected += 1;
            return McpSessionOpError.InvalidState;
        }
        if (request_id == 0) {
            _.stats.total_rejected += 1;
            return McpSessionOpError.InvalidRequestId;
        }
        if (_.findInflightIndex(request_id) != null) {
            _.stats.total_rejected += 1;
            return McpSessionOpError.DuplicateRequestId;
        }

        let free_idx: usize? = null;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].active) {
                free_idx = i;
                break;
            }
        }
        if (free_idx == null) {
            _.stats.total_rejected += 1;
            return McpSessionOpError.InflightPoolFull;
        }

        const idx = free_idx ?? 0;
        const safe_sent: i64 = if (sent_ms < 0) 0 else sent_ms;
        const safe_timeout: i64 = if (timeout_ms < 0) 0 else timeout_ms;
        _.slots[idx] = McpInflightRequest{
            active: true,
            request_id: request_id,
            method_hash: method_hash,
            sent_ms: safe_sent,
            timeout_ms: safe_timeout,
            cancelled: false,
        };
        _.active_count += 1;
        _.stats.total_started += 1;
        if (_.active_count > _.stats.peak_inflight) {
            _.stats.peak_inflight = _.active_count;
        }
        return null;
    }

    /// Validates lifecycle & capability gating for `method`, then registers the inflight request
    /// using `hashMethodName(method)`.
    fn beginMethodRequest(
        *_,
        request_id: u64,
        method: string,
        sent_ms: i64,
        timeout_ms: i64,
    ): McpSessionOpError? {
        if (err := _.checkMethodAllowed(method)) {
            _.stats.total_rejected += 1;
            return err;
        }
        return _.beginRequest(request_id, hashMethodName(method), sent_ms, timeout_ms);
    }

    /// Completes and releases an inflight request by `request_id`.
    /// Returns the removed record (allowing callers to inspect `.cancelled` for late responses),
    /// or `null` if not found. Only non-cancelled completions increment `stats.total_completed`.
    fn completeRequest(*_, request_id: u64): McpInflightRequest? {
        if (idx := _.findInflightIndex(request_id)) {
            const rec = _.slots[idx];
            _.slots[idx] = McpInflightRequest{};
            if (_.active_count > 0) {
                _.active_count -= 1;
            }
            if (!rec.cancelled) {
                _.stats.total_completed += 1;
            }
            return rec;
        }
        return null;
    }

    /// Marks an active inflight request as cancelled (corresponding to `notifications/cancelled`).
    /// The slot remains reserved until `completeRequest`, `purgeCancelled`, or `collectTimedOut`
    /// drains it, preventing premature ID reuse. Returns `false` if absent or already cancelled.
    fn cancelRequest(*_, request_id: u64): bool {
        if (idx := _.findInflightIndex(request_id)) {
            if (_.slots[idx].cancelled) {
                return false;
            }
            _.slots[idx].cancelled = true;
            _.stats.total_cancelled += 1;
            return true;
        }
        return false;
    }

    /// Drains all inflight slots marked `cancelled`, writing up to `out_ids.len` drained IDs
    /// into `out_ids` and returning the total number of drained slots.
    fn purgeCancelled(*_, out_ids: *[]u64): usize {
        let purged: usize = 0;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].active && _.slots[i].cancelled) {
                if (purged < out_ids.len) {
                    out_ids[purged] = _.slots[i].request_id;
                }
                _.slots[i] = McpInflightRequest{};
                if (_.active_count > 0) {
                    _.active_count -= 1;
                }
                purged += 1;
            }
        }
        return purged;
    }

    /// Sweeps all active inflight requests whose deadline (`sent_ms + timeout_ms <= now_ms`)
    /// has expired, freeing their slots and writing up to `out_ids.len` timed-out IDs into `out_ids`.
    /// Returns the total number of expired requests reclaimed.
    /// Requests with `timeout_ms == 0` never expire; backward clock skew (`now_ms < sent_ms`) never expires.
    fn collectTimedOut(*_, now_ms: i64, out_ids: *[]u64): usize {
        const safe_now: i64 = if (now_ms < 0) 0 else now_ms;
        let reclaimed: usize = 0;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].active) continue;
            const req = _.slots[i];
            if (req.timeout_ms <= 0) continue;
            if (safe_now < req.sent_ms) continue;
            const elapsed = safe_now - req.sent_ms;
            if (elapsed >= req.timeout_ms) {
                if (reclaimed < out_ids.len) {
                    out_ids[reclaimed] = req.request_id;
                }
                if (!req.cancelled) {
                    _.stats.total_timed_out += 1;
                }
                _.slots[i] = McpInflightRequest{};
                if (_.active_count > 0) {
                    _.active_count -= 1;
                }
                reclaimed += 1;
            }
        }
        return reclaimed;
    }
}
