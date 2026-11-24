// Source: Absorbed from /data/dev/bun/src/install/PackageManager/runTasks.cp | Synced: 2026-11-23T20:24:39Z
// ============================================================================
// lib/pkg/scriptRunner.cp — Zero-Alloc Lifecycle & Monorepo Script Task Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven script task graph scheduler and
// execution coordinator for `cap run` / `cap install` / Monorepo workspaces:
//   1. Lifecycle hook sequence planner (`planLifecycleSequence`) & custom
//      `pre<script>` -> `<script>` -> `post<script>` expander
//      (`expandCustomScriptHooks`) with implicit `node-gyp rebuild` support;
//   2. Zero-trust script policy gate (`evaluateScriptTrust`,
//      `evaluateBatchScriptTrust`) with `trustedDependencies` wildcard allowlist,
//      explicit deny-list, `--ignore-scripts`, workspace auto-trust, and
//      Bun/cap-style blocked-script summary reporter
//      (`formatUntrustedScriptsReport`);
//   3. Dependency-aware concurrent DAG worker pool (`PkgScriptScheduler`)
//      with Kahn cycle detection, priority tie-breaking, concurrency throttling,
//      per-task timeout detection, `FailFast` vs `ContinueOnError`, optional
//      package degradation, and aggregate exit code computation;
//   4. Environment variable injector (`buildScriptEnvVars`) with `.bin` PATH
//      prepending, `npm_lifecycle_*` / `npm_package_*` / `CAP_EXEC_PATH`
//      injection, and sensitive credential key filtering (`isSensitiveEnvKey`);
//   5. Color/label-prefixed line accumulator (`ScriptLineBuffer`) and
//      execution timing summary formatter (`formatScriptRunSummary`).

/// Default concurrent script worker limit.
pub const SCRIPT_RUNNER_DEFAULT_CONCURRENCY: u32 = 4;

/// Default per-script timeout in milliseconds (5 minutes).
pub const SCRIPT_RUNNER_DEFAULT_TIMEOUT_MS: u64 = 300000;

/// Exit code assigned to scripts terminated by timeout (POSIX `timeout` convention).
pub const SCRIPT_RUNNER_TIMEOUT_EXIT_CODE: i32 = 124;

/// Sentinel task index indicating no task.
pub const SCRIPT_RUNNER_NO_TASK: u32 = 0xFFFFFFFF;

/// Script task scheduling, trust gating, env building, and formatting error set.
pub err ScriptRunnerError {
    InvalidTaskIndex,
    InvalidStateTransition,
    DependencyCycleDetected,
    DuplicateTaskId,
    InvalidDependencyEdge,
    GraphNotFinalized,
    GraphAlreadyFinalized,
    UntrustedScriptsBlocked,
    ScriptTimedOut,
    RequiredScriptFailed,
    SliceCapacityExceeded,
    BufferTooSmall,
    EmptyScriptName,
}

// ============================================================================
// Internal Zero-Alloc Byte, Wildcard & Formatting Helpers
// ============================================================================

fn srBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn srAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn srBytesEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (srAsciiLower(a[i]) != srAsciiLower(b[i])) return false;
        i += 1;
    }
    return true;
}

fn srStartsWith(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) return false;
    return srBytesEql(s[0..prefix.len], prefix);
}

fn srContainsIgnoreCase(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    const limit = haystack.len - needle.len;
    while (i <= limit) {
        if (srBytesEqlIgnoreCase(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

fn srMatchesWithPrefix(candidate: []const u8, prefix: []const u8, base: []const u8): bool {
    if (candidate.len != prefix.len + base.len) return false;
    if (!srBytesEql(candidate[0..prefix.len], prefix)) return false;
    return srBytesEql(candidate[prefix.len..candidate.len], base);
}

/// Glob matcher supporting `*` wildcards (e.g. `@scope/*`, `*-plugin`, `*`).
fn srWildcardMatch(pattern: []const u8, text: []const u8): bool {
    let pi: usize = 0;
    let ti: usize = 0;
    let star_pi: usize = 0;
    let star_ti: usize = 0;
    let has_star = false;

    while (ti < text.len) {
        if (pi < pattern.len && pattern[pi] == '*') {
            has_star = true;
            star_pi = pi;
            star_ti = ti;
            pi += 1;
        } else if (pi < pattern.len && pattern[pi] == text[ti]) {
            pi += 1;
            ti += 1;
        } else if (has_star) {
            star_ti += 1;
            ti = star_ti;
            pi = star_pi + 1;
        } else {
            return false;
        }
    }

    while (pi < pattern.len && pattern[pi] == '*') {
        pi += 1;
    }
    return pi == pattern.len;
}

fn srAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn srAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn srAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        srAppendByte(out, pos, '0')?;
        return;
    }
    while (v > 0) {
        const digit: u8 = @as(u8, @intCast(v % 10));
        tmp[n] = @as(u8, '0') + digit;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        srAppendByte(out, pos, tmp[n])?;
    }
}

fn srAppendI32(out: []u8, pos: *usize, val: i32)! {
    if (val < 0) {
        srAppendByte(out, pos, '-')?;
        const abs_val: u64 = @as(u64, @intCast(-@as(i64, val)));
        srAppendU64(out, pos, abs_val)?;
    } else {
        srAppendU64(out, pos, @as(u64, @intCast(val)))?;
    }
}

/// Formats a millisecond duration into human-readable form (`42ms`, `1.25s`, `2m 05s`).
pub fn formatScriptDurationMs(ms: u64, out: []u8, pos: *usize)! {
    if (ms < 1000) {
        srAppendU64(out, pos, ms)?;
        srAppendBytes(out, pos, "ms")?;
        return;
    }
    if (ms < 60000) {
        const secs = ms / 1000;
        const centis = (ms % 1000) / 10;
        srAppendU64(out, pos, secs)?;
        srAppendByte(out, pos, '.')?;
        if (centis < 10) {
            srAppendByte(out, pos, '0')?;
        }
        srAppendU64(out, pos, centis)?;
        srAppendByte(out, pos, 's')?;
        return;
    }
    const mins = ms / 60000;
    const rem_secs = (ms % 60000) / 1000;
    srAppendU64(out, pos, mins)?;
    srAppendBytes(out, pos, "m ")?;
    if (rem_secs < 10) {
        srAppendByte(out, pos, '0')?;
    }
    srAppendU64(out, pos, rem_secs)?;
    srAppendByte(out, pos, 's')?;
}

// ============================================================================
// Section 1: Lifecycle Stage Planning & Custom pre/post Hook Expansion
// ============================================================================

/// Ordered lifecycle and custom script hook stages.
pub enum ScriptHookStage: u8 {
    Preinstall = 0,
    Install = 1,
    Postinstall = 2,
    Preprepare = 3,
    Prepare = 4,
    Postprepare = 5,
    PreCustom = 6,
    Custom = 7,
    PostCustom = 8,

    /// Returns the canonical npm/Bun lifecycle event name for built-in stages.
    fn asLifecycleName(): []const u8 {
        return switch (_) {
            .Preinstall => "preinstall",
            .Install => "install",
            .Postinstall => "postinstall",
            .Preprepare => "preprepare",
            .Prepare => "prepare",
            .Postprepare => "postprepare",
            .PreCustom => "pre<custom>",
            .Custom => "<custom>",
            .PostCustom => "post<custom>",
        };
    }

    /// Returns true for `preinstall` / `install` / `postinstall`.
    fn isInstallLifecycle(): bool {
        return switch (_) {
            .Preinstall, .Install, .Postinstall => true,
            else => false,
        };
    }

    /// Returns true for `preprepare` / `prepare` / `postprepare`.
    fn isPrepareLifecycle(): bool {
        return switch (_) {
            .Preprepare, .Prepare, .Postprepare => true,
            else => false,
        };
    }

    /// Returns true for user-defined `pre<script>` / `<script>` / `post<script>`.
    fn isCustomHook(): bool {
        return switch (_) {
            .PreCustom, .Custom, .PostCustom => true,
            else => false,
        };
    }

    /// Returns monotonic execution rank within its family.
    fn orderRank(): u8 {
        return switch (_) {
            .Preinstall => 0,
            .Install => 1,
            .Postinstall => 2,
            .Preprepare => 3,
            .Prepare => 4,
            .Postprepare => 5,
            .PreCustom => 6,
            .Custom => 7,
            .PostCustom => 8,
        };
    }

    /// Parses a standard lifecycle script name into its stage enum.
    static fn fromLifecycleName(raw_name: []const u8): ScriptHookStage? {
        if (srBytesEql(raw_name, "preinstall")) return ScriptHookStage.Preinstall;
        if (srBytesEql(raw_name, "install")) return ScriptHookStage.Install;
        if (srBytesEql(raw_name, "postinstall")) return ScriptHookStage.Postinstall;
        if (srBytesEql(raw_name, "preprepare")) return ScriptHookStage.Preprepare;
        if (srBytesEql(raw_name, "prepare")) return ScriptHookStage.Prepare;
        if (srBytesEql(raw_name, "postprepare")) return ScriptHookStage.Postprepare;
        return null;
    }
}

/// Single key/value entry from a package's `"scripts"` manifest section.
pub struct ScriptManifestEntry {
    name: []const u8 = "",
    command: []const u8 = "",

    static fn empty(): ScriptManifestEntry {
        return ScriptManifestEntry{};
    }
}

/// Package metadata descriptor used for lifecycle planning and trust evaluation.
pub struct ScriptPackageHooks {
    package_name: []const u8 = "",
    package_version: []const u8 = "",
    package_dir: []const u8 = "",
    is_workspace: bool = false,
    is_root: bool = false,
    is_optional: bool = false,
    has_binding_gyp: bool = false,

    static fn empty(): ScriptPackageHooks {
        return ScriptPackageHooks{};
    }
}

/// Concrete planned script invocation step produced by hook planners.
pub struct PlannedScriptStep {
    stage: ScriptHookStage = ScriptHookStage.Install,
    event_name: []const u8 = "",
    command: []const u8 = "",
    package_name: []const u8 = "",
    package_dir: []const u8 = "",
    is_implicit_gyp: bool = false,

    static fn empty(): PlannedScriptStep {
        return PlannedScriptStep{};
    }
}

/// Options controlling `planLifecycleSequence`.
pub struct LifecyclePlanOptions {
    include_prepare: bool = false,
    run_implicit_node_gyp: bool = true,
    ignore_scripts: bool = false,

    static fn defaults(): LifecyclePlanOptions {
        return LifecyclePlanOptions{};
    }
}

fn findScriptCommand(scripts: []const ScriptManifestEntry, name: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i < scripts.len) {
        if (srBytesEql(scripts[i].name, name) && scripts[i].command.len > 0) {
            return scripts[i].command;
        }
        i += 1;
    }
    return null;
}

/// Plans the ordered install/prepare lifecycle sequence for a package:
/// `preinstall` -> `install` (or implicit `node-gyp rebuild` when `binding.gyp`
/// is present and neither `preinstall` nor `install` is defined) -> `postinstall`
/// -> optional `preprepare` -> `prepare` -> `postprepare`.
pub fn planLifecycleSequence(
    pkg: ScriptPackageHooks,
    scripts: []const ScriptManifestEntry,
    opts: LifecyclePlanOptions,
    out_steps: []PlannedScriptStep,
): usize! {
    if (opts.ignore_scripts) return 0;

    let count: usize = 0;
    let has_preinstall = false;
    let has_install = false;

    if (cmd := findScriptCommand(scripts, "preinstall")) {
        if (count >= out_steps.len) return err.SliceCapacityExceeded;
        out_steps[count] = PlannedScriptStep{
            stage: ScriptHookStage.Preinstall,
            event_name: "preinstall",
            command: cmd,
            package_name: pkg.package_name,
            package_dir: pkg.package_dir,
            is_implicit_gyp: false,
        };
        count += 1;
        has_preinstall = true;
    }

    if (cmd := findScriptCommand(scripts, "install")) {
        if (count >= out_steps.len) return err.SliceCapacityExceeded;
        out_steps[count] = PlannedScriptStep{
            stage: ScriptHookStage.Install,
            event_name: "install",
            command: cmd,
            package_name: pkg.package_name,
            package_dir: pkg.package_dir,
            is_implicit_gyp: false,
        };
        count += 1;
        has_install = true;
    } else if (pkg.has_binding_gyp && opts.run_implicit_node_gyp && !has_preinstall && !has_install) {
        if (count >= out_steps.len) return err.SliceCapacityExceeded;
        out_steps[count] = PlannedScriptStep{
            stage: ScriptHookStage.Install,
            event_name: "install",
            command: "node-gyp rebuild",
            package_name: pkg.package_name,
            package_dir: pkg.package_dir,
            is_implicit_gyp: true,
        };
        count += 1;
    }

    if (cmd := findScriptCommand(scripts, "postinstall")) {
        if (count >= out_steps.len) return err.SliceCapacityExceeded;
        out_steps[count] = PlannedScriptStep{
            stage: ScriptHookStage.Postinstall,
            event_name: "postinstall",
            command: cmd,
            package_name: pkg.package_name,
            package_dir: pkg.package_dir,
            is_implicit_gyp: false,
        };
        count += 1;
    }

    if (opts.include_prepare) {
        if (cmd := findScriptCommand(scripts, "preprepare")) {
            if (count >= out_steps.len) return err.SliceCapacityExceeded;
            out_steps[count] = PlannedScriptStep{
                stage: ScriptHookStage.Preprepare,
                event_name: "preprepare",
                command: cmd,
                package_name: pkg.package_name,
                package_dir: pkg.package_dir,
                is_implicit_gyp: false,
            };
            count += 1;
        }
        if (cmd := findScriptCommand(scripts, "prepare")) {
            if (count >= out_steps.len) return err.SliceCapacityExceeded;
            out_steps[count] = PlannedScriptStep{
                stage: ScriptHookStage.Prepare,
                event_name: "prepare",
                command: cmd,
                package_name: pkg.package_name,
                package_dir: pkg.package_dir,
                is_implicit_gyp: false,
            };
            count += 1;
        }
        if (cmd := findScriptCommand(scripts, "postprepare")) {
            if (count >= out_steps.len) return err.SliceCapacityExceeded;
            out_steps[count] = PlannedScriptStep{
                stage: ScriptHookStage.Postprepare,
                event_name: "postprepare",
                command: cmd,
                package_name: pkg.package_name,
                package_dir: pkg.package_dir,
                is_implicit_gyp: false,
            };
            count += 1;
        }
    }

    return count;
}

/// Expands a custom user script `<script>` into `pre<script>` -> `<script>` -> `post<script>`.
/// Only emits hooks present in `scripts`. If `<script>` itself is not found, returns 0.
pub fn expandCustomScriptHooks(
    pkg: ScriptPackageHooks,
    target_script: []const u8,
    scripts: []const ScriptManifestEntry,
    ignore_pre_post: bool,
    out_steps: []PlannedScriptStep,
): usize! {
    if (target_script.len == 0) return err.EmptyScriptName;

    // Locate the main target script first; if absent, do not run orphaned pre/post hooks.
    let main_cmd: []const u8 = "";
    let found_main = false;
    let i: usize = 0;
    while (i < scripts.len) {
        if (srBytesEql(scripts[i].name, target_script) && scripts[i].command.len > 0) {
            main_cmd = scripts[i].command;
            found_main = true;
            break;
        }
        i += 1;
    }
    if (!found_main) return 0;

    let count: usize = 0;

    // 1. Optional pre<script> hook
    if (!ignore_pre_post) {
        let j: usize = 0;
        while (j < scripts.len) {
            if (srMatchesWithPrefix(scripts[j].name, "pre", target_script) && scripts[j].command.len > 0) {
                if (count >= out_steps.len) return err.SliceCapacityExceeded;
                out_steps[count] = PlannedScriptStep{
                    stage: ScriptHookStage.PreCustom,
                    event_name: scripts[j].name,
                    command: scripts[j].command,
                    package_name: pkg.package_name,
                    package_dir: pkg.package_dir,
                    is_implicit_gyp: false,
                };
                count += 1;
                break;
            }
            j += 1;
        }
    }

    // 2. Main <script>
    if (count >= out_steps.len) return err.SliceCapacityExceeded;
    out_steps[count] = PlannedScriptStep{
        stage: ScriptHookStage.Custom,
        event_name: target_script,
        command: main_cmd,
        package_name: pkg.package_name,
        package_dir: pkg.package_dir,
        is_implicit_gyp: false,
    };
    count += 1;

    // 3. Optional post<script> hook
    if (!ignore_pre_post) {
        let k: usize = 0;
        while (k < scripts.len) {
            if (srMatchesWithPrefix(scripts[k].name, "post", target_script) && scripts[k].command.len > 0) {
                if (count >= out_steps.len) return err.SliceCapacityExceeded;
                out_steps[count] = PlannedScriptStep{
                    stage: ScriptHookStage.PostCustom,
                    event_name: scripts[k].name,
                    command: scripts[k].command,
                    package_name: pkg.package_name,
                    package_dir: pkg.package_dir,
                    is_implicit_gyp: false,
                };
                count += 1;
                break;
            }
            k += 1;
        }
    }

    return count;
}

// ============================================================================
// Section 2: Zero-Trust Security Gate & Untrusted Script Summary Reporter
// ============================================================================

/// Reason code explaining why a package's lifecycle scripts were allowed or blocked.
pub enum ScriptTrustReason: u8 {
    SkippedNoScripts = 0,
    AllowedRoot = 1,
    AllowedWorkspace = 2,
    AllowedTrustedDependency = 3,
    AllowedGlobalTrustAll = 4,
    BlockedIgnoreScripts = 5,
    BlockedDeniedPackage = 6,
    BlockedUntrustedThirdParty = 7,
}

/// Zero-trust security policy for evaluating lifecycle script execution.
pub struct ScriptTrustPolicy {
    ignore_scripts: bool = false,
    trust_all: bool = false,
    auto_trust_root: bool = true,
    auto_trust_workspaces: bool = true,
    trusted_dependencies: []const []const u8 = [],
    blocked_packages: []const []const u8 = [],

    static fn defaults(): ScriptTrustPolicy {
        return ScriptTrustPolicy{};
    }
}

/// Per-package trust evaluation decision.
pub struct ScriptTrustDecision {
    package_name: []const u8 = "",
    package_version: []const u8 = "",
    allowed: bool = false,
    reason: ScriptTrustReason = ScriptTrustReason.SkippedNoScripts,
    script_count: u32 = 0,

    static fn empty(): ScriptTrustDecision {
        return ScriptTrustDecision{};
    }
}

/// Batch trust evaluation counts.
pub struct ScriptTrustSummary {
    total_evaluated: u32 = 0,
    allowed_count: u32 = 0,
    blocked_untrusted_count: u32 = 0,
    blocked_ignored_count: u32 = 0,
    blocked_denied_count: u32 = 0,
    skipped_no_scripts_count: u32 = 0,
}

fn matchesAnyPattern(name: []const u8, patterns: []const []const u8): bool {
    let i: usize = 0;
    while (i < patterns.len) {
        if (patterns[i].len > 0 && srWildcardMatch(patterns[i], name)) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Evaluates whether a package is permitted to execute its lifecycle scripts.
/// Precedence:
///   1. `script_count == 0` -> `SkippedNoScripts` (not run)
///   2. `--ignore-scripts` -> `BlockedIgnoreScripts`
///   3. Explicit `blocked_packages` deny-list -> `BlockedDeniedPackage`
///   4. Local root package (`is_root && auto_trust_root`) -> `AllowedRoot`
///   5. Local workspace package (`is_workspace && auto_trust_workspaces`) -> `AllowedWorkspace`
///   6. Explicit `trustedDependencies` match (supports `*` wildcards) -> `AllowedTrustedDependency`
///   7. `trust_all` override -> `AllowedGlobalTrustAll`
///   8. Otherwise -> `BlockedUntrustedThirdParty`
pub fn evaluateScriptTrust(
    pkg: ScriptPackageHooks,
    script_count: u32,
    policy: ScriptTrustPolicy,
): ScriptTrustDecision {
    if (script_count == 0) {
        return ScriptTrustDecision{
            package_name: pkg.package_name,
            package_version: pkg.package_version,
            allowed: false,
            reason: ScriptTrustReason.SkippedNoScripts,
            script_count: 0,
        };
    }

    if (policy.ignore_scripts) {
        return ScriptTrustDecision{
            package_name: pkg.package_name,
            package_version: pkg.package_version,
            allowed: false,
            reason: ScriptTrustReason.BlockedIgnoreScripts,
            script_count: script_count,
        };
    }

    if (matchesAnyPattern(pkg.package_name, policy.blocked_packages)) {
        return ScriptTrustDecision{
            package_name: pkg.package_name,
            package_version: pkg.package_version,
            allowed: false,
            reason: ScriptTrustReason.BlockedDeniedPackage,
            script_count: script_count,
        };
    }

    if (pkg.is_root && policy.auto_trust_root) {
        return ScriptTrustDecision{
            package_name: pkg.package_name,
            package_version: pkg.package_version,
            allowed: true,
            reason: ScriptTrustReason.AllowedRoot,
            script_count: script_count,
        };
    }

    if (pkg.is_workspace && policy.auto_trust_workspaces) {
        return ScriptTrustDecision{
            package_name: pkg.package_name,
            package_version: pkg.package_version,
            allowed: true,
            reason: ScriptTrustReason.AllowedWorkspace,
            script_count: script_count,
        };
    }

    if (matchesAnyPattern(pkg.package_name, policy.trusted_dependencies)) {
        return ScriptTrustDecision{
            package_name: pkg.package_name,
            package_version: pkg.package_version,
            allowed: true,
            reason: ScriptTrustReason.AllowedTrustedDependency,
            script_count: script_count,
        };
    }

    if (policy.trust_all) {
        return ScriptTrustDecision{
            package_name: pkg.package_name,
            package_version: pkg.package_version,
            allowed: true,
            reason: ScriptTrustReason.AllowedGlobalTrustAll,
            script_count: script_count,
        };
    }

    return ScriptTrustDecision{
        package_name: pkg.package_name,
        package_version: pkg.package_version,
        allowed: false,
        reason: ScriptTrustReason.BlockedUntrustedThirdParty,
        script_count: script_count,
    };
}

/// Evaluates trust policy across a batch of packages into caller-owned `out_decisions`.
pub fn evaluateBatchScriptTrust(
    pkgs: []const ScriptPackageHooks,
    script_counts: []const u32,
    policy: ScriptTrustPolicy,
    out_decisions: []ScriptTrustDecision,
): ScriptTrustSummary! {
    if (pkgs.len != script_counts.len || pkgs.len > out_decisions.len) {
        return err.SliceCapacityExceeded;
    }

    let sum = ScriptTrustSummary{};
    let i: usize = 0;
    while (i < pkgs.len) {
        const d = evaluateScriptTrust(pkgs[i], script_counts[i], policy);
        out_decisions[i] = d;
        sum.total_evaluated += 1;
        switch (d.reason) {
            .SkippedNoScripts => { sum.skipped_no_scripts_count += 1; },
            .AllowedRoot, .AllowedWorkspace, .AllowedTrustedDependency, .AllowedGlobalTrustAll => {
                sum.allowed_count += 1;
            },
            .BlockedIgnoreScripts => { sum.blocked_ignored_count += 1; },
            .BlockedDeniedPackage => { sum.blocked_denied_count += 1; },
            .BlockedUntrustedThirdParty => { sum.blocked_untrusted_count += 1; },
        }
        i += 1;
    }
    return sum;
}

/// Formats a Bun/cap-style actionable report for blocked untrusted postinstall scripts.
/// Returns 0 if no untrusted scripts were blocked.
pub fn formatUntrustedScriptsReport(
    decisions: []const ScriptTrustDecision,
    out: []u8,
): usize! {
    let blocked_pkgs: u32 = 0;
    let blocked_scripts: u32 = 0;
    let i: usize = 0;
    while (i < decisions.len) {
        if (decisions[i].reason == ScriptTrustReason.BlockedUntrustedThirdParty) {
            blocked_pkgs += 1;
            blocked_scripts += decisions[i].script_count;
        }
        i += 1;
    }
    if (blocked_pkgs == 0) return 0;

    let pos: usize = 0;
    srAppendBytes(out, &pos, "Blocked ")?;
    srAppendU64(out, &pos, @as(u64, blocked_scripts))?;
    srAppendBytes(out, &pos, " postinstall script(s) from ")?;
    srAppendU64(out, &pos, @as(u64, blocked_pkgs))?;
    srAppendBytes(out, &pos, " untrusted package(s):\n")?;

    let j: usize = 0;
    while (j < decisions.len) {
        if (decisions[j].reason == ScriptTrustReason.BlockedUntrustedThirdParty) {
            srAppendBytes(out, &pos, "  - ")?;
            srAppendBytes(out, &pos, decisions[j].package_name)?;
            if (decisions[j].package_version.len > 0) {
                srAppendByte(out, &pos, '@')?;
                srAppendBytes(out, &pos, decisions[j].package_version)?;
            }
            srAppendBytes(out, &pos, " (")?;
            srAppendU64(out, &pos, @as(u64, decisions[j].script_count))?;
            srAppendBytes(out, &pos, " script(s))\n")?;
        }
        j += 1;
    }

    srAppendBytes(out, &pos, "Run `cap pm untrusted` to inspect, or `cap pm trust")?;
    let k: usize = 0;
    let listed: u32 = 0;
    while (k < decisions.len && listed < 4) {
        if (decisions[k].reason == ScriptTrustReason.BlockedUntrustedThirdParty) {
            srAppendByte(out, &pos, ' ')?;
            srAppendBytes(out, &pos, decisions[k].package_name)?;
            listed += 1;
        }
        k += 1;
    }
    if (blocked_pkgs > 4) {
        srAppendBytes(out, &pos, " --all")?;
    }
    srAppendBytes(out, &pos, "` to allow them.\n")?;
    return pos;
}

// ============================================================================
// Section 3: Dependency-Aware Concurrent Script DAG Scheduler
// ============================================================================

/// Execution lifecycle state of a single script task in `PkgScriptScheduler`.
pub enum ScriptTaskStatus: u8 {
    Pending = 0,
    Ready = 1,
    Running = 2,
    Succeeded = 3,
    Failed = 4,
    TimedOut = 5,
    SkippedDependencyFailed = 6,
    Cancelled = 7,
    BlockedUntrusted = 8,

    fn isTerminal(): bool {
        return switch (_) {
            .Succeeded, .Failed, .TimedOut, .SkippedDependencyFailed, .Cancelled, .BlockedUntrusted => true,
            else => false,
        };
    }

    fn asTag(): []const u8 {
        return switch (_) {
            .Pending => "PENDING",
            .Ready => "READY",
            .Running => "RUNNING",
            .Succeeded => "OK",
            .Failed => "FAIL",
            .TimedOut => "TIMEOUT",
            .SkippedDependencyFailed => "SKIP",
            .Cancelled => "CANCEL",
            .BlockedUntrusted => "UNTRUSTED",
        };
    }
}

/// Failure handling strategy when a required script fails or times out.
pub enum ScriptFailurePolicy: u8 {
    FailFast = 0,
    ContinueOnError = 1,
}

/// Directed dependency edge: `dependency_index` must succeed before `dependent_index` starts.
pub struct ScriptDepEdge {
    dependency_index: u32 = 0,
    dependent_index: u32 = 0,

    static fn empty(): ScriptDepEdge {
        return ScriptDepEdge{};
    }
}

/// Script task node stored in caller-owned slice backing `PkgScriptScheduler`.
pub struct ScriptTaskNode {
    task_id: u32 = 0,
    package_name: []const u8 = "",
    package_version: []const u8 = "",
    script_name: []const u8 = "",
    command: []const u8 = "",
    working_dir: []const u8 = "",
    stage: ScriptHookStage = ScriptHookStage.Custom,
    is_optional: bool = false,
    is_workspace: bool = false,
    timeout_ms: u64 = 0,
    priority: u16 = 0,

    // Scheduler-managed runtime state
    status: ScriptTaskStatus = ScriptTaskStatus.Pending,
    in_degree: u32 = 0,
    started_at_ms: u64 = 0,
    finished_at_ms: u64 = 0,
    exit_code: i32 = 0,
    output_lines: u32 = 0,

    static fn empty(): ScriptTaskNode {
        return ScriptTaskNode{};
    }

    fn durationMs(): u64 {
        if (_.finished_at_ms >= _.started_at_ms) {
            return _.finished_at_ms - _.started_at_ms;
        }
        return 0;
    }
}

/// Dispatched task payload returned by `PkgScriptScheduler.dispatchReadyBatch`.
pub struct ScriptDispatchItem {
    task_index: u32 = 0,
    task_id: u32 = 0,
    package_name: []const u8 = "",
    script_name: []const u8 = "",
    command: []const u8 = "",
    working_dir: []const u8 = "",
    effective_timeout_ms: u64 = 0,

    static fn empty(): ScriptDispatchItem {
        return ScriptDispatchItem{};
    }
}

/// Aggregate execution statistics and exit code computed by `PkgScriptScheduler.stats`.
pub struct ScriptSchedulerStats {
    total_tasks: u32 = 0,
    pending: u32 = 0,
    ready: u32 = 0,
    running: u32 = 0,
    succeeded: u32 = 0,
    failed: u32 = 0,
    timed_out: u32 = 0,
    skipped_dep_failed: u32 = 0,
    cancelled: u32 = 0,
    blocked_untrusted: u32 = 0,
    optional_failed: u32 = 0,
    aggregate_exit_code: i32 = 0,
    total_duration_ms: u64 = 0,
    max_task_duration_ms: u64 = 0,
}

/// Zero-allocation, topology-aware concurrent script scheduler for Monorepos and installs.
pub struct PkgScriptScheduler {
    nodes: []ScriptTaskNode,
    edges: []ScriptDepEdge,
    node_count: usize = 0,
    edge_count: usize = 0,
    max_concurrency: u32 = SCRIPT_RUNNER_DEFAULT_CONCURRENCY,
    default_timeout_ms: u64 = SCRIPT_RUNNER_DEFAULT_TIMEOUT_MS,
    failure_policy: ScriptFailurePolicy = ScriptFailurePolicy.FailFast,
    aborted: bool = false,
    first_failure_exit_code: i32 = 0,
    graph_finalized: bool = false,

    static fn init(
        nodes: []ScriptTaskNode,
        edges: []ScriptDepEdge,
        max_concurrency: u32,
        default_timeout_ms: u64,
        failure_policy: ScriptFailurePolicy,
    ): PkgScriptScheduler {
        const conc = if (max_concurrency == 0) @as(u32, 1) else max_concurrency;
        return PkgScriptScheduler{
            nodes: nodes,
            edges: edges,
            max_concurrency: conc,
            default_timeout_ms: default_timeout_ms,
            failure_policy: failure_policy,
        };
    }

    /// Registers a new script task node and returns its 0-based task index.
    fn addTask(*_, node: ScriptTaskNode): u32! {
        if (_.graph_finalized) return err.GraphAlreadyFinalized;
        if (_.node_count >= _.nodes.len) return err.SliceCapacityExceeded;

        // Enforce unique task_id
        let i: usize = 0;
        while (i < _.node_count) {
            if (_.nodes[i].task_id == node.task_id) return err.DuplicateTaskId;
            i += 1;
        }

        const idx: u32 = @as(u32, @intCast(_.node_count));
        _.nodes[_.node_count] = node;
        _.nodes[_.node_count].status = ScriptTaskStatus.Pending;
        _.nodes[_.node_count].in_degree = 0;
        _.nodes[_.node_count].started_at_ms = 0;
        _.nodes[_.node_count].finished_at_ms = 0;
        _.nodes[_.node_count].exit_code = 0;
        _.nodes[_.node_count].output_lines = 0;
        _.node_count += 1;
        return idx;
    }

    /// Marks a registered task as blocked by zero-trust policy before graph finalization.
    fn markTaskBlockedUntrusted(*_, task_idx: u32)! {
        if (_.graph_finalized) return err.GraphAlreadyFinalized;
        if (@as(usize, task_idx) >= _.node_count) return err.InvalidTaskIndex;
        _.nodes[@as(usize, task_idx)].status = ScriptTaskStatus.BlockedUntrusted;
    }

    /// Adds a dependency edge: `dependency_idx` must complete before `dependent_idx`.
    fn addDependency(*_, dependency_idx: u32, dependent_idx: u32)! {
        if (_.graph_finalized) return err.GraphAlreadyFinalized;
        if (@as(usize, dependency_idx) >= _.node_count || @as(usize, dependent_idx) >= _.node_count) {
            return err.InvalidDependencyEdge;
        }
        if (dependency_idx == dependent_idx) {
            return err.DependencyCycleDetected;
        }

        // Deduplicate existing edges
        let i: usize = 0;
        while (i < _.edge_count) {
            if (_.edges[i].dependency_index == dependency_idx && _.edges[i].dependent_index == dependent_idx) {
                return;
            }
            i += 1;
        }

        if (_.edge_count >= _.edges.len) return err.SliceCapacityExceeded;
        _.edges[_.edge_count] = ScriptDepEdge{
            dependency_index: dependency_idx,
            dependent_index: dependent_idx,
        };
        _.edge_count += 1;
    }

    /// Validates the dependency DAG via Kahn's algorithm (zero-alloc using caller scratch slices),
    /// computes live in-degrees, and promotes all in-degree-0 tasks to `Ready`.
    fn finalizeGraph(*_, scratch_in_deg: []u32, scratch_queue: []u32)! {
        if (_.graph_finalized) return err.GraphAlreadyFinalized;
        if (scratch_in_deg.len < _.node_count || scratch_queue.len < _.node_count) {
            return err.SliceCapacityExceeded;
        }

        let i: usize = 0;
        while (i < _.node_count) {
            scratch_in_deg[i] = 0;
            _.nodes[i].in_degree = 0;
            i += 1;
        }

        let e: usize = 0;
        while (e < _.edge_count) {
            const dep = @as(usize, _.edges[e].dependency_index);
            const target = @as(usize, _.edges[e].dependent_index);
            scratch_in_deg[target] += 1;
            // Only non-blocked dependencies block runtime execution
            if (_.nodes[dep].status != ScriptTaskStatus.BlockedUntrusted) {
                _.nodes[target].in_degree += 1;
            }
            e += 1;
        }

        // Kahn topological sort cycle check across all edges
        let q_head: usize = 0;
        let q_tail: usize = 0;
        let k: usize = 0;
        while (k < _.node_count) {
            if (scratch_in_deg[k] == 0) {
                scratch_queue[q_tail] = @as(u32, @intCast(k));
                q_tail += 1;
            }
            k += 1;
        }

        let visited: usize = 0;
        while (q_head < q_tail) {
            const u = scratch_queue[q_head];
            q_head += 1;
            visited += 1;

            let ei: usize = 0;
            while (ei < _.edge_count) {
                if (_.edges[ei].dependency_index == u) {
                    const v = @as(usize, _.edges[ei].dependent_index);
                    if (scratch_in_deg[v] > 0) {
                        scratch_in_deg[v] -= 1;
                        if (scratch_in_deg[v] == 0) {
                            scratch_queue[q_tail] = @as(u32, @intCast(v));
                            q_tail += 1;
                        }
                    }
                }
                ei += 1;
            }
        }

        if (visited != _.node_count) {
            return err.DependencyCycleDetected;
        }

        // Promote initial in-degree-0 pending tasks to Ready
        let m: usize = 0;
        while (m < _.node_count) {
            if (_.nodes[m].status == ScriptTaskStatus.Pending && _.nodes[m].in_degree == 0) {
                _.nodes[m].status = ScriptTaskStatus.Ready;
            }
            m += 1;
        }

        _.graph_finalized = true;
    }

    fn runningCount(): u32 {
        let count: u32 = 0;
        let i: usize = 0;
        while (i < _.node_count) {
            if (_.nodes[i].status == ScriptTaskStatus.Running) count += 1;
            i += 1;
        }
        return count;
    }

    /// Dispatches up to `min(available_slots, out_batch.len)` ready tasks in priority order.
    fn dispatchReadyBatch(*_, now_ms: u64, out_batch: []ScriptDispatchItem): usize! {
        if (!_.graph_finalized) return err.GraphNotFinalized;
        if (_.aborted) return 0;

        const active = _.runningCount();
        if (active >= _.max_concurrency) return 0;

        const max_to_take = @as(usize, _.max_concurrency - active);
        const limit = if (max_to_take < out_batch.len) max_to_take else out_batch.len;
        let dispatched: usize = 0;

        while (dispatched < limit) {
            // Select highest-priority Ready task (tie-break by lowest index)
            let best_idx: usize = _.node_count;
            let best_prio: u16 = 0;
            let i: usize = 0;
            while (i < _.node_count) {
                if (_.nodes[i].status == ScriptTaskStatus.Ready) {
                    if (best_idx == _.node_count || _.nodes[i].priority > best_prio) {
                        best_idx = i;
                        best_prio = _.nodes[i].priority;
                    }
                }
                i += 1;
            }

            if (best_idx == _.node_count) break;

            _.nodes[best_idx].status = ScriptTaskStatus.Running;
            _.nodes[best_idx].started_at_ms = now_ms;
            const eff_timeout = if (_.nodes[best_idx].timeout_ms > 0)
                _.nodes[best_idx].timeout_ms
            else
                _.default_timeout_ms;

            out_batch[dispatched] = ScriptDispatchItem{
                task_index: @as(u32, @intCast(best_idx)),
                task_id: _.nodes[best_idx].task_id,
                package_name: _.nodes[best_idx].package_name,
                script_name: _.nodes[best_idx].script_name,
                command: _.nodes[best_idx].command,
                working_dir: _.nodes[best_idx].working_dir,
                effective_timeout_ms: eff_timeout,
            };
            dispatched += 1;
        }

        return dispatched;
    }

    fn propagateDownstreamSkip(*_, failed_idx: u32) {
        // Iterative fixed-point propagation without extra memory
        let changed = true;
        while (changed) {
            changed = false;
            let e: usize = 0;
            while (e < _.edge_count) {
                const src = @as(usize, _.edges[e].dependency_index);
                const dst = @as(usize, _.edges[e].dependent_index);
                const src_st = _.nodes[src].status;
                const src_failed = (src_st == ScriptTaskStatus.Failed ||
                    src_st == ScriptTaskStatus.TimedOut ||
                    src_st == ScriptTaskStatus.SkippedDependencyFailed) &&
                    !_.nodes[src].is_optional;
                const src_is_trigger = (_.edges[e].dependency_index == failed_idx) || src_failed;
                if (src_is_trigger && src_failed) {
                    if (_.nodes[dst].status == ScriptTaskStatus.Pending || _.nodes[dst].status == ScriptTaskStatus.Ready) {
                        _.nodes[dst].status = ScriptTaskStatus.SkippedDependencyFailed;
                        changed = true;
                    }
                }
                e += 1;
            }
        }
    }

    fn cancelAllUnstarted(*_) {
        let i: usize = 0;
        while (i < _.node_count) {
            if (_.nodes[i].status == ScriptTaskStatus.Pending || _.nodes[i].status == ScriptTaskStatus.Ready) {
                _.nodes[i].status = ScriptTaskStatus.Cancelled;
            }
            i += 1;
        }
    }

    fn releaseDependents(*_, completed_idx: u32) {
        if (_.aborted) return;
        let e: usize = 0;
        while (e < _.edge_count) {
            if (_.edges[e].dependency_index == completed_idx) {
                const dst = @as(usize, _.edges[e].dependent_index);
                if (_.nodes[dst].status == ScriptTaskStatus.Pending) {
                    if (_.nodes[dst].in_degree > 0) {
                        _.nodes[dst].in_degree -= 1;
                    }
                    if (_.nodes[dst].in_degree == 0) {
                        _.nodes[dst].status = ScriptTaskStatus.Ready;
                    }
                }
            }
            e += 1;
        }
    }

    /// Records completion of a running task and advances or halts the dependency graph.
    fn completeTask(*_, task_idx: u32, exit_code: i32, now_ms: u64, output_lines: u32)! {
        if (!_.graph_finalized) return err.GraphNotFinalized;
        if (@as(usize, task_idx) >= _.node_count) return err.InvalidTaskIndex;

        const idx = @as(usize, task_idx);
        if (_.nodes[idx].status != ScriptTaskStatus.Running) {
            return err.InvalidStateTransition;
        }

        _.nodes[idx].finished_at_ms = now_ms;
        _.nodes[idx].exit_code = exit_code;
        _.nodes[idx].output_lines = output_lines;

        if (exit_code == 0) {
            _.nodes[idx].status = ScriptTaskStatus.Succeeded;
            _.releaseDependents(task_idx);
            return;
        }

        _.nodes[idx].status = ScriptTaskStatus.Failed;
        if (_.nodes[idx].is_optional) {
            // Optional package script failure degrades gracefully without blocking dependents
            _.releaseDependents(task_idx);
            return;
        }

        if (_.first_failure_exit_code == 0) {
            _.first_failure_exit_code = exit_code;
        }

        _.propagateDownstreamSkip(task_idx);
        if (_.failure_policy == ScriptFailurePolicy.FailFast) {
            _.aborted = true;
            _.cancelAllUnstarted();
        }
    }

    /// Scans all `Running` tasks and transitions any whose elapsed time meets or exceeds
    /// their effective timeout into `TimedOut` (exit code 124).
    fn checkTimeouts(*_, now_ms: u64, out_timed_out: []u32): usize! {
        if (!_.graph_finalized) return err.GraphNotFinalized;
        let count: usize = 0;

        let i: usize = 0;
        while (i < _.node_count) {
            if (_.nodes[i].status == ScriptTaskStatus.Running) {
                const eff_timeout = if (_.nodes[i].timeout_ms > 0)
                    _.nodes[i].timeout_ms
                else
                    _.default_timeout_ms;
                if (eff_timeout > 0 && now_ms >= _.nodes[i].started_at_ms && (now_ms - _.nodes[i].started_at_ms) >= eff_timeout) {
                    _.nodes[i].status = ScriptTaskStatus.TimedOut;
                    _.nodes[i].finished_at_ms = now_ms;
                    _.nodes[i].exit_code = SCRIPT_RUNNER_TIMEOUT_EXIT_CODE;

                    if (count < out_timed_out.len) {
                        out_timed_out[count] = @as(u32, @intCast(i));
                    }
                    count += 1;

                    const idx_u32 = @as(u32, @intCast(i));
                    if (_.nodes[i].is_optional) {
                        _.releaseDependents(idx_u32);
                    } else {
                        if (_.first_failure_exit_code == 0) {
                            _.first_failure_exit_code = SCRIPT_RUNNER_TIMEOUT_EXIT_CODE;
                        }
                        _.propagateDownstreamSkip(idx_u32);
                        if (_.failure_policy == ScriptFailurePolicy.FailFast) {
                            _.aborted = true;
                            _.cancelAllUnstarted();
                        }
                    }
                }
            }
            i += 1;
        }

        return count;
    }

    /// Returns true when no tasks remain in `Pending`, `Ready`, or `Running` state.
    fn isComplete(): bool {
        let i: usize = 0;
        while (i < _.node_count) {
            if (!_.nodes[i].status.isTerminal()) return false;
            i += 1;
        }
        return true;
    }

    /// Computes live scheduler statistics and aggregate exit code.
    fn stats(): ScriptSchedulerStats {
        let st = ScriptSchedulerStats{
            total_tasks: @as(u32, @intCast(_.node_count)),
            aggregate_exit_code: _.first_failure_exit_code,
        };
        let i: usize = 0;
        while (i < _.node_count) {
            const node = _.nodes[i];
            switch (node.status) {
                .Pending => { st.pending += 1; },
                .Ready => { st.ready += 1; },
                .Running => { st.running += 1; },
                .Succeeded => { st.succeeded += 1; },
                .Failed => {
                    if (node.is_optional) {
                        st.optional_failed += 1;
                    } else {
                        st.failed += 1;
                    }
                },
                .TimedOut => {
                    if (node.is_optional) {
                        st.optional_failed += 1;
                    } else {
                        st.timed_out += 1;
                    }
                },
                .SkippedDependencyFailed => { st.skipped_dep_failed += 1; },
                .Cancelled => { st.cancelled += 1; },
                .BlockedUntrusted => { st.blocked_untrusted += 1; },
            }

            if (node.status == ScriptTaskStatus.Succeeded ||
                node.status == ScriptTaskStatus.Failed ||
                node.status == ScriptTaskStatus.TimedOut) {
                const dur = node.durationMs();
                st.total_duration_ms += dur;
                if (dur > st.max_task_duration_ms) {
                    st.max_task_duration_ms = dur;
                }
            }
            i += 1;
        }
        return st;
    }
}

// ============================================================================
// Section 4: Environment Variable Injector & Sensitive Key Filter
// ============================================================================

/// Single environment variable key/value pair.
pub struct ScriptEnvEntry {
    key: []const u8 = "",
    value: []const u8 = "",

    static fn empty(): ScriptEnvEntry {
        return ScriptEnvEntry{};
    }
}

/// Configuration for `buildScriptEnvVars`.
pub struct ScriptEnvOptions {
    package_name: []const u8 = "",
    package_version: []const u8 = "",
    package_json_path: []const u8 = "",
    lifecycle_event: []const u8 = "",
    lifecycle_script: []const u8 = "",
    init_cwd: []const u8 = "",
    cap_exec_path: []const u8 = "",
    local_bin_dir: []const u8 = "",
    workspace_root_bin_dir: []const u8 = "",
    path_separator: u8 = ':',
    filter_sensitive_keys: bool = true,
    preserve_existing_path: bool = true,

    static fn defaults(): ScriptEnvOptions {
        return ScriptEnvOptions{};
    }
}

/// Result metrics returned by `buildScriptEnvVars`.
pub struct ScriptEnvBuildResult {
    entry_count: usize = 0,
    filtered_sensitive_count: u32 = 0,
    injected_standard_count: u32 = 0,
    path_bytes_written: usize = 0,
}

/// Returns true if an environment variable key matches common credential/secret patterns
/// that should be stripped before invoking untrusted or third-party lifecycle scripts.
pub fn isSensitiveEnvKey(key: []const u8): bool {
    if (key.len == 0) return false;
    // Exact well-known secret keys
    if (srBytesEqlIgnoreCase(key, "NPM_TOKEN")) return true;
    if (srBytesEqlIgnoreCase(key, "NODE_AUTH_TOKEN")) return true;
    if (srBytesEqlIgnoreCase(key, "CAP_AUTH_TOKEN")) return true;
    if (srBytesEqlIgnoreCase(key, "GITHUB_TOKEN")) return true;
    if (srBytesEqlIgnoreCase(key, "GH_TOKEN")) return true;
    if (srBytesEqlIgnoreCase(key, "DATABASE_URL")) return true;
    if (srBytesEqlIgnoreCase(key, "AWS_SECRET_ACCESS_KEY")) return true;
    if (srBytesEqlIgnoreCase(key, "AWS_SESSION_TOKEN")) return true;
    if (srBytesEqlIgnoreCase(key, "OPENAI_API_KEY")) return true;
    if (srBytesEqlIgnoreCase(key, "ANTHROPIC_API_KEY")) return true;

    // Substring patterns for generic secrets
    if (srContainsIgnoreCase(key, "_TOKEN")) return true;
    if (srContainsIgnoreCase(key, "_SECRET")) return true;
    if (srContainsIgnoreCase(key, "_PASSWORD")) return true;
    if (srContainsIgnoreCase(key, "_API_KEY")) return true;
    if (srContainsIgnoreCase(key, "_PRIVATE_KEY")) return true;
    if (srContainsIgnoreCase(key, "AUTH_HEADER")) return true;
    return false;
}

fn isManagedScriptEnvKey(key: []const u8): bool {
    if (srBytesEqlIgnoreCase(key, "PATH")) return true;
    if (srBytesEql(key, "npm_lifecycle_event")) return true;
    if (srBytesEql(key, "npm_lifecycle_script")) return true;
    if (srBytesEql(key, "npm_package_name")) return true;
    if (srBytesEql(key, "npm_package_version")) return true;
    if (srBytesEql(key, "npm_package_json")) return true;
    if (srBytesEql(key, "npm_execpath")) return true;
    if (srBytesEql(key, "CAP_EXEC_PATH")) return true;
    if (srBytesEql(key, "INIT_CWD")) return true;
    return false;
}

fn pushEnvEntry(out: []ScriptEnvEntry, count: *usize, key: []const u8, value: []const u8)! {
    if (count.* >= out.len) return err.SliceCapacityExceeded;
    out[count.*] = ScriptEnvEntry{ key: key, value: value };
    count.* += 1;
}

/// Builds a sanitized, zero-allocation environment table for script execution:
///   - Filters sensitive credential keys when `opts.filter_sensitive_keys` is true;
///   - Prepends `local_bin_dir` (and optional `workspace_root_bin_dir`) to `PATH`
///     into `path_buf` with deduplication;
///   - Injects `npm_lifecycle_event`, `npm_lifecycle_script`, `npm_package_name`,
///     `npm_package_version`, `npm_package_json`, `INIT_CWD`, `CAP_EXEC_PATH`,
///     and `npm_execpath`.
pub fn buildScriptEnvVars(
    inherited: []const ScriptEnvEntry,
    opts: ScriptEnvOptions,
    out_entries: []ScriptEnvEntry,
    path_buf: []u8,
): ScriptEnvBuildResult! {
    let res = ScriptEnvBuildResult{};
    let existing_path: []const u8 = "";

    // 1. Copy non-managed, non-sensitive inherited variables and capture PATH
    let i: usize = 0;
    while (i < inherited.len) {
        const entry = inherited[i];
        if (srBytesEqlIgnoreCase(entry.key, "PATH")) {
            existing_path = entry.value;
            i += 1;
            continue;
        }
        if (opts.filter_sensitive_keys && isSensitiveEnvKey(entry.key)) {
            res.filtered_sensitive_count += 1;
            i += 1;
            continue;
        }
        if (isManagedScriptEnvKey(entry.key)) {
            i += 1;
            continue;
        }
        pushEnvEntry(out_entries, &res.entry_count, entry.key, entry.value)?;
        i += 1;
    }

    // 2. Construct PATH in caller-owned `path_buf`
    let path_pos: usize = 0;
    if (opts.local_bin_dir.len > 0) {
        srAppendBytes(path_buf, &path_pos, opts.local_bin_dir)?;
    }
    if (opts.workspace_root_bin_dir.len > 0 && !srBytesEql(opts.workspace_root_bin_dir, opts.local_bin_dir)) {
        if (path_pos > 0) srAppendByte(path_buf, &path_pos, opts.path_separator)?;
        srAppendBytes(path_buf, &path_pos, opts.workspace_root_bin_dir)?;
    }
    if (opts.preserve_existing_path && existing_path.len > 0) {
        if (path_pos > 0) srAppendByte(path_buf, &path_pos, opts.path_separator)?;
        srAppendBytes(path_buf, &path_pos, existing_path)?;
    }
    if (path_pos > 0) {
        pushEnvEntry(out_entries, &res.entry_count, "PATH", path_buf[0..path_pos])?;
        res.path_bytes_written = path_pos;
        res.injected_standard_count += 1;
    }

    // 3. Inject standard lifecycle & cap metadata environment variables
    if (opts.lifecycle_event.len > 0) {
        pushEnvEntry(out_entries, &res.entry_count, "npm_lifecycle_event", opts.lifecycle_event)?;
        res.injected_standard_count += 1;
    }
    if (opts.lifecycle_script.len > 0) {
        pushEnvEntry(out_entries, &res.entry_count, "npm_lifecycle_script", opts.lifecycle_script)?;
        res.injected_standard_count += 1;
    }
    if (opts.package_name.len > 0) {
        pushEnvEntry(out_entries, &res.entry_count, "npm_package_name", opts.package_name)?;
        res.injected_standard_count += 1;
    }
    if (opts.package_version.len > 0) {
        pushEnvEntry(out_entries, &res.entry_count, "npm_package_version", opts.package_version)?;
        res.injected_standard_count += 1;
    }
    if (opts.package_json_path.len > 0) {
        pushEnvEntry(out_entries, &res.entry_count, "npm_package_json", opts.package_json_path)?;
        res.injected_standard_count += 1;
    }
    if (opts.init_cwd.len > 0) {
        pushEnvEntry(out_entries, &res.entry_count, "INIT_CWD", opts.init_cwd)?;
        res.injected_standard_count += 1;
    }
    if (opts.cap_exec_path.len > 0) {
        pushEnvEntry(out_entries, &res.entry_count, "CAP_EXEC_PATH", opts.cap_exec_path)?;
        pushEnvEntry(out_entries, &res.entry_count, "npm_execpath", opts.cap_exec_path)?;
        res.injected_standard_count += 2;
    }

    return res;
}

// ============================================================================
// Section 5: Prefixed Line Buffer & Execution Summary Formatter
// ============================================================================

/// ANSI foreground colors rotated deterministically across parallel workspace scripts.
pub enum ScriptAnsiColor: u8 {
    None = 0,
    Cyan = 1,
    Green = 2,
    Yellow = 3,
    Magenta = 4,
    Blue = 5,
    Red = 6,

    fn ansiCode(): []const u8 {
        return switch (_) {
            .None => "",
            .Cyan => "\x1b[36m",
            .Green => "\x1b[32m",
            .Yellow => "\x1b[33m",
            .Magenta => "\x1b[35m",
            .Blue => "\x1b[34m",
            .Red => "\x1b[31m",
        };
    }

    static fn pickForIndex(idx: u32): ScriptAnsiColor {
        return switch (idx % 5) {
            0 => ScriptAnsiColor.Cyan,
            1 => ScriptAnsiColor.Green,
            2 => ScriptAnsiColor.Yellow,
            3 => ScriptAnsiColor.Magenta,
            else => ScriptAnsiColor.Blue,
        };
    }
}

/// Formatting options for `ScriptLineBuffer`.
pub struct ScriptLinePrefixConfig {
    use_color: bool = true,
    include_script_name: bool = true,
    pad_width: usize = 0,

    static fn defaults(): ScriptLinePrefixConfig {
        return ScriptLinePrefixConfig{};
    }
}

/// Zero-allocation chunk-to-line accumulator that prefixes every complete line
/// with `[<pkg>:<script>] ` (optionally ANSI-colored and column-padded).
pub struct ScriptLineBuffer {
    pending: []u8,
    pending_len: usize = 0,
    pkg_label: []const u8 = "",
    script_label: []const u8 = "",
    color: ScriptAnsiColor = ScriptAnsiColor.Cyan,
    config: ScriptLinePrefixConfig = ScriptLinePrefixConfig{},
    lines_flushed: u32 = 0,
    truncated_chunks: u32 = 0,

    static fn init(
        pending_buf: []u8,
        pkg_label: []const u8,
        script_label: []const u8,
        color: ScriptAnsiColor,
        config: ScriptLinePrefixConfig,
    ): ScriptLineBuffer {
        return ScriptLineBuffer{
            pending: pending_buf,
            pkg_label: pkg_label,
            script_label: script_label,
            color: color,
            config: config,
        };
    }

    fn emitPrefixedLine(*_, line: []const u8, out: []u8, out_pos: *usize)! {
        const use_ansi = _.config.use_color && _.color != ScriptAnsiColor.None;
        if (use_ansi) {
            srAppendBytes(out, out_pos, _.color.ansiCode())?;
        }
        srAppendByte(out, out_pos, '[')?;
        srAppendBytes(out, out_pos, _.pkg_label)?;
        let raw_label_len = _.pkg_label.len;
        if (_.config.include_script_name && _.script_label.len > 0) {
            srAppendByte(out, out_pos, ':')?;
            srAppendBytes(out, out_pos, _.script_label)?;
            raw_label_len += 1 + _.script_label.len;
        }
        srAppendByte(out, out_pos, ']')?;
        if (use_ansi) {
            srAppendBytes(out, out_pos, "\x1b[0m")?;
        }

        // Optional column padding after closing bracket
        if (_.config.pad_width > raw_label_len) {
            let pad = _.config.pad_width - raw_label_len;
            while (pad > 0) {
                srAppendByte(out, out_pos, ' ')?;
                pad -= 1;
            }
        }
        srAppendByte(out, out_pos, ' ')?;

        // Strip single trailing CR for CRLF normalization
        let effective_len = line.len;
        if (effective_len > 0 && line[effective_len - 1] == '\r') {
            effective_len -= 1;
        }
        srAppendBytes(out, out_pos, line[0..effective_len])?;
        srAppendByte(out, out_pos, '\n')?;
        _.lines_flushed += 1;
    }

    /// Feeds an arbitrary stdout/stderr byte chunk; emits every complete line into `out`
    /// and retains any trailing partial line in `pending`.
    fn pushChunk(*_, chunk: []const u8, out: []u8, out_pos: *usize): u32! {
        let emitted: u32 = 0;
        let i: usize = 0;

        while (i < chunk.len) {
            const b = chunk[i];
            if (b == '\n') {
                _.emitPrefixedLine(_.pending[0.._.pending_len], out, out_pos)?;
                _.pending_len = 0;
                emitted += 1;
            } else {
                if (_.pending_len >= _.pending.len) {
                    // Flush overflowing partial line cleanly so no bytes are silently dropped
                    _.emitPrefixedLine(_.pending[0.._.pending_len], out, out_pos)?;
                    _.pending_len = 0;
                    _.truncated_chunks += 1;
                    emitted += 1;
                }
                if (_.pending.len > 0) {
                    _.pending[_.pending_len] = b;
                    _.pending_len += 1;
                }
            }
            i += 1;
        }

        return emitted;
    }

    /// Flushes any remaining partial line at EOF. Returns true if a line was emitted.
    fn flush(*_, out: []u8, out_pos: *usize): bool! {
        if (_.pending_len == 0) return false;
        _.emitPrefixedLine(_.pending[0.._.pending_len], out, out_pos)?;
        _.pending_len = 0;
        return true;
    }
}

/// Formats a complete execution timing & status summary report for a script run.
pub fn formatScriptRunSummary(
    nodes: []const ScriptTaskNode,
    stats: ScriptSchedulerStats,
    use_color: bool,
    out: []u8,
): usize! {
    let pos: usize = 0;

    if (use_color) {
        if (stats.aggregate_exit_code == 0) {
            srAppendBytes(out, &pos, "\x1b[32m")?;
        } else {
            srAppendBytes(out, &pos, "\x1b[31m")?;
        }
    }
    srAppendBytes(out, &pos, "=== Script Run Summary: ")?;
    if (stats.aggregate_exit_code == 0) {
        srAppendBytes(out, &pos, "PASS")?;
    } else {
        srAppendBytes(out, &pos, "FAIL (exit ")?;
        srAppendI32(out, &pos, stats.aggregate_exit_code)?;
        srAppendByte(out, &pos, ')')?;
    }
    srAppendBytes(out, &pos, " ===")?;
    if (use_color) {
        srAppendBytes(out, &pos, "\x1b[0m")?;
    }
    srAppendByte(out, &pos, '\n')?;

    // Aggregate counts line
    srAppendBytes(out, &pos, "Tasks: ")?;
    srAppendU64(out, &pos, @as(u64, stats.total_tasks))?;
    srAppendBytes(out, &pos, " total, ")?;
    srAppendU64(out, &pos, @as(u64, stats.succeeded))?;
    srAppendBytes(out, &pos, " passed, ")?;
    srAppendU64(out, &pos, @as(u64, stats.failed))?;
    srAppendBytes(out, &pos, " failed")?;
    if (stats.timed_out > 0) {
        srAppendBytes(out, &pos, ", ")?;
        srAppendU64(out, &pos, @as(u64, stats.timed_out))?;
        srAppendBytes(out, &pos, " timed out")?;
    }
    if (stats.skipped_dep_failed > 0) {
        srAppendBytes(out, &pos, ", ")?;
        srAppendU64(out, &pos, @as(u64, stats.skipped_dep_failed))?;
        srAppendBytes(out, &pos, " skipped")?;
    }
    if (stats.cancelled > 0) {
        srAppendBytes(out, &pos, ", ")?;
        srAppendU64(out, &pos, @as(u64, stats.cancelled))?;
        srAppendBytes(out, &pos, " cancelled")?;
    }
    if (stats.blocked_untrusted > 0) {
        srAppendBytes(out, &pos, ", ")?;
        srAppendU64(out, &pos, @as(u64, stats.blocked_untrusted))?;
        srAppendBytes(out, &pos, " untrusted")?;
    }
    if (stats.optional_failed > 0) {
        srAppendBytes(out, &pos, ", ")?;
        srAppendU64(out, &pos, @as(u64, stats.optional_failed))?;
        srAppendBytes(out, &pos, " optional-failed")?;
    }
    srAppendByte(out, &pos, '\n')?;

    // Per-task detail lines
    let i: usize = 0;
    while (i < nodes.len) {
        const n = nodes[i];
        srAppendBytes(out, &pos, "  [")?;
        if (use_color) {
            const color_code = switch (n.status) {
                .Succeeded => "\x1b[32m",
                .Failed, .TimedOut => "\x1b[31m",
                .SkippedDependencyFailed, .Cancelled, .BlockedUntrusted => "\x1b[33m",
                else => "\x1b[36m",
            };
            srAppendBytes(out, &pos, color_code)?;
        }
        srAppendBytes(out, &pos, n.status.asTag())?;
        if (use_color) {
            srAppendBytes(out, &pos, "\x1b[0m")?;
        }
        srAppendBytes(out, &pos, "] ")?;
        srAppendBytes(out, &pos, n.package_name)?;
        if (n.script_name.len > 0) {
            srAppendByte(out, &pos, ':')?;
            srAppendBytes(out, &pos, n.script_name)?;
        }
        if (n.status == ScriptTaskStatus.Succeeded ||
            n.status == ScriptTaskStatus.Failed ||
            n.status == ScriptTaskStatus.TimedOut) {
            srAppendBytes(out, &pos, " (")?;
            formatScriptDurationMs(n.durationMs(), out, &pos)?;
            if (n.exit_code != 0) {
                srAppendBytes(out, &pos, ", exit ")?;
                srAppendI32(out, &pos, n.exit_code)?;
            }
            srAppendByte(out, &pos, ')')?;
        }
        if (n.is_optional) {
            srAppendBytes(out, &pos, " [optional]")?;
        }
        srAppendByte(out, &pos, '\n')?;
        i += 1;
    }

    // Timing footer
    srAppendBytes(out, &pos, "Cumulative time: ")?;
    formatScriptDurationMs(stats.total_duration_ms, out, &pos)?;
    srAppendBytes(out, &pos, " (slowest: ")?;
    formatScriptDurationMs(stats.max_task_duration_ms, out, &pos)?;
    srAppendBytes(out, &pos, ")\n")?;

    return pos;
}
