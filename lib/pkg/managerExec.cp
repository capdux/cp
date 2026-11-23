// Source: Absorbed from /data/dev/bun/src/install/PackageManager/install_with_manager.cp | Synced: 2026-11-22T12:17:03Z
// ============================================================================
// lib/pkg/managerExec.cp — Top-Level Package Manager Execution Pipeline Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven top-level orchestrator for
// `cap install / add / remove / update / ci / link / audit / outdated`:
//   1. Action & CLI flag intent router (`PmExecAction`, `parsePmExecRequest`)
//      with semantic validation (`--ci` -> frozen-lockfile + clean rebuild,
//      `--dry-run`, `--lockfile-only`, `--no-save`, `--ignore-scripts`, `--filter`);
//   2. 9-stage monotonic execution pipeline state machine (`PmExecSession`):
//      `LoadConfig -> ReadManifest -> LoadLockfile -> ResolveGraph -> ComputeDiff
//       -> MaterializeStore -> LinkWorkspaces -> RunLifecycle -> PersistLockfile -> Done`
//      with structured progress event recording and frozen-lockfile drift guard;
//   3. `cap:simd`-assisted workspace glob filter matcher (`matchWorkspaceFilterGlob`)
//      and topological wave planner (`planWorkspaceInstallOrder`) with transitive
//      dependency closure and cycle detection;
//   4. Structured progress event log & final execution summary reporter
//      (`formatPmExecSummary`, `formatPmExecEventsLog`).

import { Simd } from "cap:simd";

/// Default concurrent network/extraction worker budget.
pub const PM_EXEC_DEFAULT_CONCURRENCY: u32 = 8;

/// Maximum permitted glob wildcard length safeguard.
pub const PM_EXEC_MAX_FILTER_LEN: usize = 256;

/// Sentinel index indicating an unassigned workspace slot.
pub const PM_EXEC_INVALID_WS_INDEX: usize = 0xFFFFFFFF;

/// Error set for top-level package manager execution routing, state transitions, and planning.
pub err PmExecError {
    UnknownAction,
    ConflictingFlags,
    InvalidFlagSyntax,
    InvalidStageTransition,
    FrozenLockfileDrift,
    WorkspaceCycleDetected,
    InvalidWorkspaceEdge,
    SliceCapacityExceeded,
    BufferTooSmall,
    EmptyFilterPattern,
}

// ============================================================================
// Internal Zero-Alloc Byte, Number, and Glob Helpers
// ============================================================================

fn pmBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn pmStartsWith(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) return false;
    return pmBytesEql(s[0..prefix.len], prefix);
}

fn pmBytesLessThan(a: []const u8, b: []const u8): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) return a[i] < b[i];
        i += 1;
    }
    return a.len < b.len;
}

fn pmParseU32Decimal(s: []const u8): u32? {
    if (s.len == 0 || s.len > 10) return null;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        val = val * 10 + @as(u64, c - '0');
        if (val > 0xFFFFFFFF) return null;
        i += 1;
    }
    return @as(u32, @intCast(val));
}

fn pmAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn pmAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn pmAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        pmAppendByte(out, pos, '0')?;
        return;
    }
    while (v > 0) {
        const d: u8 = @as(u8, @intCast(v % 10));
        tmp[n] = @as(u8, '0') + d;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        pmAppendByte(out, pos, tmp[n])?;
    }
}

fn pmTrimLeadingDotSlash(s: []const u8): []const u8 {
    let cur = s;
    while (cur.len >= 2 && cur[0] == '.' && cur[1] == '/') {
        cur = cur[2..];
    }
    return cur;
}

/// Iterative O(P*S) worst-case, O(1) space wildcard matcher supporting `*` and `?`.
/// Uses `cap:simd` fast-path (`Simd.indexOfAny2`) when pattern contains no wildcards.
fn pmGlobMatchIterative(pattern: []const u8, text: []const u8): bool {
    if (Simd.indexOfAny2(pattern, '*', '?') == null) {
        return pmBytesEql(pattern, text);
    }

    let pi: usize = 0;
    let ti: usize = 0;
    let star_pi: usize = PM_EXEC_INVALID_WS_INDEX;
    let star_ti: usize = 0;

    while (ti < text.len) {
        if (pi < pattern.len && (pattern[pi] == '?' || pattern[pi] == text[ti])) {
            pi += 1;
            ti += 1;
        } else if (pi < pattern.len && pattern[pi] == '*') {
            star_pi = pi;
            star_ti = ti;
            pi += 1;
        } else if (star_pi != PM_EXEC_INVALID_WS_INDEX) {
            pi = star_pi + 1;
            star_ti += 1;
            ti = star_ti;
        } else {
            return false;
        }
    }

    while (pi < pattern.len && pattern[pi] == '*') {
        pi += 1;
    }
    return pi == pattern.len;
}

// ============================================================================
// Section 1: Execution Action & CLI Intent Router
// ============================================================================

/// High-level package manager execution mode.
pub enum PmExecAction: u8 {
    Install = 0,
    Add = 1,
    Remove = 2,
    Update = 3,
    Ci = 4,
    Link = 5,
    Audit = 6,
    Outdated = 7,
}

/// Parses a CLI subcommand token (including common aliases) into `PmExecAction`.
pub fn parsePmExecAction(token: []const u8): PmExecAction? {
    if (pmBytesEql(token, "install") || pmBytesEql(token, "i")) return PmExecAction.Install;
    if (pmBytesEql(token, "add") || pmBytesEql(token, "a")) return PmExecAction.Add;
    if (pmBytesEql(token, "remove") || pmBytesEql(token, "rm") || pmBytesEql(token, "uninstall")) {
        return PmExecAction.Remove;
    }
    if (pmBytesEql(token, "update") || pmBytesEql(token, "up")) return PmExecAction.Update;
    if (pmBytesEql(token, "ci")) return PmExecAction.Ci;
    if (pmBytesEql(token, "link") || pmBytesEql(token, "ln")) return PmExecAction.Link;
    if (pmBytesEql(token, "audit")) return PmExecAction.Audit;
    if (pmBytesEql(token, "outdated")) return PmExecAction.Outdated;
    return null;
}

/// Returns the canonical lowercase string label for `action`.
pub fn pmExecActionName(action: PmExecAction): []const u8 {
    if (action == PmExecAction.Install) return "install";
    if (action == PmExecAction.Add) return "add";
    if (action == PmExecAction.Remove) return "remove";
    if (action == PmExecAction.Update) return "update";
    if (action == PmExecAction.Ci) return "ci";
    if (action == PmExecAction.Link) return "link";
    if (action == PmExecAction.Audit) return "audit";
    return "outdated";
}

/// Normalized execution policy flags parsed from CLI arguments.
pub struct PmExecFlags {
    dry_run: bool = false,
    lockfile_only: bool = false,
    frozen_lockfile: bool = false,
    clean_node_modules: bool = false,
    no_save: bool = false,
    ignore_scripts: bool = false,
    production: bool = false,
    save_dev: bool = false,
    save_optional: bool = false,
    save_exact: bool = false,
    force: bool = false,
    silent: bool = false,
    concurrency: u32 = PM_EXEC_DEFAULT_CONCURRENCY,

    static fn defaults(): PmExecFlags {
        return PmExecFlags{};
    }
}

/// Fully validated package manager execution request.
pub struct PmExecRequest {
    action: PmExecAction = PmExecAction.Install,
    flags: PmExecFlags = PmExecFlags{},
    target_count: usize = 0,
    filter_count: usize = 0,
    manifest_write_allowed: bool = false,
    lockfile_write_allowed: bool = true,
}

/// Parses `argv` (starting with the action verb such as `"install"`, `"add"`, `"ci"`)
/// into a validated `PmExecRequest`, writing positional package targets into
/// `targets_buf` and `--filter` patterns into `filters_buf`.
pub fn parsePmExecRequest(
    argv: []const []const u8,
    targets_buf: [][]const u8,
    filters_buf: [][]const u8,
): PmExecError!PmExecRequest {
    if (argv.len == 0) return err.UnknownAction;

    const action_opt = parsePmExecAction(argv[0]);
    let action = PmExecAction.Install;
    if (a := action_opt) {
        action = a;
    } else {
        return err.UnknownAction;
    }

    let flags = PmExecFlags.defaults();
    let target_count: usize = 0;
    let filter_count: usize = 0;
    let end_of_flags = false;

    // `cap ci` enforces frozen-lockfile and clean node_modules rebuild by default.
    if (action == PmExecAction.Ci) {
        flags.frozen_lockfile = true;
        flags.clean_node_modules = true;
    }

    let i: usize = 1;
    while (i < argv.len) {
        const arg = argv[i];
        if (!end_of_flags && pmBytesEql(arg, "--")) {
            end_of_flags = true;
            i += 1;
            continue;
        }

        if (!end_of_flags && arg.len > 1 && arg[0] == '-') {
            if (pmBytesEql(arg, "--dry-run")) {
                flags.dry_run = true;
            } else if (pmBytesEql(arg, "--lockfile-only")) {
                flags.lockfile_only = true;
            } else if (pmBytesEql(arg, "--frozen-lockfile")) {
                flags.frozen_lockfile = true;
            } else if (pmBytesEql(arg, "--no-save")) {
                flags.no_save = true;
            } else if (pmBytesEql(arg, "--ignore-scripts")) {
                flags.ignore_scripts = true;
            } else if (pmBytesEql(arg, "--production") || pmBytesEql(arg, "-P")) {
                flags.production = true;
            } else if (pmBytesEql(arg, "--dev") || pmBytesEql(arg, "-D") || pmBytesEql(arg, "--save-dev")) {
                flags.save_dev = true;
            } else if (pmBytesEql(arg, "--optional") || pmBytesEql(arg, "-O") || pmBytesEql(arg, "--save-optional")) {
                flags.save_optional = true;
            } else if (pmBytesEql(arg, "--exact") || pmBytesEql(arg, "-E") || pmBytesEql(arg, "--save-exact")) {
                flags.save_exact = true;
            } else if (pmBytesEql(arg, "--force") || pmBytesEql(arg, "-f")) {
                flags.force = true;
            } else if (pmBytesEql(arg, "--silent") || pmBytesEql(arg, "-s")) {
                flags.silent = true;
            } else if (pmStartsWith(arg, "--filter=")) {
                const pat = arg[9..];
                if (pat.len == 0 || pat.len > PM_EXEC_MAX_FILTER_LEN) return err.EmptyFilterPattern;
                if (filter_count >= filters_buf.len) return err.SliceCapacityExceeded;
                filters_buf[filter_count] = pat;
                filter_count += 1;
            } else if (pmBytesEql(arg, "--filter") || pmBytesEql(arg, "-F")) {
                if (i + 1 >= argv.len) return err.InvalidFlagSyntax;
                i += 1;
                const pat = argv[i];
                if (pat.len == 0 || pat.len > PM_EXEC_MAX_FILTER_LEN) return err.EmptyFilterPattern;
                if (filter_count >= filters_buf.len) return err.SliceCapacityExceeded;
                filters_buf[filter_count] = pat;
                filter_count += 1;
            } else if (pmStartsWith(arg, "--concurrency=")) {
                const raw = arg[14..];
                const parsed = pmParseU32Decimal(raw);
                if (n := parsed) {
                    if (n == 0 || n > 256) return err.InvalidFlagSyntax;
                    flags.concurrency = n;
                } else {
                    return err.InvalidFlagSyntax;
                }
            } else if (pmBytesEql(arg, "--concurrency")) {
                if (i + 1 >= argv.len) return err.InvalidFlagSyntax;
                i += 1;
                const parsed = pmParseU32Decimal(argv[i]);
                if (n := parsed) {
                    if (n == 0 || n > 256) return err.InvalidFlagSyntax;
                    flags.concurrency = n;
                } else {
                    return err.InvalidFlagSyntax;
                }
            } else {
                return err.InvalidFlagSyntax;
            }
            i += 1;
            continue;
        }

        // Positional package target
        if (arg.len == 0) {
            i += 1;
            continue;
        }
        if (target_count >= targets_buf.len) return err.SliceCapacityExceeded;
        targets_buf[target_count] = arg;
        target_count += 1;
        i += 1;
    }

    // Semantic conflict validation
    if (flags.save_dev && flags.save_optional) {
        return err.ConflictingFlags;
    }
    if (action == PmExecAction.Ci && flags.lockfile_only) {
        return err.ConflictingFlags;
    }
    if (flags.frozen_lockfile && (action == PmExecAction.Add || action == PmExecAction.Remove || action == PmExecAction.Update)) {
        return err.ConflictingFlags;
    }
    if ((action == PmExecAction.Add || action == PmExecAction.Remove) && target_count == 0) {
        return err.InvalidFlagSyntax;
    }

    const is_read_only_action = action == PmExecAction.Audit || action == PmExecAction.Outdated;
    const is_mutating_manifest = action == PmExecAction.Add || action == PmExecAction.Remove || action == PmExecAction.Update;

    const manifest_write_allowed = is_mutating_manifest && !flags.no_save && !flags.dry_run && !flags.frozen_lockfile;
    const lockfile_write_allowed = !is_read_only_action && !flags.dry_run && !flags.frozen_lockfile;

    return PmExecRequest{
        action: action,
        flags: flags,
        target_count: target_count,
        filter_count: filter_count,
        manifest_write_allowed: manifest_write_allowed,
        lockfile_write_allowed: lockfile_write_allowed,
    };
}

// ============================================================================
// Section 2: 9-Stage Monotonic Execution Pipeline State Machine
// ============================================================================

/// Monotonic lifecycle stages of `PmExecSession`.
pub enum PmExecStage: u8 {
    Init = 0,
    LoadConfig = 1,
    ReadManifest = 2,
    LoadLockfile = 3,
    ResolveGraph = 4,
    ComputeDiff = 5,
    MaterializeStore = 6,
    LinkWorkspaces = 7,
    RunLifecycle = 8,
    PersistLockfile = 9,
    Done = 10,
    Failed = 11,
}

/// Returns the canonical diagnostic name for `stage`.
pub fn pmExecStageName(stage: PmExecStage): []const u8 {
    if (stage == PmExecStage.Init) return "Init";
    if (stage == PmExecStage.LoadConfig) return "LoadConfig";
    if (stage == PmExecStage.ReadManifest) return "ReadManifest";
    if (stage == PmExecStage.LoadLockfile) return "LoadLockfile";
    if (stage == PmExecStage.ResolveGraph) return "ResolveGraph";
    if (stage == PmExecStage.ComputeDiff) return "ComputeDiff";
    if (stage == PmExecStage.MaterializeStore) return "MaterializeStore";
    if (stage == PmExecStage.LinkWorkspaces) return "LinkWorkspaces";
    if (stage == PmExecStage.RunLifecycle) return "RunLifecycle";
    if (stage == PmExecStage.PersistLockfile) return "PersistLockfile";
    if (stage == PmExecStage.Done) return "Done";
    return "Failed";
}

fn pmStageOrder(stage: PmExecStage): u8 {
    if (stage == PmExecStage.Init) return 0;
    if (stage == PmExecStage.LoadConfig) return 1;
    if (stage == PmExecStage.ReadManifest) return 2;
    if (stage == PmExecStage.LoadLockfile) return 3;
    if (stage == PmExecStage.ResolveGraph) return 4;
    if (stage == PmExecStage.ComputeDiff) return 5;
    if (stage == PmExecStage.MaterializeStore) return 6;
    if (stage == PmExecStage.LinkWorkspaces) return 7;
    if (stage == PmExecStage.RunLifecycle) return 8;
    if (stage == PmExecStage.PersistLockfile) return 9;
    if (stage == PmExecStage.Done) return 10;
    return 11;
}

fn pmStageFromOrder(ord: u8): PmExecStage {
    if (ord == 1) return PmExecStage.LoadConfig;
    if (ord == 2) return PmExecStage.ReadManifest;
    if (ord == 3) return PmExecStage.LoadLockfile;
    if (ord == 4) return PmExecStage.ResolveGraph;
    if (ord == 5) return PmExecStage.ComputeDiff;
    if (ord == 6) return PmExecStage.MaterializeStore;
    if (ord == 7) return PmExecStage.LinkWorkspaces;
    if (ord == 8) return PmExecStage.RunLifecycle;
    if (ord == 9) return PmExecStage.PersistLockfile;
    if (ord == 10) return PmExecStage.Done;
    return PmExecStage.Failed;
}

/// Structured progress event kind emitted during pipeline execution.
pub enum PmExecEventKind: u8 {
    StageEntered = 0,
    StageSkipped = 1,
    StageCompleted = 2,
    DiffComputed = 3,
    PackageMaterialized = 4,
    WorkspaceLinked = 5,
    ScriptExecuted = 6,
    Warning = 7,
    Error = 8,
}

/// Zero-copy structured progress event stored in caller-provided event buffer.
pub struct PmExecEvent {
    kind: PmExecEventKind = PmExecEventKind.StageEntered,
    stage: PmExecStage = PmExecStage.Init,
    timestamp_ms: u64 = 0,
    value_a: u32 = 0,
    value_b: u32 = 0,
    detail: []const u8 = "",

    static fn empty(): PmExecEvent {
        return PmExecEvent{};
    }
}

/// Aggregate execution telemetry collected by `PmExecSession`.
pub struct PmExecMetrics {
    packages_added: u32 = 0,
    packages_removed: u32 = 0,
    packages_updated: u32 = 0,
    packages_unchanged: u32 = 0,
    packages_materialized: u32 = 0,
    workspaces_selected: u32 = 0,
    workspaces_linked: u32 = 0,
    scripts_executed: u32 = 0,
    scripts_skipped: u32 = 0,
    lockfile_updated: bool = false,
    manifest_updated: bool = false,
    clean_rebuild_triggered: bool = false,
    stages_completed: u32 = 0,
    stages_skipped: u32 = 0,
    started_at_ms: u64 = 0,
    finished_at_ms: u64 = 0,
}

/// Returns whether `stage` should be skipped under `req`'s action and flags.
pub fn shouldSkipPmExecStage(req: PmExecRequest, stage: PmExecStage): bool {
    const is_read_only = req.action == PmExecAction.Audit || req.action == PmExecAction.Outdated;

    if (stage == PmExecStage.MaterializeStore) {
        return req.flags.dry_run || req.flags.lockfile_only || is_read_only;
    }
    if (stage == PmExecStage.LinkWorkspaces) {
        return req.flags.dry_run || req.flags.lockfile_only || is_read_only;
    }
    if (stage == PmExecStage.RunLifecycle) {
        return req.flags.dry_run || req.flags.lockfile_only || req.flags.ignore_scripts || is_read_only;
    }
    if (stage == PmExecStage.PersistLockfile) {
        return req.flags.dry_run || is_read_only || (!req.lockfile_write_allowed && !req.manifest_write_allowed);
    }
    return false;
}

/// Zero-heap, caller-slice-backed 9-stage monotonic execution session.
pub struct PmExecSession {
    request: PmExecRequest,
    stage: PmExecStage = PmExecStage.Init,
    events: []PmExecEvent,
    event_count: usize = 0,
    metrics: PmExecMetrics = PmExecMetrics{},
    error_code: u32 = 0,
    error_detail: []const u8 = "",

    static fn init(
        req: PmExecRequest,
        events_buf: []PmExecEvent,
        started_at_ms: u64,
    ): PmExecSession {
        return PmExecSession{
            request: req,
            stage: PmExecStage.Init,
            events: events_buf,
            event_count: 0,
            metrics: PmExecMetrics{
                clean_rebuild_triggered: req.flags.clean_node_modules,
                started_at_ms: started_at_ms,
                finished_at_ms: started_at_ms,
            },
            error_code: 0,
            error_detail: "",
        };
    }

    fn pushEvent(*_, ev: PmExecEvent) {
        if (_.event_count < _.events.len) {
            _.events[_.event_count] = ev;
            _.event_count += 1;
        }
    }

    /// Computes the next required non-skipped stage following the current stage.
    fn nextRequiredStage(): PmExecStage {
        if (_.stage == PmExecStage.Done || _.stage == PmExecStage.Failed) {
            return _.stage;
        }
        let ord: u8 = pmStageOrder(_.stage) + 1;
        while (ord < 10) {
            const cand = pmStageFromOrder(ord);
            if (!shouldSkipPmExecStage(_.request, cand)) {
                return cand;
            }
            ord += 1;
        }
        return PmExecStage.Done;
    }

    /// Advances the session strictly monotonically to `next_stage`.
    /// Intermediate stages that are policy-exempt (e.g. due to `--dry-run`,
    /// `--lockfile-only`, or `--ignore-scripts`) are automatically marked as
    /// `StageSkipped`; attempting to skip a mandatory stage or move backwards
    /// returns `err.InvalidStageTransition`.
    fn advanceStage(*_, next_stage: PmExecStage, now_ms: u64)! {
        if (_.stage == PmExecStage.Done || _.stage == PmExecStage.Failed) {
            return err.InvalidStageTransition;
        }
        const cur_ord = pmStageOrder(_.stage);
        const nxt_ord = pmStageOrder(next_stage);
        if (nxt_ord <= cur_ord || next_stage == PmExecStage.Failed) {
            return err.InvalidStageTransition;
        }
        if (next_stage != PmExecStage.Done && shouldSkipPmExecStage(_.request, next_stage)) {
            return err.InvalidStageTransition;
        }

        // Verify all intermediate stages between cur_ord and nxt_ord are policy-skippable.
        let check: u8 = cur_ord + 1;
        while (check < nxt_ord) {
            const mid = pmStageFromOrder(check);
            if (!shouldSkipPmExecStage(_.request, mid)) {
                return err.InvalidStageTransition;
            }
            check += 1;
        }

        // Complete previous active stage
        if (_.stage != PmExecStage.Init) {
            _.metrics.stages_completed += 1;
            _.pushEvent(PmExecEvent{
                kind: PmExecEventKind.StageCompleted,
                stage: _.stage,
                timestamp_ms: now_ms,
                detail: pmExecStageName(_.stage),
            });
        }

        // Emit skipped events for policy-exempt intermediate stages
        let skip_ord: u8 = cur_ord + 1;
        while (skip_ord < nxt_ord) {
            const skipped_stage = pmStageFromOrder(skip_ord);
            _.metrics.stages_skipped += 1;
            _.pushEvent(PmExecEvent{
                kind: PmExecEventKind.StageSkipped,
                stage: skipped_stage,
                timestamp_ms: now_ms,
                detail: pmExecStageName(skipped_stage),
            });
            skip_ord += 1;
        }

        _.stage = next_stage;
        _.metrics.finished_at_ms = now_ms;
        _.pushEvent(PmExecEvent{
            kind: PmExecEventKind.StageEntered,
            stage: next_stage,
            timestamp_ms: now_ms,
            detail: pmExecStageName(next_stage),
        });
    }

    /// Records the dependency diff during `ComputeDiff`.
    /// Under `--frozen-lockfile` (or `cap ci`), any drift (`added + removed + updated > 0`)
    /// transitions the session to `Failed` and returns `err.FrozenLockfileDrift`.
    fn recordDiff(
        *_,
        added: u32,
        removed: u32,
        updated: u32,
        unchanged: u32,
        now_ms: u64,
    )! {
        if (_.stage != PmExecStage.ComputeDiff) return err.InvalidStageTransition;
        _.metrics.packages_added = added;
        _.metrics.packages_removed = removed;
        _.metrics.packages_updated = updated;
        _.metrics.packages_unchanged = unchanged;
        _.metrics.finished_at_ms = now_ms;

        _.pushEvent(PmExecEvent{
            kind: PmExecEventKind.DiffComputed,
            stage: PmExecStage.ComputeDiff,
            timestamp_ms: now_ms,
            value_a: added + updated,
            value_b: removed,
            detail: "diff",
        });

        if (_.request.flags.frozen_lockfile && (added > 0 || removed > 0 || updated > 0)) {
            _.stage = PmExecStage.Failed;
            _.error_code = 1;
            _.error_detail = "lockfile had changes, but lockfile is frozen";
            _.pushEvent(PmExecEvent{
                kind: PmExecEventKind.Error,
                stage: PmExecStage.Failed,
                timestamp_ms: now_ms,
                value_a: added + removed + updated,
                detail: _.error_detail,
            });
            return err.FrozenLockfileDrift;
        }
    }

    /// Records materialized packages during `MaterializeStore`.
    fn recordMaterialized(*_, count: u32, now_ms: u64)! {
        if (_.stage != PmExecStage.MaterializeStore) return err.InvalidStageTransition;
        _.metrics.packages_materialized += count;
        _.metrics.finished_at_ms = now_ms;
        _.pushEvent(PmExecEvent{
            kind: PmExecEventKind.PackageMaterialized,
            stage: PmExecStage.MaterializeStore,
            timestamp_ms: now_ms,
            value_a: count,
            detail: "materialized",
        });
    }

    /// Records linked workspaces during `LinkWorkspaces`.
    fn recordWorkspacesLinked(*_, selected: u32, linked: u32, now_ms: u64)! {
        if (_.stage != PmExecStage.LinkWorkspaces) return err.InvalidStageTransition;
        _.metrics.workspaces_selected = selected;
        _.metrics.workspaces_linked = linked;
        _.metrics.finished_at_ms = now_ms;
        _.pushEvent(PmExecEvent{
            kind: PmExecEventKind.WorkspaceLinked,
            stage: PmExecStage.LinkWorkspaces,
            timestamp_ms: now_ms,
            value_a: selected,
            value_b: linked,
            detail: "workspaces",
        });
    }

    /// Records lifecycle script execution counts during `RunLifecycle`.
    fn recordLifecycleRun(*_, executed: u32, skipped: u32, now_ms: u64)! {
        if (_.stage != PmExecStage.RunLifecycle) return err.InvalidStageTransition;
        _.metrics.scripts_executed += executed;
        _.metrics.scripts_skipped += skipped;
        _.metrics.finished_at_ms = now_ms;
        _.pushEvent(PmExecEvent{
            kind: PmExecEventKind.ScriptExecuted,
            stage: PmExecStage.RunLifecycle,
            timestamp_ms: now_ms,
            value_a: executed,
            value_b: skipped,
            detail: "lifecycle",
        });
    }

    /// Records manifest & lockfile persistence outcomes during `PersistLockfile`.
    /// Respects `--no-save` by preventing `manifest_updated` when disallowed.
    fn recordPersistence(*_, wrote_manifest: bool, wrote_lockfile: bool, now_ms: u64)! {
        if (_.stage != PmExecStage.PersistLockfile) return err.InvalidStageTransition;
        _.metrics.manifest_updated = wrote_manifest && _.request.manifest_write_allowed;
        _.metrics.lockfile_updated = wrote_lockfile && _.request.lockfile_write_allowed;
        _.metrics.finished_at_ms = now_ms;
    }

    /// Transitions the session into `Failed` with structured error metadata.
    fn markFailed(*_, error_code: u32, detail: []const u8, now_ms: u64) {
        _.stage = PmExecStage.Failed;
        _.error_code = error_code;
        _.error_detail = detail;
        _.metrics.finished_at_ms = now_ms;
        _.pushEvent(PmExecEvent{
            kind: PmExecEventKind.Error,
            stage: PmExecStage.Failed,
            timestamp_ms: now_ms,
            value_a: error_code,
            detail: detail,
        });
    }

    /// Returns true if the session has reached terminal `Done` or `Failed`.
    fn isFinished(): bool {
        return _.stage == PmExecStage.Done || _.stage == PmExecStage.Failed;
    }
}

// ============================================================================
// Section 3: Workspace Multi-Package Glob Filtering & Topological Wave Planner
// ============================================================================

/// Outcome of matching a single `--filter` pattern against a workspace package.
pub enum WorkspaceFilterMatch: u8 {
    NoMatch = 0,
    Match = 1,
    NegateExclude = 2,
}

/// Evaluates a single Bun/Cap `--filter` pattern against a workspace's `name`
/// and `rel_path`.
///   - Leading `!` negates the match (producing `NegateExclude` when matched);
///   - Patterns containing `/` or starting with `.` match against `rel_path`;
///   - Otherwise patterns match against package `name`.
pub fn matchWorkspaceFilterGlob(
    raw_pattern: []const u8,
    pkg_name: []const u8,
    rel_path: []const u8,
): WorkspaceFilterMatch {
    if (raw_pattern.len == 0 || raw_pattern.len > PM_EXEC_MAX_FILTER_LEN) {
        return WorkspaceFilterMatch.NoMatch;
    }

    let negated = false;
    let pat = raw_pattern;
    if (pat[0] == '!') {
        negated = true;
        pat = pat[1..];
        if (pat.len == 0) return WorkspaceFilterMatch.NoMatch;
    }

    // Determine path filter vs name filter using SIMD slash scan.
    const is_path_filter = (pat[0] == '.') || (Simd.indexOfByte(pat, '/') != null && pat[0] != '@');
    const matched = if (is_path_filter)
        pmGlobMatchIterative(pmTrimLeadingDotSlash(pat), pmTrimLeadingDotSlash(rel_path))
    else
        pmGlobMatchIterative(pat, pkg_name);

    if (!matched) return WorkspaceFilterMatch.NoMatch;
    if (negated) return WorkspaceFilterMatch.NegateExclude;
    return WorkspaceFilterMatch.Match;
}

/// Input descriptor for one workspace package in a monorepo.
/// `deps_start..deps_start + deps_len` indexes into `ws_deps` (indices of workspace dependencies).
pub struct WorkspaceNodeInput {
    name: []const u8 = "",
    rel_path: []const u8 = "",
    is_root: bool = false,
    deps_start: usize = 0,
    deps_len: usize = 0,

    static fn empty(): WorkspaceNodeInput {
        return WorkspaceNodeInput{};
    }
}

/// Scheduled workspace installation step in topological wave order.
pub struct WorkspaceInstallStep {
    ws_index: usize = 0,
    wave: u32 = 0,
    direct_match: bool = false,
    transitive_dep: bool = false,

    static fn empty(): WorkspaceInstallStep {
        return WorkspaceInstallStep{};
    }
}

/// Summary of the planned workspace install order.
pub struct WorkspacePlanSummary {
    total_workspaces: usize = 0,
    selected_count: usize = 0,
    direct_match_count: usize = 0,
    transitive_dep_count: usize = 0,
    wave_count: u32 = 0,
    include_root: bool = true,
}

/// Filters workspaces using `filters` (supporting positive globs and `!` exclusions),
/// expands transitive workspace dependencies for selected packages, and computes a
/// deterministic topological wave schedule (Kahn's algorithm; ties broken by `name`).
///
/// Caller provides scratch slices of length `>= nodes.len`:
///   - `selected_flags: []u8` (0=unselected, 1=direct, 2=transitive)
///   - `indegree_scratch: []u32`
///   - `wave_scratch: []u32`
///   - `out_steps: []WorkspaceInstallStep`
pub fn planWorkspaceInstallOrder(
    nodes: []const WorkspaceNodeInput,
    ws_deps: []const usize,
    filters: []const []const u8,
    selected_flags: []u8,
    indegree_scratch: []u32,
    wave_scratch: []u32,
    out_steps: []WorkspaceInstallStep,
): PmExecError!WorkspacePlanSummary {
    const n = nodes.len;
    if (selected_flags.len < n || indegree_scratch.len < n || wave_scratch.len < n || out_steps.len < n) {
        return err.SliceCapacityExceeded;
    }

    let summary = WorkspacePlanSummary{
        total_workspaces: n,
        include_root: filters.len == 0,
    };
    if (n == 0) return summary;

    // Validate all dependency edge indices upfront.
    let v: usize = 0;
    while (v < n) {
        const node = nodes[v];
        if (node.deps_start > ws_deps.len || node.deps_len > ws_deps.len - node.deps_start) {
            return err.InvalidWorkspaceEdge;
        }
        let d: usize = 0;
        while (d < node.deps_len) {
            const target = ws_deps[node.deps_start + d];
            if (target >= n) return err.InvalidWorkspaceEdge;
            d += 1;
        }
        selected_flags[v] = 0;
        indegree_scratch[v] = 0;
        wave_scratch[v] = 0;
        v += 1;
    }

    // Step 1: Evaluate `--filter` patterns.
    // Check whether any positive (non-'!') filter was supplied.
    let has_positive_filter = false;
    let fi: usize = 0;
    while (fi < filters.len) {
        const pat = filters[fi];
        if (pat.len == 0) return err.EmptyFilterPattern;
        if (pat[0] != '!') has_positive_filter = true;
        fi += 1;
    }

    let i: usize = 0;
    while (i < n) {
        const node = nodes[i];
        let included = !has_positive_filter;
        let excluded = false;

        let k: usize = 0;
        while (k < filters.len) {
            const m = matchWorkspaceFilterGlob(filters[k], node.name, node.rel_path);
            if (m == WorkspaceFilterMatch.Match) {
                included = true;
            } else if (m == WorkspaceFilterMatch.NegateExclude) {
                excluded = true;
            }
            k += 1;
        }

        if (included && !excluded) {
            selected_flags[i] = 1;
            summary.direct_match_count += 1;
            if (node.is_root) summary.include_root = true;
        }
        i += 1;
    }

    // Step 2: Expand transitive workspace dependencies (fixed-point closure).
    let changed = true;
    while (changed) {
        changed = false;
        let u: usize = 0;
        while (u < n) {
            if (selected_flags[u] != 0) {
                const node = nodes[u];
                let d: usize = 0;
                while (d < node.deps_len) {
                    const dep_idx = ws_deps[node.deps_start + d];
                    if (selected_flags[dep_idx] == 0) {
                        selected_flags[dep_idx] = 2;
                        summary.transitive_dep_count += 1;
                        changed = true;
                    }
                    d += 1;
                }
            }
            u += 1;
        }
    }

    // Count total selected & compute in-degrees (number of selected dependencies per node).
    let selected_total: usize = 0;
    let node_idx: usize = 0;
    while (node_idx < n) {
        if (selected_flags[node_idx] != 0) {
            selected_total += 1;
            const node = nodes[node_idx];
            let deg: u32 = 0;
            let d: usize = 0;
            while (d < node.deps_len) {
                const dep_idx = ws_deps[node.deps_start + d];
                if (selected_flags[dep_idx] != 0) {
                    // Self-dependency is a cycle
                    if (dep_idx == node_idx) return err.WorkspaceCycleDetected;
                    deg += 1;
                }
                d += 1;
            }
            indegree_scratch[node_idx] = deg;
        }
        node_idx += 1;
    }

    // Step 3: Layered Kahn topological sort (wave by wave, deterministic name order within each wave).
    // `wave_scratch[idx]` uses 0 = unscheduled, >=1 = scheduled in wave (wave_scratch[idx] - 1).
    let scheduled_count: usize = 0;
    let current_wave: u32 = 0;

    while (scheduled_count < selected_total) {
        const wave_start = scheduled_count;
        let idx: usize = 0;
        while (idx < n) {
            if (selected_flags[idx] != 0 && wave_scratch[idx] == 0 && indegree_scratch[idx] == 0) {
                out_steps[scheduled_count] = WorkspaceInstallStep{
                    ws_index: idx,
                    wave: current_wave,
                    direct_match: selected_flags[idx] == 1,
                    transitive_dep: selected_flags[idx] == 2,
                };
                scheduled_count += 1;
            }
            idx += 1;
        }

        if (scheduled_count == wave_start) {
            return err.WorkspaceCycleDetected;
        }

        // Insertion-sort nodes inside this wave by `name` ascending for determinism.
        let s = wave_start + 1;
        while (s < scheduled_count) {
            const item = out_steps[s];
            let j = s;
            while (j > wave_start && pmBytesLessThan(nodes[item.ws_index].name, nodes[out_steps[j - 1].ws_index].name)) {
                out_steps[j] = out_steps[j - 1];
                j -= 1;
            }
            out_steps[j] = item;
            s += 1;
        }

        // Mark nodes in this wave as scheduled and decrement dependents' in-degrees.
        let w = wave_start;
        while (w < scheduled_count) {
            const done_ws = out_steps[w].ws_index;
            wave_scratch[done_ws] = current_wave + 1;
            w += 1;
        }

        let cand: usize = 0;
        while (cand < n) {
            if (selected_flags[cand] != 0 && wave_scratch[cand] == 0) {
                const node = nodes[cand];
                let d: usize = 0;
                while (d < node.deps_len) {
                    const dep_idx = ws_deps[node.deps_start + d];
                    if (wave_scratch[dep_idx] == current_wave + 1 && indegree_scratch[cand] > 0) {
                        indegree_scratch[cand] -= 1;
                    }
                    d += 1;
                }
            }
            cand += 1;
        }

        current_wave += 1;
    }

    summary.selected_count = selected_total;
    summary.wave_count = current_wave;
    return summary;
}

// ============================================================================
// Section 4: Structured Progress Event Log & Execution Summary Formatter
// ============================================================================

fn pmEventKindName(kind: PmExecEventKind): []const u8 {
    if (kind == PmExecEventKind.StageEntered) return "ENTER";
    if (kind == PmExecEventKind.StageSkipped) return "SKIP";
    if (kind == PmExecEventKind.StageCompleted) return "DONE";
    if (kind == PmExecEventKind.DiffComputed) return "DIFF";
    if (kind == PmExecEventKind.PackageMaterialized) return "STORE";
    if (kind == PmExecEventKind.WorkspaceLinked) return "LINK";
    if (kind == PmExecEventKind.ScriptExecuted) return "SCRIPT";
    if (kind == PmExecEventKind.Warning) return "WARN";
    return "ERROR";
}

/// Formats a structured execution summary report for `session` into `out_buf`.
pub fn formatPmExecSummary(
    session: PmExecSession,
    out_buf: []u8,
): PmExecError!usize {
    let pos: usize = 0;
    const m = session.metrics;
    const elapsed_ms = if (m.finished_at_ms >= m.started_at_ms) m.finished_at_ms - m.started_at_ms else 0;

    pmAppendBytes(out_buf, &pos, "cap ")?;
    pmAppendBytes(out_buf, &pos, pmExecActionName(session.request.action))?;
    pmAppendBytes(out_buf, &pos, " [")?;
    pmAppendBytes(out_buf, &pos, pmExecStageName(session.stage))?;
    pmAppendBytes(out_buf, &pos, "] (")?;
    pmAppendU64(out_buf, &pos, elapsed_ms)?;
    pmAppendBytes(out_buf, &pos, "ms)\n")?;

    pmAppendBytes(out_buf, &pos, "  packages: +")?;
    pmAppendU64(out_buf, &pos, @as(u64, m.packages_added))?;
    pmAppendBytes(out_buf, &pos, " -")?;
    pmAppendU64(out_buf, &pos, @as(u64, m.packages_removed))?;
    pmAppendBytes(out_buf, &pos, " ~")?;
    pmAppendU64(out_buf, &pos, @as(u64, m.packages_updated))?;
    pmAppendBytes(out_buf, &pos, " =")?;
    pmAppendU64(out_buf, &pos, @as(u64, m.packages_unchanged))?;
    pmAppendBytes(out_buf, &pos, " materialized=")?;
    pmAppendU64(out_buf, &pos, @as(u64, m.packages_materialized))?;
    pmAppendByte(out_buf, &pos, '\n')?;

    pmAppendBytes(out_buf, &pos, "  workspaces: selected=")?;
    pmAppendU64(out_buf, &pos, @as(u64, m.workspaces_selected))?;
    pmAppendBytes(out_buf, &pos, " linked=")?;
    pmAppendU64(out_buf, &pos, @as(u64, m.workspaces_linked))?;
    pmAppendBytes(out_buf, &pos, " scripts=")?;
    pmAppendU64(out_buf, &pos, @as(u64, m.scripts_executed))?;
    pmAppendBytes(out_buf, &pos, " skipped_scripts=")?;
    pmAppendU64(out_buf, &pos, @as(u64, m.scripts_skipped))?;
    pmAppendByte(out_buf, &pos, '\n')?;

    pmAppendBytes(out_buf, &pos, "  pipeline: completed=")?;
    pmAppendU64(out_buf, &pos, @as(u64, m.stages_completed))?;
    pmAppendBytes(out_buf, &pos, " skipped=")?;
    pmAppendU64(out_buf, &pos, @as(u64, m.stages_skipped))?;
    pmAppendBytes(out_buf, &pos, " manifest_saved=")?;
    pmAppendBytes(out_buf, &pos, if (m.manifest_updated) "true" else "false")?;
    pmAppendBytes(out_buf, &pos, " lockfile_saved=")?;
    pmAppendBytes(out_buf, &pos, if (m.lockfile_updated) "true" else "false")?;
    pmAppendByte(out_buf, &pos, '\n')?;

    if (session.stage == PmExecStage.Failed && session.error_detail.len > 0) {
        pmAppendBytes(out_buf, &pos, "  error: ")?;
        pmAppendBytes(out_buf, &pos, session.error_detail)?;
        pmAppendByte(out_buf, &pos, '\n')?;
    }

    return pos;
}

/// Formats the structured progress event timeline into `out_buf`.
pub fn formatPmExecEventsLog(
    events: []const PmExecEvent,
    out_buf: []u8,
): PmExecError!usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < events.len) {
        const ev = events[i];
        pmAppendByte(out_buf, &pos, '[')?;
        pmAppendU64(out_buf, &pos, ev.timestamp_ms)?;
        pmAppendBytes(out_buf, &pos, "ms] ")?;
        pmAppendBytes(out_buf, &pos, pmEventKindName(ev.kind))?;
        pmAppendByte(out_buf, &pos, ' ')?;
        pmAppendBytes(out_buf, &pos, pmExecStageName(ev.stage))?;
        if (ev.detail.len > 0) {
            pmAppendBytes(out_buf, &pos, ": ")?;
            pmAppendBytes(out_buf, &pos, ev.detail)?;
        }
        pmAppendByte(out_buf, &pos, '\n')?;
        i += 1;
    }
    return pos;
}
