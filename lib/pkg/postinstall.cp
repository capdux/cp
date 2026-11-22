// Source: Absorbed from /data/dev/bun/src/install/postinstall_optimizer.cp | Synced: 2026-11-22T01:28:00Z
// Pure-CP zero-allocation Postinstall Optimizer & Native Binlink Replacement Matcher
// (Absorbed from Bun src/install/postinstall_optimizer.cp).

import { PackageID } from "./types.cp";
import { OperatingSystem, Architecture, PackageMeta } from "./resolution.cp";

pub const MAX_DYNAMIC_OPTIMIZER_RULES: usize = 32;

/// Optimization strategy applied to a package's lifecycle scripts.
pub enum PostinstallMode: u8 {
    /// Replace postinstall script execution with direct native binary linking
    /// when a platform-specific optional dependency is resolved.
    native_binlink = 0,
    /// Skip postinstall lifecycle scripts unconditionally (e.g. `sharp >= 0.33.0`).
    ignore = 1,
}

/// Lightweight semantic version triple (`major.minor.patch`) for minimum-version gating.
pub struct SemverTriple {
    major: u32 = 0,
    minor: u32 = 0,
    patch: u32 = 0,

    pub static fn init(major: u32, minor: u32, patch: u32): SemverTriple {
        return SemverTriple{ major: major, minor: minor, patch: patch };
    }

    /// Parse leading `major.minor.patch` (ignores optional leading `v` and prerelease/build suffixes).
    pub static fn parse(raw: []const u8): SemverTriple? {
        if (raw.len == 0) return null;
        let s = raw;
        if (s[0] == 'v' || s[0] == 'V') {
            s = s[1..];
        }
        if (s.len == 0) return null;

        let pos: usize = 0;
        const maj = parseU32Part(s, &pos) ?? return null;
        if (pos >= s.len || s[pos] != '.') return null;
        pos += 1;
        const min = parseU32Part(s, &pos) ?? return null;
        if (pos >= s.len || s[pos] != '.') return null;
        pos += 1;
        const pat = parseU32Part(s, &pos) ?? return null;

        if (pos < s.len && s[pos] != '-' && s[pos] != '+') {
            return null;
        }
        return SemverTriple{ major: maj, minor: min, patch: pat };
    }

    /// Returns true if `this >= minimum`.
    pub fn isAtLeast(minimum: SemverTriple): bool {
        if (_.major != minimum.major) return _.major > minimum.major;
        if (_.minor != minimum.minor) return _.minor > minimum.minor;
        return _.patch >= minimum.patch;
    }
}

fn parseU32Part(s: []const u8, pos: *usize): u32? {
    const start = pos.*;
    let acc: u32 = 0;
    while (pos.* < s.len) {
        const c = s[pos.*];
        if (c < '0' || c > '9') break;
        const digit: u32 = @as(u32, c - '0');
        if (acc > 429496729 || (acc == 429496729 && digit > 5)) {
            return null;
        }
        acc = acc * 10 + digit;
        pos.* += 1;
    }
    if (pos.* == start) return null;
    return acc;
}

/// Package identity passed to postinstall optimizer evaluation.
pub struct PostinstallPkgInfo {
    name: []const u8 = "",
    version: SemverTriple? = null,
}

/// Single dynamic override rule configured via package.json (`nativeDependencies` / `ignoreScripts`).
pub struct DynamicOptimizerRule {
    name: []const u8 = "",
    mode: PostinstallMode = .native_binlink,
    active: bool = false,
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Default Bun built-in optimizer rules:
/// - `esbuild`, `@anthropic-ai/claude-code` -> `.native_binlink`
/// - `sharp` (>= 0.33.0) -> `.ignore`
fn lookupDefaultMode(pkg_info: PostinstallPkgInfo): PostinstallMode? {
    if (strEq(pkg_info.name, "esbuild") || strEq(pkg_info.name, "@anthropic-ai/claude-code")) {
        return PostinstallMode.native_binlink;
    }
    if (strEq(pkg_info.name, "sharp")) {
        if (ver := pkg_info.version) {
            const min_sharp = SemverTriple.init(0, 33, 0);
            if (!ver.isAtLeast(min_sharp)) {
                return null;
            }
        }
        return PostinstallMode.ignore;
    }
    return null;
}

/// Find a platform-specific optional dependency package ID that can replace a
/// package's postinstall script via direct native binary linking.
///
/// Mirrors Bun `PostinstallOptimizer.getNativeBinlinkReplacementPackageID`:
/// - Always returns `null` on Windows (`win32` requires `.exe` extensions).
/// - Skips out-of-bounds IDs and platform-agnostic packages (`cpu == ALL || os == ALL`).
/// - Returns the first resolution matching both `target_cpu` and `target_os`.
pub fn findNativeBinlinkReplacement(
    resolutions: []const PackageID,
    metas: []const PackageMeta,
    target_cpu: u16,
    target_os: u16,
): PackageID? {
    if (OperatingSystem.isMatch(target_os, OperatingSystem.WIN32)) {
        return null;
    }
    let i: usize = 0;
    while (i < resolutions.len) : (i += 1) {
        const res_id = resolutions[i];
        const idx: usize = @as(usize, res_id);
        if (idx >= metas.len) continue;
        const meta = metas[idx];
        if (meta.cpu == Architecture.ALL || meta.os == OperatingSystem.ALL) continue;
        if (Architecture.isMatch(meta.cpu, target_cpu) && OperatingSystem.isMatch(meta.os, target_os)) {
            return res_id;
        }
    }
    return null;
}

/// Zero-allocation postinstall optimizer policy & evaluator.
pub struct PostinstallOptimizerConfig {
    rules: [MAX_DYNAMIC_OPTIMIZER_RULES]DynamicOptimizerRule = [DynamicOptimizerRule{}; MAX_DYNAMIC_OPTIMIZER_RULES],
    rule_count: usize = 0,
    disable_default_native_binlinks: bool = false,
    disable_default_ignore: bool = false,
    disable_native_linker_flag: bool = false,
    disable_ignore_scripts_flag: bool = false,

    pub static fn init(): PostinstallOptimizerConfig {
        return PostinstallOptimizerConfig{};
    }

    /// Add or update a dynamic package optimization rule.
    pub fn addDynamicRule(*_, pkg_name: []const u8, mode: PostinstallMode): bool {
        if (pkg_name.len == 0) return false;
        let i: usize = 0;
        while (i < _.rule_count) : (i += 1) {
            if (_.rules[i].active && strEq(_.rules[i].name, pkg_name)) {
                _.rules[i].mode = mode;
                return true;
            }
        }
        if (_.rule_count >= MAX_DYNAMIC_OPTIMIZER_RULES) return false;
        _.rules[_.rule_count] = DynamicOptimizerRule{
            name: pkg_name,
            mode: mode,
            active: true,
        };
        _.rule_count += 1;
        return true;
    }

    /// Register custom `nativeDependencies` list and disable default native binlink list
    /// (matching Bun `PostinstallOptimizer.fromPackageJSON`).
    pub fn configureNativeDependencies(*_, names: []const []const u8): bool {
        _.disable_default_native_binlinks = true;
        let i: usize = 0;
        while (i < names.len) : (i += 1) {
            if (names[i].len == 0) continue;
            if (!_.addDynamicRule(names[i], PostinstallMode.native_binlink)) {
                return false;
            }
        }
        return true;
    }

    /// Register custom `ignoreScripts` list and disable default ignore list
    /// (matching Bun `PostinstallOptimizer.fromPackageJSON`).
    pub fn configureIgnoreScripts(*_, names: []const []const u8): bool {
        _.disable_default_ignore = true;
        let i: usize = 0;
        while (i < names.len) : (i += 1) {
            if (names[i].len == 0) continue;
            if (!_.addDynamicRule(names[i], PostinstallMode.ignore)) {
                return false;
            }
        }
        return true;
    }

    /// Returns whether native dependency binlinking is enabled under current policy & flags.
    pub fn isNativeBinlinkEnabled(): bool {
        if (_.disable_native_linker_flag) return false;
        return true;
    }

    /// Resolve effective optimization mode for a package (dynamic overrides take precedence over defaults).
    pub fn resolveMode(pkg_info: PostinstallPkgInfo): PostinstallMode? {
        if (pkg_info.name.len == 0) return null;
        let i: usize = 0;
        while (i < _.rule_count) : (i += 1) {
            if (_.rules[i].active && strEq(_.rules[i].name, pkg_info.name)) {
                return _.rules[i].mode;
            }
        }
        const def_mode = lookupDefaultMode(pkg_info) ?? return null;
        return switch (def_mode) {
            .native_binlink => if (!_.disable_default_native_binlinks) PostinstallMode.native_binlink else null,
            .ignore => if (!_.disable_default_ignore) PostinstallMode.ignore else null,
        };
    }

    /// Determine whether lifecycle scripts for `pkg_info` should be skipped.
    ///
    /// - Returns `false` if `disable_ignore_scripts_flag` is active.
    /// - For `.ignore`, returns `true`.
    /// - For `.native_binlink`, requires root install tree (`tree_id == null || tree_id == 0`),
    ///   native binlink enabled, and a matching platform-specific replacement package.
    pub fn shouldIgnoreLifecycleScripts(
        pkg_info: PostinstallPkgInfo,
        resolutions: []const PackageID,
        metas: []const PackageMeta,
        target_cpu: u16,
        target_os: u16,
        tree_id: u32?,
    ): bool {
        if (_.disable_ignore_scripts_flag) return false;
        const mode = _.resolveMode(pkg_info) ?? return false;
        return switch (mode) {
            .ignore => true,
            .native_binlink => {
                if (!_.isNativeBinlinkEnabled()) return false;
                if (tid := tree_id) {
                    if (tid != 0) return false;
                }
                return findNativeBinlinkReplacement(resolutions, metas, target_cpu, target_os) != null;
            },
        };
    }
}
