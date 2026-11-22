// Origin: Pure-CP Stdlib (Zero-Alloc Hierarchical Request Deadlines & gRPC Timeout Propagation) | Created: 2026-11-23
// Pure CP zero-heap-allocation Hierarchical Request Deadlines & RPC Timeout Propagation Engine:
//   - Wire-level `grpc-timeout` codec (`parseGrpcTimeoutHeader` / `formatGrpcTimeoutHeader`):
//       * Supports `n` / `u` / `m` / `S` / `M` / `H` units with strict 8-digit gRPC cap & overflow guards
//       * Automatically selects optimal compact wire unit when formatting remaining microseconds
//   - `RequestDeadline` value object with monotonic child derivation (`deriveChild`), remaining budget
//     inspection, stage admission (`canFitStage`), and retry/hedge reserve slicing (`reserveForRetry`)
//   - Fixed 64-node `DeadlineTree` with monotonic parent->child truncation, iterative zero-recursion
//     cascade cancellation (`parent_cancelled`), expired node sweeping & slot reclamation, and stats

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Fixed capacity of the zero-heap hierarchical `DeadlineTree`.
pub const DEADLINE_MAX_NODES: usize = 64;

/// Sentinel invalid node identifier for `DeadlineTree`.
pub const DEADLINE_NO_NODE: u16 = 0xFFFF;

/// Maximum length in bytes of a node diagnostic tag.
pub const DEADLINE_TAG_MAX_LEN: usize = 32;

/// Maximum number of decimal digits allowed by the gRPC `grpc-timeout` wire specification.
pub const GRPC_TIMEOUT_MAX_DIGITS: usize = 8;

const I64_MAX: i64 = 9223372036854775807;
const GRPC_MAX_VAL: i64 = 99_999_999;

/// Lifecycle / evaluation status of a request deadline.
pub enum DeadlineStatus {
    active,
    expired,
    cancelled,
    parent_cancelled,
    exhausted_reserve,
}

/// Wire timeout unit supported by `grpc-timeout`.
pub enum DeadlineUnit {
    ns,
    us,
    ms,
    sec,
    min,
    hour,
}

/// Immutable-style monotonic request deadline value object (absolute microsecond epoch).
pub struct RequestDeadline {
    deadline_us: i64 = 0;

    /// Construct a `RequestDeadline` from current time `now_us` plus `timeout_us` (saturating).
    /// Non-positive `timeout_us` yields an immediately expired deadline at `now_us`.
    static fn fromTimeoutUs(now_us: i64, timeout_us: i64): RequestDeadline {
        if (timeout_us <= 0) {
            return RequestDeadline{ deadline_us: now_us };
        }
        if (now_us > 0 && timeout_us > I64_MAX - now_us) {
            return RequestDeadline{ deadline_us: I64_MAX };
        }
        return RequestDeadline{ deadline_us: now_us + timeout_us };
    }

    /// Derive a child deadline clamped strictly `<=` parent deadline (monotonic non-increasing).
    fn deriveChild(local_timeout_us: i64, now_us: i64): RequestDeadline {
        const local = RequestDeadline.fromTimeoutUs(now_us, local_timeout_us);
        const eff: i64 = if (local.deadline_us < deadline_us) local.deadline_us else deadline_us;
        return RequestDeadline{ deadline_us: eff };
    }

    /// Return remaining budget in microseconds at `now_us` (`0` if expired).
    fn remainingUs(now_us: i64): i64 {
        if (now_us >= deadline_us) return 0;
        return deadline_us - now_us;
    }

    /// Return true if the deadline has been reached or passed at `now_us`.
    fn isExpired(now_us: i64): bool {
        return now_us >= deadline_us;
    }

    /// Return true if at least `min_stage_us` remains at `now_us` to execute a downstream stage.
    fn canFitStage(now_us: i64, min_stage_us: i64): bool {
        if (min_stage_us <= 0) return !isExpired(now_us);
        return remainingUs(now_us) >= min_stage_us;
    }

    /// Slice a single-attempt deadline leaving `reserve_us` safety margin for retries or hedging.
    /// Returns `null` if remaining budget is `<= reserve_us` (exhausted reserve).
    fn reserveForRetry(now_us: i64, reserve_us: i64): RequestDeadline? {
        if (reserve_us < 0) return null;
        const rem = remainingUs(now_us);
        if (rem <= reserve_us) return null;
        return RequestDeadline{ deadline_us: deadline_us - reserve_us };
    }
}

/// Single node inside `DeadlineTree`.
pub struct DeadlineNode {
    active: bool = false;
    id: u16 = DEADLINE_NO_NODE;
    parent_id: u16 = DEADLINE_NO_NODE;
    deadline: RequestDeadline = RequestDeadline{};
    status: DeadlineStatus = DeadlineStatus.active;
    tag_buf: [DEADLINE_TAG_MAX_LEN]u8 = [0; DEADLINE_TAG_MAX_LEN];
    tag_len: usize = 0;

    /// Return the node's diagnostic tag slice.
    fn tag(): []const u8 {
        return tag_buf[0..tag_len];
    }
}

/// Zero-heap global telemetry snapshot for `DeadlineTree`.
pub struct DeadlineTreeStats {
    capacity: usize = DEADLINE_MAX_NODES;
    active_slots: usize = 0;
    running_count: usize = 0;
    cancelled_count: usize = 0;
    expired_swept_total: u64 = 0;
    created_total: u64 = 0;
    cascade_cancelled_total: u64 = 0;
}

// ============================================================================
// 2. gRPC `grpc-timeout` Wire Codec & Formatting Helpers
// ============================================================================

/// Format a `DeadlineStatus` variant as a canonical ASCII label.
pub fn formatDeadlineStatus(status: DeadlineStatus): []const u8 {
    if (status == DeadlineStatus.active) return "active";
    if (status == DeadlineStatus.expired) return "expired";
    if (status == DeadlineStatus.cancelled) return "cancelled";
    if (status == DeadlineStatus.parent_cancelled) return "parent_cancelled";
    return "exhausted_reserve";
}

/// Format a `DeadlineUnit` variant as its gRPC wire suffix.
pub fn formatDeadlineUnit(unit: DeadlineUnit): []const u8 {
    if (unit == DeadlineUnit.ns) return "n";
    if (unit == DeadlineUnit.us) return "u";
    if (unit == DeadlineUnit.ms) return "m";
    if (unit == DeadlineUnit.sec) return "S";
    if (unit == DeadlineUnit.min) return "M";
    return "H";
}

/// Parse a gRPC `grpc-timeout` wire header (`<1..8 digits><n|u|m|S|M|H>`) into microseconds.
/// Rejects empty strings, negative signs, >8 digits, whitespace, unknown suffixes, or overflow.
pub fn parseGrpcTimeoutHeader(header: []const u8): i64? {
    if (header.len < 2 || header.len > GRPC_TIMEOUT_MAX_DIGITS + 1) return null;

    const unit_ch = header[header.len - 1];
    const digits = header[0 .. header.len - 1];

    let val: u64 = 0;
    let i: usize = 0;
    while (i < digits.len) : (i += 1) {
        const c = digits[i];
        if (c < '0' || c > '9') return null;
        val = val * 10 + @as(u64, c - '0');
    }

    if (unit_ch == 'n') {
        if (val == 0) return 0;
        const us = val / 1000;
        return if (us == 0) 1 else @as(i64, @intCast(us));
    }
    if (unit_ch == 'u') {
        return @as(i64, @intCast(val));
    }
    if (unit_ch == 'm') {
        const res = val * 1_000;
        return @as(i64, @intCast(res));
    }
    if (unit_ch == 'S') {
        const res = val * 1_000_000;
        return @as(i64, @intCast(res));
    }
    if (unit_ch == 'M') {
        const res = val * 60_000_000;
        return @as(i64, @intCast(res));
    }
    if (unit_ch == 'H') {
        const res = val * 3_600_000_000;
        return @as(i64, @intCast(res));
    }
    return null;
}

fn writeGrpcValUnit(val: i64, unit_ch: u8, out_buf: *[]u8): usize? {
    if (val < 0 || val > GRPC_MAX_VAL) return null;
    let tmp: [16]u8 = [0; 16];
    let n: usize = 0;
    let v: u64 = @as(u64, @intCast(val));
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) : (n += 1) {
            tmp[n] = @as(u8, @intCast('0' + (v % 10)));
            v = v / 10;
        }
    }
    if (n + 1 > out_buf.len) return null;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out_buf[i] = tmp[n - 1 - i];
    }
    out_buf[n] = unit_ch;
    return n + 1;
}

/// Format `remaining_us` into a compact gRPC `grpc-timeout` wire string (respecting the
/// 8-digit gRPC limit) in `out_buf`. Non-positive values format as `"0u"`.
/// Returns bytes written, or `null` if `out_buf` is too small or exceeds max gRPC hours.
pub fn formatGrpcTimeoutHeader(remaining_us: i64, out_buf: *[]u8): usize? {
    if (remaining_us <= 0) {
        return writeGrpcValUnit(0, 'u', out_buf);
    }

    // Prefer exact compact units when divisible and within 8-digit cap.
    if (remaining_us % 3_600_000_000 == 0) {
        const hrs = remaining_us / 3_600_000_000;
        if (hrs <= GRPC_MAX_VAL) return writeGrpcValUnit(hrs, 'H', out_buf);
    }
    if (remaining_us % 60_000_000 == 0) {
        const mins = remaining_us / 60_000_000;
        if (mins <= GRPC_MAX_VAL) return writeGrpcValUnit(mins, 'M', out_buf);
    }
    if (remaining_us % 1_000_000 == 0) {
        const secs = remaining_us / 1_000_000;
        if (secs <= GRPC_MAX_VAL) return writeGrpcValUnit(secs, 'S', out_buf);
    }
    if (remaining_us % 1_000 == 0) {
        const ms = remaining_us / 1_000;
        if (ms <= GRPC_MAX_VAL) return writeGrpcValUnit(ms, 'm', out_buf);
    }

    // Fallback: pick finest unit that fits within 8 decimal digits (<= 99_999_999).
    if (remaining_us <= GRPC_MAX_VAL) {
        return writeGrpcValUnit(remaining_us, 'u', out_buf);
    }
    const ms_trunc = remaining_us / 1_000;
    if (ms_trunc <= GRPC_MAX_VAL) {
        return writeGrpcValUnit(ms_trunc, 'm', out_buf);
    }
    const sec_trunc = remaining_us / 1_000_000;
    if (sec_trunc <= GRPC_MAX_VAL) {
        return writeGrpcValUnit(sec_trunc, 'S', out_buf);
    }
    const min_trunc = remaining_us / 60_000_000;
    if (min_trunc <= GRPC_MAX_VAL) {
        return writeGrpcValUnit(min_trunc, 'M', out_buf);
    }
    const hr_trunc = remaining_us / 3_600_000_000;
    if (hr_trunc <= GRPC_MAX_VAL) {
        return writeGrpcValUnit(hr_trunc, 'H', out_buf);
    }
    return null;
}

// ============================================================================
// 3. DeadlineTree — Fixed-Capacity Hierarchical Deadline & Cascade Cancel Tree
// ============================================================================

/// Zero-heap-allocation hierarchical request deadline & cascade cancellation tree.
pub struct DeadlineTree {
    nodes: [DEADLINE_MAX_NODES]DeadlineNode = [DeadlineNode{}; DEADLINE_MAX_NODES];
    expired_swept_total: u64 = 0;
    created_total: u64 = 0;
    cascade_cancelled_total: u64 = 0;

    /// Initialize an empty `DeadlineTree`.
    static fn init(): DeadlineTree {
        return DeadlineTree{};
    }

    fn allocSlot(): u16? {
        let i: usize = 0;
        while (i < DEADLINE_MAX_NODES) : (i += 1) {
            if (!nodes[i].active) {
                return @as(u16, @intCast(i));
            }
        }
        return null;
    }

    fn copyTag(node: *DeadlineNode, tag: []const u8) {
        const n = if (tag.len > DEADLINE_TAG_MAX_LEN) DEADLINE_TAG_MAX_LEN else tag.len;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            node.tag_buf[i] = tag[i];
        }
        node.tag_len = n;
    }

    /// Create a new root deadline node. Returns node ID `0..63`, or `null` if tree is full.
    fn createRoot(*_, now_us: i64, timeout_us: i64, tag: []const u8): u16? {
        const slot = allocSlot() ?? return null;
        let n = DeadlineNode{};
        n.active = true;
        n.id = slot;
        n.parent_id = DEADLINE_NO_NODE;
        n.deadline = RequestDeadline.fromTimeoutUs(now_us, timeout_us);
        n.status = DeadlineStatus.active;
        copyTag(*n, tag);

        nodes[@as(usize, slot)] = n;
        created_total += 1;
        return slot;
    }

    /// Create a child deadline derived from `parent_id` with monotonic truncation (`<= parent`).
    /// Returns `null` if `parent_id` is invalid, already cancelled, already expired, or tree is full.
    fn createChild(*_, parent_id: u16, now_us: i64, local_timeout_us: i64, tag: []const u8): u16? {
        if (@as(usize, parent_id) >= DEADLINE_MAX_NODES) return null;
        const p_idx = @as(usize, parent_id);
        if (!nodes[p_idx].active) return null;
        if (nodes[p_idx].status == DeadlineStatus.cancelled || nodes[p_idx].status == DeadlineStatus.parent_cancelled) {
            return null;
        }
        if (nodes[p_idx].deadline.isExpired(now_us)) return null;

        const slot = allocSlot() ?? return null;
        let n = DeadlineNode{};
        n.active = true;
        n.id = slot;
        n.parent_id = parent_id;
        n.deadline = nodes[p_idx].deadline.deriveChild(local_timeout_us, now_us);
        n.status = DeadlineStatus.active;
        copyTag(*n, tag);

        nodes[@as(usize, slot)] = n;
        created_total += 1;
        return slot;
    }

    /// Retrieve a copy of a node by ID.
    fn getNode(id: u16): DeadlineNode? {
        if (@as(usize, id) >= DEADLINE_MAX_NODES) return null;
        const idx = @as(usize, id);
        if (!nodes[idx].active) return null;
        return nodes[idx];
    }

    /// Evaluate the live status of node `id` at `now_us`.
    fn statusAt(id: u16, now_us: i64): DeadlineStatus? {
        if (@as(usize, id) >= DEADLINE_MAX_NODES) return null;
        const idx = @as(usize, id);
        if (!nodes[idx].active) return null;
        if (nodes[idx].status == DeadlineStatus.cancelled || nodes[idx].status == DeadlineStatus.parent_cancelled) {
            return nodes[idx].status;
        }
        if (nodes[idx].deadline.isExpired(now_us)) {
            return DeadlineStatus.expired;
        }
        return DeadlineStatus.active;
    }

    /// Return remaining microseconds for node `id` at `now_us` (`0` if cancelled or expired).
    fn remainingUs(id: u16, now_us: i64): i64? {
        if (@as(usize, id) >= DEADLINE_MAX_NODES) return null;
        const idx = @as(usize, id);
        if (!nodes[idx].active) return null;
        if (nodes[idx].status == DeadlineStatus.cancelled || nodes[idx].status == DeadlineStatus.parent_cancelled) {
            return 0;
        }
        return nodes[idx].deadline.remainingUs(now_us);
    }

    /// Cancel node `id` (`cancelled`) and iteratively cascade `parent_cancelled` to all
    /// active descendants without recursion. Returns total number of nodes transitioned.
    fn cancel(*_, id: u16): usize {
        if (@as(usize, id) >= DEADLINE_MAX_NODES) return 0;
        const root_idx = @as(usize, id);
        if (!nodes[root_idx].active) return 0;

        let changed: usize = 0;
        if (nodes[root_idx].status == DeadlineStatus.active) {
            nodes[root_idx].status = DeadlineStatus.cancelled;
            changed += 1;
        }

        let queue: [DEADLINE_MAX_NODES]u16 = [DEADLINE_NO_NODE; DEADLINE_MAX_NODES];
        let head: usize = 0;
        let tail: usize = 0;
        queue[tail] = id;
        tail += 1;

        while (head < tail) {
            const cur_id = queue[head];
            head += 1;
            let i: usize = 0;
            while (i < DEADLINE_MAX_NODES) : (i += 1) {
                if (!nodes[i].active) continue;
                if (nodes[i].parent_id != cur_id) continue;
                if (nodes[i].status == DeadlineStatus.active) {
                    nodes[i].status = DeadlineStatus.parent_cancelled;
                    changed += 1;
                    cascade_cancelled_total += 1;
                }
                if (tail < DEADLINE_MAX_NODES) {
                    queue[tail] = nodes[i].id;
                    tail += 1;
                }
            }
        }
        return changed;
    }

    /// Release a node slot back to the free pool (e.g., when a request completes normally).
    fn release(*_, id: u16): bool {
        if (@as(usize, id) >= DEADLINE_MAX_NODES) return false;
        const idx = @as(usize, id);
        if (!nodes[idx].active) return false;
        nodes[idx] = DeadlineNode{};
        return true;
    }

    /// Sweep all expired active slots at `now_us`, reclaiming their slots for reuse.
    /// Returns the number of expired nodes swept.
    fn sweepExpired(*_, now_us: i64): usize {
        let swept: usize = 0;
        let i: usize = 0;
        while (i < DEADLINE_MAX_NODES) : (i += 1) {
            if (nodes[i].active && nodes[i].deadline.isExpired(now_us)) {
                nodes[i] = DeadlineNode{};
                swept += 1;
            }
        }
        expired_swept_total += @as(u64, @intCast(swept));
        return swept;
    }

    /// Return a zero-heap global telemetry snapshot.
    fn stats(): DeadlineTreeStats {
        let st = DeadlineTreeStats{};
        st.capacity = DEADLINE_MAX_NODES;
        st.expired_swept_total = expired_swept_total;
        st.created_total = created_total;
        st.cascade_cancelled_total = cascade_cancelled_total;

        let i: usize = 0;
        while (i < DEADLINE_MAX_NODES) : (i += 1) {
            if (!nodes[i].active) continue;
            st.active_slots += 1;
            if (nodes[i].status == DeadlineStatus.active) {
                st.running_count += 1;
            } else if (nodes[i].status == DeadlineStatus.cancelled || nodes[i].status == DeadlineStatus.parent_cancelled) {
                st.cancelled_count += 1;
            }
        }
        return st;
    }
}
