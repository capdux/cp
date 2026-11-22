// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc MCP Capability Negotiator & Resource Subscription Router) | Created: 2026-11-22
// Pure CP zero-heap-allocation Model Context Protocol (MCP) protocol version & capability bitmask
// negotiator, strict session lifecycle state machine, and fixed-slot wildcard resource subscription
// broadcast registry:
//   - Bitmask capability constants covering MCP 2024-11-05 / 2025-03-26 / 2025-06-18 (incl. elicitation)
//   - Lossless bidirectional `McpNegotiatedVersion` parsing, formatting & ordering
//   - Strict 5-phase `McpSessionNegotiator` (`uninitialized -> initializing -> ready -> closing -> closed`)
//   - Fixed 64-slot inline-buffer `McpResourceSubscriptionRegistry` supporting exact & trailing-`*` prefix matching

const std = @import("std");

// ============================================================================
// 1. MCP Capability Bitmask & Subscription Capacity Constants
// ============================================================================

/// Server/client supports MCP Tools invocation (`tools/list`, `tools/call`).
pub const MCP_CAP_TOOLS: u32 = 1 << 0;
/// Server emits `notifications/tools/list_changed` when registered tools change.
pub const MCP_CAP_TOOLS_LIST_CHANGED: u32 = 1 << 1;
/// Server exposes MCP Resources (`resources/list`, `resources/read`).
pub const MCP_CAP_RESOURCES: u32 = 1 << 2;
/// Server supports MCP Resource subscriptions (`resources/subscribe`, `notifications/resources/updated`).
pub const MCP_CAP_RESOURCES_SUBSCRIBE: u32 = 1 << 3;
/// Server exposes MCP Prompt templates (`prompts/list`, `prompts/get`).
pub const MCP_CAP_PROMPTS: u32 = 1 << 4;
/// Server supports structured log level control & log notifications (`logging/setLevel`).
pub const MCP_CAP_LOGGING: u32 = 1 << 5;
/// Client supports reverse LLM sampling (`sampling/createMessage`).
pub const MCP_CAP_SAMPLING: u32 = 1 << 6;
/// Client exposes filesystem/workspace roots (`roots/list`).
pub const MCP_CAP_ROOTS: u32 = 1 << 7;
/// Client supports interactive user elicitation introduced in MCP 2025-06-18 (`elicitation/create`).
pub const MCP_CAP_ELICITATION: u32 = 1 << 8;
/// Mask of all known MCP capability bits.
pub const MCP_CAP_ALL_KNOWN: u32 = 0x1FF;

/// Fixed slot capacity for `McpResourceSubscriptionRegistry` (zero dynamic heap allocations).
pub const MCP_SUB_MAX_SLOTS: usize = 64;
/// Maximum inline URI / wildcard pattern length per subscription slot.
pub const MCP_SUB_MAX_URI_LEN: usize = 128;

// ============================================================================
// 2. Protocol Version & Session Lifecycle Phase Enums
// ============================================================================

/// Negotiated MCP protocol specification version supporting `2024-11-05`, `2025-03-26`, and `2025-06-18`.
pub enum McpNegotiatedVersion {
    unknown = 0,
    v2024_11_05 = 1,
    v2025_03_26 = 2,
    v2025_06_18 = 3,

    /// Parses a canonical MCP version date string into `McpNegotiatedVersion`.
    static fn parse(s: []const u8): McpNegotiatedVersion {
        if (std.mem.eql(u8, s, "2024-11-05")) {
            return McpNegotiatedVersion.v2024_11_05;
        }
        if (std.mem.eql(u8, s, "2025-03-26")) {
            return McpNegotiatedVersion.v2025_03_26;
        }
        if (std.mem.eql(u8, s, "2025-06-18")) {
            return McpNegotiatedVersion.v2025_06_18;
        }
        return McpNegotiatedVersion.unknown;
    }

    /// Formats `McpNegotiatedVersion` back into its canonical protocol date string.
    fn toString(): []const u8 {
        return switch (_) {
            McpNegotiatedVersion.v2024_11_05 => "2024-11-05",
            McpNegotiatedVersion.v2025_03_26 => "2025-03-26",
            McpNegotiatedVersion.v2025_06_18 => "2025-06-18",
            McpNegotiatedVersion.unknown => "unknown",
        };
    }

    /// Returns monotonic version ordinal (`0` for `unknown`, `1..3` for known specs).
    fn rank(): u8 {
        return switch (_) {
            McpNegotiatedVersion.v2024_11_05 => 1,
            McpNegotiatedVersion.v2025_03_26 => 2,
            McpNegotiatedVersion.v2025_06_18 => 3,
            McpNegotiatedVersion.unknown => 0,
        };
    }

    /// Returns `true` if this version is a recognized MCP specification.
    fn isKnown(): bool {
        return _.rank() > 0;
    }

    /// Compares two protocol versions (`-1` if older, `0` if equal, `1` if newer).
    fn compare(other: McpNegotiatedVersion): i8 {
        const a = _.rank();
        const b = other.rank();
        if (a < b) return -1;
        if (a > b) return 1;
        return 0;
    }

    /// Returns `true` when both versions are known and `this >= other`.
    fn isAtLeast(other: McpNegotiatedVersion): bool {
        const a = _.rank();
        const b = other.rank();
        return a > 0 && b > 0 && a >= b;
    }

    /// Returns `true` when both versions are known and `this <= other`.
    fn isAtMost(other: McpNegotiatedVersion): bool {
        const a = _.rank();
        const b = other.rank();
        return a > 0 && b > 0 && a <= b;
    }
}

/// Strict 5-phase lifecycle state of an MCP session.
pub enum McpSessionPhase {
    uninitialized,
    initializing,
    ready,
    closing,
    closed,
}

// ============================================================================
// 3. Handshake Models & Session Capability Negotiator
// ============================================================================

/// Client handshake payload submitted during MCP `initialize`.
pub struct McpHandshakeRequest {
    client_version: []const u8 = "";
    client_caps: u32 = 0;
}

/// Outcome of an MCP protocol version and capability bitmask handshake.
pub struct McpHandshakeResult {
    accepted: bool = false;
    negotiated_version: McpNegotiatedVersion = McpNegotiatedVersion.unknown;
    effective_caps: u32 = 0;
    missing_required_caps: u32 = 0;
}

/// Zero-alloc MCP session protocol negotiator and lifecycle gatekeeper.
pub struct McpSessionNegotiator {
    server_min_ver: McpNegotiatedVersion = McpNegotiatedVersion.v2024_11_05;
    server_max_ver: McpNegotiatedVersion = McpNegotiatedVersion.v2025_06_18;
    server_caps: u32 = 0;
    required_client_caps: u32 = 0;
    phase: McpSessionPhase = McpSessionPhase.uninitialized;
    negotiated_version: McpNegotiatedVersion = McpNegotiatedVersion.unknown;
    effective_caps: u32 = 0;

    /// Initializes a session negotiator in `uninitialized` phase with server version bounds
    /// and capability bitmasks (unknown high bits are automatically masked off).
    static fn init(
        server_min_ver: McpNegotiatedVersion,
        server_max_ver: McpNegotiatedVersion,
        server_caps: u32,
        required_client_caps: u32,
    ): McpSessionNegotiator {
        return McpSessionNegotiator{
            server_min_ver: server_min_ver,
            server_max_ver: server_max_ver,
            server_caps: server_caps & MCP_CAP_ALL_KNOWN,
            required_client_caps: required_client_caps & MCP_CAP_ALL_KNOWN,
            phase: McpSessionPhase.uninitialized,
            negotiated_version: McpNegotiatedVersion.unknown,
            effective_caps: 0,
        };
    }

    /// Negotiates protocol version and capability intersection against a client `initialize` request.
    ///
    /// Rules:
    /// - Only permitted in `uninitialized` phase.
    /// - Rejects unknown versions, invalid server version ranges, or client versions older than `server_min_ver`.
    /// - Downgrades client versions newer than `server_max_ver` to `server_max_ver`.
    /// - Rejects if any bit in `required_client_caps` is absent from `client_caps` (reported via `missing_required_caps`).
    /// - On acceptance, computes `effective_caps = server_caps & client_caps` and advances `uninitialized -> initializing`.
    fn negotiate(
        *_,
        client_ver_str: []const u8,
        client_caps: u32,
    ): McpHandshakeResult {
        if (_.phase != McpSessionPhase.uninitialized) {
            return McpHandshakeResult{
                accepted: false,
                negotiated_version: _.negotiated_version,
                effective_caps: 0,
                missing_required_caps: 0,
            };
        }

        const min_rank = _.server_min_ver.rank();
        const max_rank = _.server_max_ver.rank();
        if (min_rank == 0 || max_rank == 0 || min_rank > max_rank) {
            return McpHandshakeResult{};
        }

        const client_ver = McpNegotiatedVersion.parse(client_ver_str);
        const client_rank = client_ver.rank();
        if (client_rank == 0 || client_rank < min_rank) {
            return McpHandshakeResult{
                accepted: false,
                negotiated_version: McpNegotiatedVersion.unknown,
                effective_caps: 0,
                missing_required_caps: 0,
            };
        }

        const chosen_ver = if (client_rank > max_rank) _.server_max_ver else client_ver;
        const masked_client = client_caps & MCP_CAP_ALL_KNOWN;
        const missing = _.required_client_caps & (~masked_client);
        const effective = _.server_caps & masked_client;

        if (missing != 0) {
            return McpHandshakeResult{
                accepted: false,
                negotiated_version: chosen_ver,
                effective_caps: effective,
                missing_required_caps: missing,
            };
        }

        _.phase = McpSessionPhase.initializing;
        _.negotiated_version = chosen_ver;
        _.effective_caps = effective;

        return McpHandshakeResult{
            accepted: true,
            negotiated_version: chosen_ver,
            effective_caps: effective,
            missing_required_caps: 0,
        };
    }

    /// Convenience wrapper accepting a structured `McpHandshakeRequest`.
    fn negotiate_request(*_, req: McpHandshakeRequest): McpHandshakeResult {
        return _.negotiate(req.client_version, req.client_caps);
    }

    /// Completes the handshake upon receiving `notifications/initialized`.
    /// Only valid when `phase == initializing`; transitions to `ready`.
    fn mark_initialized(*_): bool {
        if (_.phase != McpSessionPhase.initializing) {
            return false;
        }
        _.phase = McpSessionPhase.ready;
        return true;
    }

    /// Returns `true` iff the session is in `ready` phase and every bit in `required_cap_bit`
    /// was mutually negotiated during handshake.
    fn can_invoke(required_cap_bit: u32): bool {
        if (_.phase != McpSessionPhase.ready) {
            return false;
        }
        return (_.effective_caps & required_cap_bit) == required_cap_bit;
    }

    /// Initiates graceful session shutdown (`uninitialized | initializing | ready -> closing`).
    fn begin_close(*_): bool {
        if (_.phase == McpSessionPhase.closing || _.phase == McpSessionPhase.closed) {
            return false;
        }
        _.phase = McpSessionPhase.closing;
        return true;
    }

    /// Finalizes session shutdown (`closing -> closed`) and revokes active negotiated capabilities.
    fn finish_close(*_): bool {
        if (_.phase != McpSessionPhase.closing) {
            return false;
        }
        _.phase = McpSessionPhase.closed;
        _.effective_caps = 0;
        return true;
    }
}

// ============================================================================
// 4. Fixed-Slot Zero-Alloc Resource Subscription Registry
// ============================================================================

/// Inline storage slot for a single client resource subscription rule.
pub struct McpSubscriptionSlot {
    active: bool = false;
    client_id: u64 = 0;
    uri_buf: [MCP_SUB_MAX_URI_LEN]u8 = [0; MCP_SUB_MAX_URI_LEN];
    uri_len: usize = 0;
    is_wildcard: bool = false;
}

/// Zero-heap-allocation MCP resource subscription router with 64 inline URI slots.
/// Supports exact URI subscriptions and trailing-`*` prefix wildcards (e.g. `file:///workspace/*`).
pub struct McpResourceSubscriptionRegistry {
    slots: [MCP_SUB_MAX_SLOTS]McpSubscriptionSlot = [McpSubscriptionSlot{}; MCP_SUB_MAX_SLOTS];
    count: usize = 0;

    /// Initializes an empty 64-slot subscription registry.
    static fn init(): McpResourceSubscriptionRegistry {
        return McpResourceSubscriptionRegistry{
            slots: [McpSubscriptionSlot{}; MCP_SUB_MAX_SLOTS],
            count: 0,
        };
    }

    /// Subscribes `client_id` to `uri_pattern` (exact URI or trailing-`*` prefix wildcard).
    ///
    /// Guards:
    /// - Rejects `client_id == 0`, empty pattern, pattern exceeding `MCP_SUB_MAX_URI_LEN` (128 bytes),
    ///   or malformed mid-string `*` wildcards.
    /// - Deduplicates identical `(client_id, uri_pattern)` subscriptions idempotently.
    /// - Returns `false` when all 64 slots are occupied.
    fn subscribe(*_, client_id: u64, uri_pattern: []const u8): bool {
        if (client_id == 0 || uri_pattern.len == 0 || uri_pattern.len > MCP_SUB_MAX_URI_LEN) {
            return false;
        }

        // Validate wildcard placement: '*' is only allowed as the final character
        let k: usize = 0;
        while (k + 1 < uri_pattern.len) : (k += 1) {
            if (uri_pattern[k] == 42) {
                return false;
            }
        }
        const is_wild = uri_pattern[uri_pattern.len - 1] == 42;

        let free_idx: usize? = null;
        let i: usize = 0;
        while (i < MCP_SUB_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].active) {
                if (_.slots[i].client_id == client_id && _.slots[i].uri_len == uri_pattern.len) {
                    if (std.mem.eql(u8, _.slots[i].uri_buf[0 .. _.slots[i].uri_len], uri_pattern)) {
                        return true; // idempotent dedup
                    }
                }
            } else if (free_idx == null) {
                free_idx = i;
            }
        }

        const slot_idx = free_idx ?? return false;
        _.slots[slot_idx].active = true;
        _.slots[slot_idx].client_id = client_id;
        _.slots[slot_idx].uri_len = uri_pattern.len;
        _.slots[slot_idx].is_wildcard = is_wild;
        let j: usize = 0;
        while (j < uri_pattern.len) : (j += 1) {
            _.slots[slot_idx].uri_buf[j] = uri_pattern[j];
        }
        _.count += 1;
        return true;
    }

    /// Removes a specific `(client_id, uri_pattern)` subscription and frees its slot for reuse.
    fn unsubscribe(*_, client_id: u64, uri_pattern: []const u8): bool {
        if (client_id == 0 || uri_pattern.len == 0 || uri_pattern.len > MCP_SUB_MAX_URI_LEN) {
            return false;
        }

        let i: usize = 0;
        while (i < MCP_SUB_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].active && _.slots[i].client_id == client_id && _.slots[i].uri_len == uri_pattern.len) {
                if (std.mem.eql(u8, _.slots[i].uri_buf[0 .. _.slots[i].uri_len], uri_pattern)) {
                    _.slots[i].active = false;
                    _.slots[i].client_id = 0;
                    _.slots[i].uri_len = 0;
                    _.slots[i].is_wildcard = false;
                    _.count -= 1;
                    return true;
                }
            }
        }
        return false;
    }

    /// Removes all subscriptions owned by `client_id` (e.g. on disconnect).
    /// Returns the number of reclaimed slots.
    fn unsubscribe_all(*_, client_id: u64): usize {
        if (client_id == 0 || _.count == 0) {
            return 0;
        }
        let removed: usize = 0;
        let i: usize = 0;
        while (i < MCP_SUB_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].active && _.slots[i].client_id == client_id) {
                _.slots[i].active = false;
                _.slots[i].client_id = 0;
                _.slots[i].uri_len = 0;
                _.slots[i].is_wildcard = false;
                removed += 1;
            }
        }
        _.count -= removed;
        return removed;
    }

    /// Matches `changed_uri` against all active exact and trailing-`*` prefix subscriptions,
    /// writing deduplicated `client_id`s into `out_client_ids` with zero heap allocations.
    /// Returns the number of unique subscriber IDs written (`<= out_client_ids.len`).
    fn match_subscribers(changed_uri: []const u8, out_client_ids: *[]u64): usize {
        if (changed_uri.len == 0 || out_client_ids.len == 0 || _.count == 0) {
            return 0;
        }

        let written: usize = 0;
        let i: usize = 0;
        while (i < MCP_SUB_MAX_SLOTS) : (i += 1) {
            if (!_.slots[i].active) {
                continue;
            }
            const uri_len = _.slots[i].uri_len;
            let matched = false;
            if (_.slots[i].is_wildcard) {
                const prefix_len = uri_len - 1;
                if (changed_uri.len >= prefix_len) {
                    if (prefix_len == 0 || std.mem.eql(u8, changed_uri[0..prefix_len], _.slots[i].uri_buf[0..prefix_len])) {
                        matched = true;
                    }
                }
            } else {
                if (changed_uri.len == uri_len && std.mem.eql(u8, changed_uri, _.slots[i].uri_buf[0..uri_len])) {
                    matched = true;
                }
            }

            if (!matched) {
                continue;
            }

            // Deduplicate client_id across multiple matching patterns
            const cid = _.slots[i].client_id;
            let dup = false;
            let k: usize = 0;
            while (k < written) : (k += 1) {
                if (out_client_ids[k] == cid) {
                    dup = true;
                    break;
                }
            }
            if (dup) {
                continue;
            }

            if (written < out_client_ids.len) {
                out_client_ids[written] = cid;
                written += 1;
            } else {
                break;
            }
        }

        return written;
    }

    /// Returns the number of currently active subscription slots.
    fn active_count(): usize {
        return _.count;
    }
}
