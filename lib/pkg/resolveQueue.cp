// Source: Absorbed from /data/dev/bun/src/install/PackageManager/PackageManagerEnqueue.cp | Synced: 2026-11-22T12:17:37Z
// ============================================================================
// lib/pkg/resolveQueue.cp — Zero-Alloc Dependency Resolve Queue & SemVer Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven dependency resolution engine for
// `cap install` / `cap add`: SemVer 2.0 comparator & range evaluator (`^`/`~`/
// `>=`/`<=`/`>`/`<`/AND/OR/prerelease isolation), deduplicated priority work
// queue with concurrency throttling & exponential backoff retry
// (`PkgResolveQueue`), best-version selector with dedup reuse
// (`selectBestMatchingVersion`), diamond-dependency conflict reconciler
// (`reconcileResolvedGraph`), and zero-alloc queue/reconcile report formatter.

/// Default maximum concurrent in-flight resolution tasks.
pub const RESOLVE_QUEUE_DEFAULT_MAX_IN_FLIGHT: u32 = 16;

/// Default maximum retry attempts for transient resolution failures.
pub const RESOLVE_QUEUE_DEFAULT_MAX_RETRIES: u8 = 3;

/// Base backoff duration in milliseconds for exponential retry scheduling.
pub const RESOLVE_QUEUE_BASE_BACKOFF_MS: u64 = 100;

/// Sentinel invalid parent task identifier.
pub const RESOLVE_QUEUE_NO_PARENT: u32 = 4294967295;

/// Dependency resolution queue, SemVer, and reconciliation error set.
pub err ResolveQueueError {
    InvalidSemver,
    InvalidRange,
    InvalidTaskId,
    InvalidStateTransition,
    NoMatchingVersion,
    MaxInFlightReached,
    SliceCapacityExceeded,
    BufferTooSmall,
}

/// Resolution scheduling priority (lower numeric value = higher priority).
pub enum RqDepPriority: u8 {
    RootDirect = 0,
    Peer = 1,
    Transitive = 2,
    Optional = 3,
}

/// Lifecycle state of a single dependency resolution task.
pub enum RqTaskStatus: u8 {
    Queued = 0,
    InFlight = 1,
    Resolved = 2,
    Failed = 3,
    SkippedOptional = 4,
}

/// Placement decision produced by diamond-dependency reconciliation.
pub enum RqPlacementKind: u8 {
    HoistedShared = 0,
    NestedIsolated = 1,
    UnresolvedConflict = 2,
}

/// Parsed SemVer 2.0.0 version structure (zero-copy prerelease slice).
pub struct RqSemver {
    major: u32 = 0,
    minor: u32 = 0,
    patch: u32 = 0,
    prerelease: []const u8 = "",
    has_prerelease: bool = false,

    /// Parses a SemVer string such as `"1.2.3"`, `"v0.4.1-beta.2+build"`.
    static fn parse(raw_in: []const u8): RqSemver! {
        let s = trimWs(raw_in);
        if (s.len == 0) return ResolveQueueError.InvalidSemver;
        if (s[0] == 'v' || s[0] == 'V') {
            s = s[1..];
        }
        if (s.len == 0) return ResolveQueueError.InvalidSemver;

        // Strip +build metadata
        if (plus := indexOfByte(s, '+')) {
            s = s[0..plus];
        }

        // Split -prerelease
        let pre: []const u8 = "";
        let has_pre = false;
        if (dash := indexOfByte(s, '-')) {
            pre = s[dash + 1 ..];
            if (pre.len == 0) return ResolveQueueError.InvalidSemver;
            has_pre = true;
            s = s[0..dash];
        }

        const dot1_opt = indexOfByte(s, '.');
        if (dot1_opt == null) return ResolveQueueError.InvalidSemver;
        const dot1 = dot1_opt.?;
        const rest1 = s[dot1 + 1 ..];
        const dot2_opt = indexOfByte(rest1, '.');
        if (dot2_opt == null) return ResolveQueueError.InvalidSemver;
        const dot2 = dot2_opt.?;
        const patch_str = rest1[dot2 + 1 ..];
        if (indexOfByte(patch_str, '.') != null) return ResolveQueueError.InvalidSemver;

        const maj = parseU32Dec(s[0..dot1]) ?? return ResolveQueueError.InvalidSemver;
        const min = parseU32Dec(rest1[0..dot2]) ?? return ResolveQueueError.InvalidSemver;
        const pat = parseU32Dec(patch_str) ?? return ResolveQueueError.InvalidSemver;

        return RqSemver{
            major: maj,
            minor: min,
            patch: pat,
            prerelease: pre,
            has_prerelease: has_pre,
        };
    }

    /// SemVer 2.0.0 total ordering (`-1` if `a < b`, `0` if equal, `1` if `a > b`).
    static fn compare(a: RqSemver, b: RqSemver): i32 {
        if (a.major != b.major) return if (a.major < b.major) -1 else 1;
        if (a.minor != b.minor) return if (a.minor < b.minor) -1 else 1;
        if (a.patch != b.patch) return if (a.patch < b.patch) -1 else 1;

        if (!a.has_prerelease && !b.has_prerelease) return 0;
        if (!a.has_prerelease && b.has_prerelease) return 1;
        if (a.has_prerelease && !b.has_prerelease) return -1;

        return comparePrerelease(a.prerelease, b.prerelease);
    }

    fn sameCore(other: RqSemver): bool {
        return _.major == other.major && _.minor == other.minor && _.patch == other.patch;
    }
}

/// Single task entry in `PkgResolveQueue` backed by caller-owned slice.
pub struct RqTaskEntry {
    task_id: u32 = 0,
    name: []const u8 = "",
    spec: []const u8 = "",
    name_hash: u64 = 0,
    spec_hash: u64 = 0,
    priority: RqDepPriority = RqDepPriority.Transitive,
    status: RqTaskStatus = RqTaskStatus.Queued,
    attempts: u8 = 0,
    next_retry_after_ms: u64 = 0,
    resolved_version: []const u8 = "",
    parent_task_id: u32 = RESOLVE_QUEUE_NO_PARENT,

    static fn empty(): RqTaskEntry {
        return RqTaskEntry{};
    }
}

/// Live telemetry counters for `PkgResolveQueue`.
pub struct RqQueueStats {
    total_enqueued: u32 = 0,
    deduplicated_hits: u32 = 0,
    queued: u32 = 0,
    in_flight: u32 = 0,
    resolved: u32 = 0,
    failed: u32 = 0,
    skipped_optional: u32 = 0,
    retries_scheduled: u32 = 0,
}

/// Registry version candidate for `selectBestMatchingVersion`.
pub struct RqVersionCandidate {
    version_str: []const u8 = "",
    is_deprecated: bool = false,
    is_yanked: bool = false,
}

/// Result returned by `selectBestMatchingVersion`.
pub struct RqSelectionResult {
    found: bool = false,
    candidate_index: usize = 0,
    version_str: []const u8 = "",
    reused_existing: bool = false,
}

/// Single dependent's version range demand for a package.
pub struct RqDemandSpec {
    requester_name: []const u8 = "",
    pkg_name: []const u8 = "",
    range_expr: []const u8 = "",
}

/// Reconciled placement entry for one dependent demand.
pub struct RqReconciledDep {
    requester_name: []const u8 = "",
    pkg_name: []const u8 = "",
    range_expr: []const u8 = "",
    selected_version: []const u8 = "",
    placement: RqPlacementKind = RqPlacementKind.HoistedShared,

    static fn empty(): RqReconciledDep {
        return RqReconciledDep{};
    }
}

/// Summary produced by `reconcileResolvedGraph`.
pub struct RqReconcileSummary {
    pkg_name: []const u8 = "",
    hoisted_version: []const u8 = "",
    total_demands: u32 = 0,
    hoisted_shared: u32 = 0,
    nested_isolated: u32 = 0,
    unresolved_conflicts: u32 = 0,
    dedup_saved_installs: u32 = 0,
}

// ============================================================================
// Internal Byte, Hash & SemVer Helpers (Zero-Alloc)
// ============================================================================

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

fn indexOfByte(s: []const u8, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) return i;
        i += 1;
    }
    return null;
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end -= 1;
    }
    return s[start..end];
}

fn parseU32Dec(s: []const u8): u32? {
    if (s.len == 0) return null;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        val = val * 10 + @as(u64, c - '0');
        if (val > 4294967295) return null;
        i += 1;
    }
    return @intCast(val);
}

fn fnv1a64(data: []const u8): u64 {
    let h: u64 = 14695981039346656037;
    let i: usize = 0;
    while (i < data.len) {
        h = (h ^ @as(u64, data[i])) *% 1099511628211;
        i += 1;
    }
    return h;
}

fn cmpAscii(a: []const u8, b: []const u8): i32 {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) return if (a[i] < b[i]) -1 else 1;
        i += 1;
    }
    if (a.len != b.len) return if (a.len < b.len) -1 else 1;
    return 0;
}

fn comparePrerelease(a: []const u8, b: []const u8): i32 {
    let ai: usize = 0;
    let bi: usize = 0;
    while (ai < a.len && bi < b.len) {
        let a_end = ai;
        while (a_end < a.len && a[a_end] != '.') a_end += 1;
        let b_end = bi;
        while (b_end < b.len && b[b_end] != '.') b_end += 1;

        const a_seg = a[ai..a_end];
        const b_seg = b[bi..b_end];
        const a_num = parseU32Dec(a_seg);
        const b_num = parseU32Dec(b_seg);

        if (a_num != null && b_num != null) {
            if (a_num.? != b_num.?) return if (a_num.? < b_num.?) -1 else 1;
        } else if (a_num != null && b_num == null) {
            return -1; // Numeric identifiers always have lower precedence than non-numeric
        } else if (a_num == null && b_num != null) {
            return 1;
        } else {
            const c = cmpAscii(a_seg, b_seg);
            if (c != 0) return c;
        }

        ai = if (a_end < a.len) a_end + 1 else a.len;
        bi = if (b_end < b.len) b_end + 1 else b.len;
    }
    if (ai < a.len) return 1;
    if (bi < b.len) return -1;
    return 0;
}

fn appendBytes(out_buf: []u8, pos: *usize, data: []const u8)! {
    if (pos.* + data.len > out_buf.len) return ResolveQueueError.BufferTooSmall;
    let i: usize = 0;
    while (i < data.len) {
        out_buf[pos.* + i] = data[i];
        i += 1;
    }
    pos.* += data.len;
}

fn appendU32Decimal(out_buf: []u8, pos: *usize, value: u32)! {
    let digits: [16]u8 = [0; 16];
    let len: usize = 0;
    let v = value;
    if (v == 0) {
        digits[0] = @as(u8, 48);
        len = 1;
    } else {
        while (v > 0) {
            digits[len] = @intCast(48 + (v % 10));
            v = v / 10;
            len += 1;
        }
    }
    if (pos.* + len > out_buf.len) return ResolveQueueError.BufferTooSmall;
    let i: usize = 0;
    while (i < len) {
        out_buf[pos.* + i] = digits[len - 1 - i];
        i += 1;
    }
    pos.* += len;
}

// ============================================================================
// SemVer 2.0 Range Evaluator (`^`, `~`, `>=`, `<=`, `>`, `<`, AND, `||`, `*`)
// ============================================================================

struct ComparatorResult {
    matched: bool = false,
    allows_prerelease_for_core: bool = false,
}

fn evalSingleComparator(ver: RqSemver, raw_comp: []const u8): ComparatorResult! {
    const comp = trimWs(raw_comp);
    if (comp.len == 0 || bytesEqual(comp, "*") || bytesEqual(comp, "x") || bytesEqual(comp, "X") || bytesEqual(comp, "latest")) {
        return ComparatorResult{ matched: !ver.has_prerelease, allows_prerelease_for_core: false };
    }

    if (comp[0] == '^') {
        const base = RqSemver.parse(comp[1..])?;
        const upper = if (base.major > 0)
            RqSemver{ major: base.major + 1, minor: 0, patch: 0 }
        else if (base.minor > 0)
            RqSemver{ major: 0, minor: base.minor + 1, patch: 0 }
        else
            RqSemver{ major: 0, minor: 0, patch: base.patch + 1 };
        const ge = RqSemver.compare(ver, base) >= 0;
        const lt = RqSemver.compare(ver, upper) < 0;
        const pre_ok = !ver.has_prerelease || (base.has_prerelease && ver.sameCore(base));
        return ComparatorResult{
            matched: ge && lt && pre_ok,
            allows_prerelease_for_core: base.has_prerelease && ver.sameCore(base),
        };
    }

    if (comp[0] == '~') {
        const base = RqSemver.parse(comp[1..])?;
        const upper = RqSemver{ major: base.major, minor: base.minor + 1, patch: 0 };
        const ge = RqSemver.compare(ver, base) >= 0;
        const lt = RqSemver.compare(ver, upper) < 0;
        const pre_ok = !ver.has_prerelease || (base.has_prerelease && ver.sameCore(base));
        return ComparatorResult{
            matched: ge && lt && pre_ok,
            allows_prerelease_for_core: base.has_prerelease && ver.sameCore(base),
        };
    }

    if (startsWith(comp, ">=")) {
        const base = RqSemver.parse(comp[2..])?;
        return ComparatorResult{
            matched: RqSemver.compare(ver, base) >= 0,
            allows_prerelease_for_core: base.has_prerelease && ver.sameCore(base),
        };
    }
    if (startsWith(comp, "<=")) {
        const base = RqSemver.parse(comp[2..])?;
        return ComparatorResult{
            matched: RqSemver.compare(ver, base) <= 0,
            allows_prerelease_for_core: base.has_prerelease && ver.sameCore(base),
        };
    }
    if (comp[0] == '>') {
        const base = RqSemver.parse(comp[1..])?;
        return ComparatorResult{
            matched: RqSemver.compare(ver, base) > 0,
            allows_prerelease_for_core: base.has_prerelease && ver.sameCore(base),
        };
    }
    if (comp[0] == '<') {
        const base = RqSemver.parse(comp[1..])?;
        return ComparatorResult{
            matched: RqSemver.compare(ver, base) < 0,
            allows_prerelease_for_core: base.has_prerelease && ver.sameCore(base),
        };
    }
    if (comp[0] == '=') {
        const base = RqSemver.parse(comp[1..])?;
        return ComparatorResult{
            matched: RqSemver.compare(ver, base) == 0,
            allows_prerelease_for_core: base.has_prerelease && ver.sameCore(base),
        };
    }

    // Exact version match
    const base = RqSemver.parse(comp)?;
    return ComparatorResult{
        matched: RqSemver.compare(ver, base) == 0,
        allows_prerelease_for_core: base.has_prerelease && ver.sameCore(base),
    };
}

fn evalAndBranch(ver: RqSemver, branch_raw: []const u8): bool! {
    const branch = trimWs(branch_raw);
    if (branch.len == 0) return ResolveQueueError.InvalidRange;

    let any_pre_allowed = false;
    let i: usize = 0;
    let token_count: usize = 0;
    while (i < branch.len) {
        while (i < branch.len && (branch[i] == ' ' || branch[i] == '\t')) i += 1;
        if (i >= branch.len) break;
        const start = i;
        while (i < branch.len && branch[i] != ' ' && branch[i] != '\t') i += 1;
        const tok = branch[start..i];
        const res = evalSingleComparator(ver, tok)?;
        token_count += 1;
        if (res.allows_prerelease_for_core) any_pre_allowed = true;
        if (!res.matched) return false;
    }
    if (token_count == 0) return ResolveQueueError.InvalidRange;
    if (ver.has_prerelease && !any_pre_allowed) return false;
    return true;
}

/// Evaluates whether `ver` satisfies an npm/SemVer range expression supporting
/// `^`, `~`, `>=`, `<=`, `>`, `<`, `=`, exact versions, space-separated AND clauses,
/// `||` OR unions, `*`/`latest`, and SemVer 2.0 prerelease isolation.
pub fn satisfiesSemverRange(ver: RqSemver, range_expr: []const u8): bool! {
    const s = trimWs(range_expr);
    if (s.len == 0) return ResolveQueueError.InvalidRange;

    let start: usize = 0;
    let i: usize = 0;
    while (i <= s.len) {
        const at_or = (i + 1 < s.len && s[i] == '|' && s[i + 1] == '|');
        if (i == s.len || at_or) {
            const branch = s[start..i];
            if (evalAndBranch(ver, branch)?) {
                return true;
            }
            if (at_or) {
                i += 2;
                start = i;
                continue;
            }
        }
        i += 1;
    }
    return false;
}

// ============================================================================
// Deduplicated Priority Resolution Work Queue (`PkgResolveQueue`)
// ============================================================================

/// Zero-heap priority dependency resolution queue with `(name_hash, spec_hash)`
/// deduplication, priority elevation, concurrency throttling, and exponential backoff.
pub struct PkgResolveQueue {
    tasks: []RqTaskEntry,
    tasks_len: usize = 0,
    max_in_flight: u32 = RESOLVE_QUEUE_DEFAULT_MAX_IN_FLIGHT,
    max_retries: u8 = RESOLVE_QUEUE_DEFAULT_MAX_RETRIES,
    base_backoff_ms: u64 = RESOLVE_QUEUE_BASE_BACKOFF_MS,
    deduplicated_hits: u32 = 0,
    retries_scheduled: u32 = 0,

    static fn init(
        tasks_backing: []RqTaskEntry,
        max_in_flight: u32,
        max_retries: u8,
    ): PkgResolveQueue {
        return PkgResolveQueue{
            tasks: tasks_backing,
            tasks_len: 0,
            max_in_flight: if (max_in_flight == 0) 1 else max_in_flight,
            max_retries: max_retries,
        };
    }

    /// Enqueues `(name, spec)` or deduplicates against an existing identical task.
    /// If already queued with lower priority, promotes its priority immediately.
    fn enqueue(
        *_,
        name: []const u8,
        spec: []const u8,
        priority: RqDepPriority,
        parent_task_id: u32,
    ): u32! {
        const nh = fnv1a64(name);
        const sh = fnv1a64(spec);

        let i: usize = 0;
        while (i < _.tasks_len) {
            const t = _.tasks[i];
            if (t.name_hash == nh && t.spec_hash == sh && bytesEqual(t.name, name) && bytesEqual(t.spec, spec)) {
                _.deduplicated_hits += 1;
                if (@intFromEnum(priority) < @intFromEnum(t.priority)) {
                    let promoted = t;
                    promoted.priority = priority;
                    _.tasks[i] = promoted;
                }
                return t.task_id;
            }
            i += 1;
        }

        if (_.tasks_len >= _.tasks.len) return ResolveQueueError.SliceCapacityExceeded;
        const id: u32 = @intCast(_.tasks_len);
        _.tasks[_.tasks_len] = RqTaskEntry{
            task_id: id,
            name: name,
            spec: spec,
            name_hash: nh,
            spec_hash: sh,
            priority: priority,
            status: RqTaskStatus.Queued,
            attempts: 0,
            next_retry_after_ms: 0,
            resolved_version: "",
            parent_task_id: parent_task_id,
        };
        _.tasks_len += 1;
        return id;
    }

    fn currentInFlightCount(): u32 {
        let count: u32 = 0;
        let i: usize = 0;
        while (i < _.tasks_len) {
            if (_.tasks[i].status == RqTaskStatus.InFlight) count += 1;
            i += 1;
        }
        return count;
    }

    /// Pops the highest-priority ready task (`status == Queued` and `next_retry_after_ms <= now_ms`)
    /// if concurrency limit `max_in_flight` allows.
    fn popNextReady(*_, now_ms: u64): u32? {
        if (_.currentInFlightCount() >= _.max_in_flight) return null;

        let best_idx: usize? = null;
        let best_prio: u8 = 255;
        let i: usize = 0;
        while (i < _.tasks_len) {
            const t = _.tasks[i];
            if (t.status == RqTaskStatus.Queued && t.next_retry_after_ms <= now_ms) {
                const p = @intFromEnum(t.priority);
                if (best_idx == null || p < best_prio) {
                    best_prio = p;
                    best_idx = i;
                }
            }
            i += 1;
        }

        if (idx := best_idx) {
            let t = _.tasks[idx];
            t.status = RqTaskStatus.InFlight;
            t.attempts += 1;
            _.tasks[idx] = t;
            return t.task_id;
        }
        return null;
    }

    /// Marks an in-flight task as successfully resolved to `resolved_version`.
    fn markResolved(*_, task_id: u32, resolved_version: []const u8)! {
        const idx: usize = @intCast(task_id);
        if (idx >= _.tasks_len) return ResolveQueueError.InvalidTaskId;
        if (_.tasks[idx].status != RqTaskStatus.InFlight) return ResolveQueueError.InvalidStateTransition;
        let t = _.tasks[idx];
        t.status = RqTaskStatus.Resolved;
        t.resolved_version = resolved_version;
        _.tasks[idx] = t;
    }

    /// Handles a task failure: schedules exponential backoff retry if `retryable && attempts <= max_retries`;
    /// otherwise downgrades optional deps to `SkippedOptional` or marks `Failed`.
    fn markFailed(*_, task_id: u32, now_ms: u64, retryable: bool)! {
        const idx: usize = @intCast(task_id);
        if (idx >= _.tasks_len) return ResolveQueueError.InvalidTaskId;
        if (_.tasks[idx].status != RqTaskStatus.InFlight) return ResolveQueueError.InvalidStateTransition;

        let t = _.tasks[idx];
        if (retryable && t.attempts <= _.max_retries) {
            const shift_amt: u6 = if (t.attempts > 1) @intCast(t.attempts - 1) else 0;
            const delay = _.base_backoff_ms << shift_amt;
            t.status = RqTaskStatus.Queued;
            t.next_retry_after_ms = now_ms + delay;
            _.retries_scheduled += 1;
        } else if (t.priority == RqDepPriority.Optional) {
            t.status = RqTaskStatus.SkippedOptional;
        } else {
            t.status = RqTaskStatus.Failed;
        }
        _.tasks[idx] = t;
    }

    fn stats(): RqQueueStats {
        let s = RqQueueStats{
            total_enqueued: @intCast(_.tasks_len),
            deduplicated_hits: _.deduplicated_hits,
            retries_scheduled: _.retries_scheduled,
        };
        let i: usize = 0;
        while (i < _.tasks_len) {
            const st = _.tasks[i].status;
            if (st == RqTaskStatus.Queued) s.queued += 1;
            else if (st == RqTaskStatus.InFlight) s.in_flight += 1;
            else if (st == RqTaskStatus.Resolved) s.resolved += 1;
            else if (st == RqTaskStatus.Failed) s.failed += 1;
            else s.skipped_optional += 1;
            i += 1;
        }
        return s;
    }

    fn isDrained(): bool {
        let i: usize = 0;
        while (i < _.tasks_len) {
            const st = _.tasks[i].status;
            if (st == RqTaskStatus.Queued || st == RqTaskStatus.InFlight) return false;
            i += 1;
        }
        return true;
    }
}

// ============================================================================
// Best-Version Selector & Diamond Dependency Conflict Reconciler
// ============================================================================

/// Selects the optimal version from `candidates` matching `range_expr`.
/// Prefers reusing `existing_selected_ver` (if provided and compatible) to maximize
/// deduplication; otherwise chooses the highest non-yanked (preferring non-deprecated) SemVer.
pub fn selectBestMatchingVersion(
    range_expr: []const u8,
    candidates: []const RqVersionCandidate,
    existing_selected_ver: ([]const u8)?,
): RqSelectionResult! {
    if (existing := existing_selected_ver) {
        const ex_sv = RqSemver.parse(existing)?;
        if (satisfiesSemverRange(ex_sv, range_expr)?) {
            // Find candidate index if present
            let ci: usize = 0;
            while (ci < candidates.len) {
                if (bytesEqual(candidates[ci].version_str, existing)) {
                    return RqSelectionResult{
                        found: true,
                        candidate_index: ci,
                        version_str: candidates[ci].version_str,
                        reused_existing: true,
                    };
                }
                ci += 1;
            }
            return RqSelectionResult{
                found: true,
                candidate_index: 0,
                version_str: existing,
                reused_existing: true,
            };
        }
    }

    let best_idx: usize? = null;
    let best_sv = RqSemver{};
    let best_deprecated = true;

    let i: usize = 0;
    while (i < candidates.len) {
        const c = candidates[i];
        if (!c.is_yanked) {
            const sv = RqSemver.parse(c.version_str) catch {
                i += 1;
                continue;
            };
            if (satisfiesSemverRange(sv, range_expr)?) {
                let take = false;
                if (best_idx == null) {
                    take = true;
                } else if (best_deprecated && !c.is_deprecated) {
                    take = true;
                } else if (best_deprecated == c.is_deprecated && RqSemver.compare(sv, best_sv) > 0) {
                    take = true;
                }
                if (take) {
                    best_idx = i;
                    best_sv = sv;
                    best_deprecated = c.is_deprecated;
                }
            }
        }
        i += 1;
    }

    if (idx := best_idx) {
        return RqSelectionResult{
            found: true,
            candidate_index: idx,
            version_str: candidates[idx].version_str,
            reused_existing: false,
        };
    }
    return RqSelectionResult{};
}

/// Reconciles multiple requester demands for the same package against available `candidates`:
/// picks the highest-coverage (tie-broken by highest SemVer) candidate as the Hoisted Shared
/// version, places incompatible demands as Nested Isolated versions, and flags unresolvable demands.
pub fn reconcileResolvedGraph(
    pkg_name: []const u8,
    demands: []const RqDemandSpec,
    candidates: []const RqVersionCandidate,
    out_entries: []RqReconciledDep,
): RqReconcileSummary! {
    if (demands.len > out_entries.len) return ResolveQueueError.SliceCapacityExceeded;

    // Step 1: Pick the hoisted candidate that satisfies the most demands (tie-break: highest non-deprecated SemVer)
    let best_hoist_idx: usize? = null;
    let best_hoist_hits: u32 = 0;
    let best_hoist_sv = RqSemver{};
    let best_hoist_dep = true;

    let ci: usize = 0;
    while (ci < candidates.len) {
        const c = candidates[ci];
        if (!c.is_yanked) {
            const sv = RqSemver.parse(c.version_str) catch {
                ci += 1;
                continue;
            };
            let hits: u32 = 0;
            let di: usize = 0;
            while (di < demands.len) {
                const ok = satisfiesSemverRange(sv, demands[di].range_expr) catch false;
                if (ok) hits += 1;
                di += 1;
            }
            if (hits > 0) {
                let take = false;
                if (best_hoist_idx == null || hits > best_hoist_hits) {
                    take = true;
                } else if (hits == best_hoist_hits) {
                    if (best_hoist_dep && !c.is_deprecated) {
                        take = true;
                    } else if (best_hoist_dep == c.is_deprecated && RqSemver.compare(sv, best_hoist_sv) > 0) {
                        take = true;
                    }
                }
                if (take) {
                    best_hoist_idx = ci;
                    best_hoist_hits = hits;
                    best_hoist_sv = sv;
                    best_hoist_dep = c.is_deprecated;
                }
            }
        }
        ci += 1;
    }

    const hoisted_ver_opt: ([]const u8)? = if (hi := best_hoist_idx) candidates[hi].version_str else null;
    let summary = RqReconcileSummary{
        pkg_name: pkg_name,
        hoisted_version: hoisted_ver_opt ?? "",
        total_demands: @intCast(demands.len),
    };

    // Step 2: Assign each demand to HoistedShared, NestedIsolated, or UnresolvedConflict
    let di: usize = 0;
    while (di < demands.len) {
        const d = demands[di];
        const sel = selectBestMatchingVersion(d.range_expr, candidates, hoisted_ver_opt)?;
        if (!sel.found) {
            out_entries[di] = RqReconciledDep{
                requester_name: d.requester_name,
                pkg_name: d.pkg_name,
                range_expr: d.range_expr,
                selected_version: "",
                placement: RqPlacementKind.UnresolvedConflict,
            };
            summary.unresolved_conflicts += 1;
        } else if (sel.reused_existing) {
            out_entries[di] = RqReconciledDep{
                requester_name: d.requester_name,
                pkg_name: d.pkg_name,
                range_expr: d.range_expr,
                selected_version: sel.version_str,
                placement: RqPlacementKind.HoistedShared,
            };
            summary.hoisted_shared += 1;
        } else {
            out_entries[di] = RqReconciledDep{
                requester_name: d.requester_name,
                pkg_name: d.pkg_name,
                range_expr: d.range_expr,
                selected_version: sel.version_str,
                placement: RqPlacementKind.NestedIsolated,
            };
            summary.nested_isolated += 1;
        }
        di += 1;
    }

    if (summary.hoisted_shared > 1) {
        summary.dedup_saved_installs = summary.hoisted_shared - 1;
    }
    return summary;
}

// ============================================================================
// Zero-Alloc Queue & Reconciliation Summary Formatter
// ============================================================================

/// Formats a concise resolve-queue & reconciliation summary line into `out_buf`.
pub fn formatResolveQueueReport(
    q_stats: RqQueueStats,
    rec_sum: RqReconcileSummary,
    out_buf: []u8,
): usize! {
    let pos: usize = 0;
    appendBytes(out_buf, &pos, "resolve tasks=")?;
    appendU32Decimal(out_buf, &pos, q_stats.total_enqueued)?;
    appendBytes(out_buf, &pos, " resolved=")?;
    appendU32Decimal(out_buf, &pos, q_stats.resolved)?;
    appendBytes(out_buf, &pos, " dedup=")?;
    appendU32Decimal(out_buf, &pos, q_stats.deduplicated_hits)?;
    appendBytes(out_buf, &pos, " retries=")?;
    appendU32Decimal(out_buf, &pos, q_stats.retries_scheduled)?;
    appendBytes(out_buf, &pos, " failed=")?;
    appendU32Decimal(out_buf, &pos, q_stats.failed)?;
    if (rec_sum.pkg_name.len > 0) {
        appendBytes(out_buf, &pos, " | ")?;
        appendBytes(out_buf, &pos, rec_sum.pkg_name)?;
        appendBytes(out_buf, &pos, " hoisted=")?;
        appendBytes(out_buf, &pos, rec_sum.hoisted_version)?;
        appendBytes(out_buf, &pos, " shared=")?;
        appendU32Decimal(out_buf, &pos, rec_sum.hoisted_shared)?;
        appendBytes(out_buf, &pos, " nested=")?;
        appendU32Decimal(out_buf, &pos, rec_sum.nested_isolated)?;
        appendBytes(out_buf, &pos, " saved=")?;
        appendU32Decimal(out_buf, &pos, rec_sum.dedup_saved_installs)?;
    }
    return pos;
}
