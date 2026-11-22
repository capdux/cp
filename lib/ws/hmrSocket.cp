// Source: Absorbed from /data/dev/bun/src/bake/DevServer/HmrSocket.cp | Synced: 2026-11-22T17:19:47Z
// Pure-CP zero-allocation Bun DevServer HMR WebSocket protocol codec & session state:
// localhost privilege check (127.0.0.1 / ::1), 17-byte version handshake frame,
// 9-byte hex init frame, topic subscription bitmask diffing, active route tracking,
// bounded per-connection SourceMap reference tracking, and leak-free onClose drain.

pub const HMR_VERSION_FRAME_LEN: usize = 17;
pub const HMR_INIT_FRAME_LEN: usize = 9;
pub const HMR_MAX_REFERENCED_SOURCEMAPS: usize = 32;
pub const HMR_TOPIC_MAX_COUNT: usize = 8;

pub error HmrSocketError {
    EmptyMessage,
    InvalidOpcode,
    InvalidFrameLength,
    InvalidHexEncoding,
    TooManyTopics,
    SourceMapCapacityExceeded,
    UnauthorizedNonLocalhost,
    BufferTooSmall,
}

pub enum HmrIncomingOpcode: u8 {
    Init = 'i',
    Subscribe = 's',
    SetUrl = 'u',
    ConsoleLog = 'c',
    UnrefSourceMap = 'm',

    static fn fromByte(b: u8): HmrIncomingOpcode? {
        return switch (b) {
            'i' => .Init,
            's' => .Subscribe,
            'u' => .SetUrl,
            'c' => .ConsoleLog,
            'm' => .UnrefSourceMap,
            else => null,
        };
    }
}

pub enum HmrOutgoingOpcode: u8 {
    Version = 'V',
    HotUpdate = 'h',
    Errors = 'e',
    Visualizer = 'v',
}

/// Bitmask of HMR WebSocket subscription topics (mirrors Bun `HmrTopic.Bits`).
pub struct HmrTopicBits {
    hot_update: bool = false,
    errors: bool = false,
    incremental_visualizer: bool = false,
    memory_visualizer: bool = false,

    fn toBits(): u8 {
        let b: u8 = 0;
        if (_.hot_update) b |= 1 << 0;
        if (_.errors) b |= 1 << 1;
        if (_.incremental_visualizer) b |= 1 << 2;
        if (_.memory_visualizer) b |= 1 << 3;
        return b;
    }

    static fn fromBits(b: u8): HmrTopicBits {
        return HmrTopicBits{
            hot_update: (b & (1 << 0)) != 0,
            errors: (b & (1 << 1)) != 0,
            incremental_visualizer: (b & (1 << 2)) != 0,
            memory_visualizer: (b & (1 << 3)) != 0,
        };
    }

    fn isEmpty(): bool {
        return _.toBits() == 0;
    }

    /// Parses the topic-character slice (after opcode `'s'`) into `HmrTopicBits`.
    static fn parsePayload(topics: []const u8): HmrTopicBits ! HmrSocketError {
        if (topics.len > HMR_TOPIC_MAX_COUNT) {
            return err.TooManyTopics;
        }
        let bits = HmrTopicBits{};
        let i: usize = 0;
        while (i < topics.len) {
            const c = topics[i];
            if (c == 'h') bits.hot_update = true
            else if (c == 'e') bits.errors = true
            else if (c == 'v') bits.incremental_visualizer = true
            else if (c == 'm') bits.memory_visualizer = true;
            i += 1;
        }
        return bits;
    }

    /// Returns bits set in `_` that were NOT set in `prev`.
    fn addedFrom(prev: HmrTopicBits): HmrTopicBits {
        return HmrTopicBits.fromBits(_.toBits() & (~prev.toBits()));
    }

    /// Returns bits set in `prev` that are NOT set in `_`.
    fn removedFrom(prev: HmrTopicBits): HmrTopicBits {
        return HmrTopicBits.fromBits(prev.toBits() & (~_.toBits()));
    }
}

pub struct HmrTopicDiff {
    added: HmrTopicBits = HmrTopicBits{},
    removed: HmrTopicBits = HmrTopicBits{},
}

pub struct HmrInitPayload {
    generation_id: u32 = 0,
    source_map_key: u64 = 0,
}

pub struct HmrCloseSummary {
    removed_topics: HmrTopicBits = HmrTopicBits{},
    prev_route_id: u32? = null,
    prev_inspector_id: i32 = -1,
    drained_source_maps: usize = 0,
    total_source_maps: usize = 0,
}

/// Checks whether a remote socket IP represents loopback (`127.0.0.1` or `::1`).
/// Mirrors Bun `HmrSocket.new` localhost privilege check.
pub fn isHmrLocalhostIp(ip: []const u8, is_ipv6: bool): bool {
    if (is_ipv6) return ip == "::1";
    return ip == "127.0.0.1";
}

/// Encodes the 17-byte HMR version handshake frame (`['V', 16-byte config_hash]`).
pub fn encodeHmrVersionFrame(config_hash: [16]u8, out: []u8): []const u8 ! HmrSocketError {
    if (out.len < HMR_VERSION_FRAME_LEN) return err.BufferTooSmall;
    out[0] = @intFromEnum(HmrOutgoingOpcode.Version);
    let i: usize = 0;
    while (i < 16) {
        out[1 + i] = config_hash[i];
        i += 1;
    }
    return out[0..HMR_VERSION_FRAME_LEN];
}

/// Decodes and validates a 17-byte HMR version handshake frame.
pub fn decodeHmrVersionFrame(frame: []const u8): [16]u8 ! HmrSocketError {
    if (frame.len == 0) return err.EmptyMessage;
    if (frame[0] != @intFromEnum(HmrOutgoingOpcode.Version)) return err.InvalidOpcode;
    if (frame.len != HMR_VERSION_FRAME_LEN) return err.InvalidFrameLength;
    let hash: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) {
        hash[i] = frame[1 + i];
        i += 1;
    }
    return hash;
}

/// Encodes the 9-byte client init frame (`['i', 8-char lower-hex generation_id]`).
pub fn encodeHmrInitFrame(generation_id: u32, out: []u8): []const u8 ! HmrSocketError {
    if (out.len < HMR_INIT_FRAME_LEN) return err.BufferTooSmall;
    out[0] = @intFromEnum(HmrIncomingOpcode.Init);
    const hex = "0123456789abcdef";
    let shift: u5 = 28;
    let i: usize = 0;
    while (i < 8) {
        const nib: usize = @as(usize, (generation_id >> shift) & 0xF);
        out[1 + i] = hex[nib];
        if (shift >= 4) shift -= 4;
        i += 1;
    }
    return out[0..HMR_INIT_FRAME_LEN];
}

/// Decodes the 9-byte client init frame and derives `source_map_key = generation_id << 32`.
pub fn decodeHmrInitFrame(frame: []const u8): HmrInitPayload ! HmrSocketError {
    if (frame.len == 0) return err.EmptyMessage;
    if (frame[0] != @intFromEnum(HmrIncomingOpcode.Init)) return err.InvalidOpcode;
    if (frame.len != HMR_INIT_FRAME_LEN) return err.InvalidFrameLength;

    let gen: u32 = 0;
    let i: usize = 0;
    while (i < 8) {
        const nib = parseHexNibble(frame[1 + i]) ?? return err.InvalidHexEncoding;
        gen = (gen << 4) | @as(u32, nib);
        i += 1;
    }
    return HmrInitPayload{
        generation_id: gen,
        source_map_key: @as(u64, gen) << 32,
    };
}

/// Zero-allocation per-connection HMR WebSocket session state.
pub struct HmrSocketSession {
    is_from_localhost: bool = false,
    subscriptions: HmrTopicBits = HmrTopicBits{},
    active_route_id: u32? = null,
    inspector_connection_id: i32 = -1,
    referenced_source_maps: [HMR_MAX_REFERENCED_SOURCEMAPS]u64 = [0; HMR_MAX_REFERENCED_SOURCEMAPS],
    ref_sm_count: usize = 0,

    static fn init(remote_ip: []const u8, is_ipv6: bool): HmrSocketSession {
        return HmrSocketSession{
            is_from_localhost: isHmrLocalhostIp(remote_ip, is_ipv6),
        };
    }

    /// Enforces localhost-only access for sensitive DevServer inspection/mutation actions.
    fn requireLocalhost(): bool ! HmrSocketError {
        if (!_.is_from_localhost) return err.UnauthorizedNonLocalhost;
        return true;
    }

    /// Updates topic subscriptions from a `'s'<topics>` frame payload and returns the exact
    /// added/removed bitmask diff (mirrors Bun `HmrSocket.onMessage(.subscribe)`).
    fn applySubscribePayload(*_, topics: []const u8): HmrTopicDiff ! HmrSocketError {
        const next_bits = HmrTopicBits.parsePayload(topics)?;
        const diff = HmrTopicDiff{
            added: next_bits.addedFrom(_.subscriptions),
            removed: next_bits.removedFrom(_.subscriptions),
        };
        _.subscriptions = next_bits;
        return diff;
    }

    /// Switches the session's active route and returns the previous route ID (if any)
    /// so the caller can decrement the old route's `active_viewers` and increment the new one.
    fn setActiveRoute(*_, route_id: u32?): u32? {
        const prev = _.active_route_id;
        _.active_route_id = route_id;
        return prev;
    }

    /// Tracks a strong SourceMap reference held by this client (deduplicated).
    /// Returns `true` if newly added, `false` if already tracked.
    fn trackSourceMap(*_, key: u64): bool ! HmrSocketError {
        let i: usize = 0;
        while (i < _.ref_sm_count) {
            if (_.referenced_source_maps[i] == key) return false;
            i += 1;
        }
        if (_.ref_sm_count >= HMR_MAX_REFERENCED_SOURCEMAPS) {
            return err.SourceMapCapacityExceeded;
        }
        _.referenced_source_maps[_.ref_sm_count] = key;
        _.ref_sm_count += 1;
        return true;
    }

    /// Removes a tracked SourceMap key via O(1) swap-remove. Returns `true` if present.
    fn untrackSourceMap(*_, key: u64): bool {
        let i: usize = 0;
        while (i < _.ref_sm_count) {
            if (_.referenced_source_maps[i] == key) {
                const last = _.ref_sm_count - 1;
                if (i != last) {
                    _.referenced_source_maps[i] = _.referenced_source_maps[last];
                }
                _.ref_sm_count -= 1;
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn referencedSourceMapCount(): usize {
        return _.ref_sm_count;
    }

    /// Mirrors Bun `HmrSocket.onClose`: drains all tracked SourceMap keys into `out_unref_keys`,
    /// clears subscriptions, active route, and inspector ID, returning a complete teardown summary.
    fn closeAndDrain(*_, out_unref_keys: []u64): HmrCloseSummary {
        const total = _.ref_sm_count;
        let written: usize = 0;
        let i: usize = 0;
        while (i < total && written < out_unref_keys.len) {
            out_unref_keys[written] = _.referenced_source_maps[i];
            written += 1;
            i += 1;
        }
        const summary = HmrCloseSummary{
            removed_topics: _.subscriptions,
            prev_route_id: _.active_route_id,
            prev_inspector_id: _.inspector_connection_id,
            drained_source_maps: written,
            total_source_maps: total,
        };
        _.subscriptions = HmrTopicBits{};
        _.active_route_id = null;
        _.inspector_connection_id = -1;
        _.ref_sm_count = 0;
        return summary;
    }
}

fn parseHexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}
