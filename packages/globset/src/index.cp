// @cap/globset — Zero-allocation multi-glob set matcher & override engine for CP
// Clean-room MIT implementation (Rust `globset` v0.4 & `ignore::overrides` parity).
// Composes `@cap/picomatch` (`PicoOptions`, `isMatchWithOptions`, `scanBase`)
// and `@cap/normalize-path` (`normalizePath`) with zero duplication.

import {
    PicoOptions,
    isMatchWithOptions,
    scanBase
} from "../../picomatch/src/index.cp";
import { normalizePath } from "../../normalize-path/src/index.cp";

pub const GLOBSET_MAX_PATTERNS: usize = 32;
pub const OVERRIDE_NONE: i32 = 0;
pub const OVERRIDE_WHITELIST: i32 = 1;
pub const OVERRIDE_IGNORE: i32 = 2;

const GS_MAX_PATH_BUF: usize = 1024;
const GS_MAX_BASE_PREFIX: usize = 64;

/// Configuration options for `GlobSet` and `GlobOverride`.
pub struct GlobSetOptions {
    pub case_insensitive: bool,
    pub literal_separator: bool,
    pub backslash_escape: bool,

    pub static fn defaults(): GlobSetOptions {
        return GlobSetOptions {
            case_insensitive: false,
            literal_separator: true,
            backslash_escape: true
        };
    }
}

// ============================================================================
// Private helpers (`gs` prefix to avoid module-flattening symbol collisions)
// Accelerated with 16-lane @Vector(16, u8) SIMD prefix matching and comptime 256-entry LUT.
// ============================================================================

type GsVec16 = @Vector(16, u8);

fn gsBuildLowerLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = undefined;
    let c: usize = 0;
    while (c < 256) {
        lut[c] = if (c >= 65 && c <= 90) (c + 32) as u8 else c as u8;
        c += 1;
    }
    return lut;
}

const GS_LOWER_LUT: [256]u8 = gsBuildLowerLut();

fn gsToLower(b: u8): u8 {
    return GS_LOWER_LUT[b as usize];
}

fn gsByteEq(a: u8, b: u8, nocase: bool): bool {
    if (nocase) {
        return GS_LOWER_LUT[a as usize] == GS_LOWER_LUT[b as usize];
    }
    return a == b;
}

fn gsHasPrefix(path: string, prefix: []const u8, nocase: bool): bool {
    if (prefix.len == 0) {
        return true;
    }
    if (path.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    if (!nocase) {
        while (i + 16 <= prefix.len) {
            const va: GsVec16 = path[i .. i + 16][0..16].*;
            const vb: GsVec16 = prefix[i .. i + 16][0..16].*;
            if (@reduce(.Or, va != vb)) {
                return false;
            }
            i += 16;
        }
        while (i < prefix.len) {
            if (path[i] != prefix[i]) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    const v_zero: GsVec16 = @splat(0 as u8);
    const v_one: GsVec16 = @splat(1 as u8);
    const v_up_a: GsVec16 = @splat(65 as u8);
    const v_up_z: GsVec16 = @splat(90 as u8);
    const v_diff: GsVec16 = @splat(32 as u8);
    while (i + 16 <= prefix.len) {
        const va: GsVec16 = path[i .. i + 16][0..16].*;
        const vb: GsVec16 = prefix[i .. i + 16][0..16].*;
        const ma = @select(u8, va >= v_up_a, v_one, v_zero) & @select(u8, va <= v_up_z, v_one, v_zero);
        const mb = @select(u8, vb >= v_up_a, v_one, v_zero) & @select(u8, vb <= v_up_z, v_one, v_zero);
        const la: GsVec16 = va + @select(u8, ma != v_zero, v_diff, v_zero);
        const lb: GsVec16 = vb + @select(u8, mb != v_zero, v_diff, v_zero);
        if (@reduce(.Or, la != lb)) {
            return false;
        }
        i += 16;
    }
    while (i < prefix.len) {
        if (GS_LOWER_LUT[path[i] as usize] != GS_LOWER_LUT[prefix[i] as usize]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn gsToPicoOptions(opts: GlobSetOptions): PicoOptions {
    let p = PicoOptions.defaults();
    p.nocase = opts.case_insensitive;
    p.dot = true;
    p.noglobstar = false;
    return p;
}

fn gsMatchOne(normPath: string, pat: string, opts: GlobSetOptions, picoOpts: PicoOptions): bool {
    if (!opts.backslash_escape && pat.len <= GS_MAX_PATH_BUF) {
        let patBuf: [1024]u8 = undefined;
        let pLen = normalizePath(pat, false, patBuf[0..1024]);
        return isMatchWithOptions(normPath, patBuf[0..pLen], picoOpts);
    }
    return isMatchWithOptions(normPath, pat, picoOpts);
}

// ============================================================================
// Public Struct — GlobSet (fixed-capacity multi-pattern matcher, max 32 globs)
// ============================================================================

/// Zero-allocation collection of up to 32 compiled glob patterns with static-base prefiltering.
pub struct GlobSet {
    patterns: [32]string,
    base_buf: [32][64]u8,
    base_lens: [32]usize,
    count: usize,
    opts: GlobSetOptions,

    pub static fn new(): GlobSet {
        return GlobSet.withOptions(GlobSetOptions.defaults());
    }

    pub static fn withOptions(opts: GlobSetOptions): GlobSet {
        return GlobSet {
            patterns: [""; 32],
            base_buf: undefined,
            base_lens: undefined,
            count: 0,
            opts: opts
        };
    }

    /// Adds a glob pattern to the set. Returns `false` if empty or capacity (32) is reached.
    pub fn add(*_, pattern: string): bool {
        if (pattern.len == 0 || _.count >= GLOBSET_MAX_PATTERNS) {
            return false;
        }
        let idx = _.count;
        _.patterns[idx] = pattern;
        // Pre-scan static base prefix via @cap/picomatch::scanBase for fast pruning
        let bLen = scanBase(pattern, _.base_buf[idx][0..GS_MAX_BASE_PREFIX]);
        _.base_lens[idx] = bLen;
        _.count = idx + 1;
        return true;
    }

    pub fn len(): usize {
        return _.count;
    }

    pub fn isEmpty(): bool {
        return _.count == 0;
    }

    pub fn clear(*_) {
        _.count = 0;
    }

    pub fn patternAt(idx: usize): string {
        if (idx >= _.count) {
            return "";
        }
        return _.patterns[idx];
    }

    /// Returns `true` if `path` matches at least one pattern in the set.
    pub fn isMatch(path: string): bool {
        return _.firstMatch(path) >= 0;
    }

    /// Returns the 0-based index of the first matching pattern, or `-1` if none match.
    pub fn firstMatch(path: string): i32 {
        if (_.count == 0) {
            return -1;
        }
        let pathBuf: [1024]u8 = undefined;
        let normPath = path;
        if (path.len > 0 && path.len <= GS_MAX_PATH_BUF) {
            let nLen = normalizePath(path, false, pathBuf[0..1024]);
            normPath = pathBuf[0..nLen];
        }
        let picoOpts = gsToPicoOptions(_.opts);
        let i: usize = 0;
        while (i < _.count) {
            let bLen = _.base_lens[i];
            if (bLen == 0 || gsHasPrefix(normPath, _.base_buf[i][0..bLen], _.opts.case_insensitive)) {
                if (gsMatchOne(normPath, _.patterns[i], _.opts, picoOpts)) {
                    return i as i32;
                }
            }
            i = i + 1;
        }
        return -1;
    }

    /// Writes the 0-based indices of ALL patterns matching `path` into `out_indices`.
    /// Returns the number of indices written (stops cleanly at `out_indices.len`).
    pub fn matches(path: string, out_indices: []usize): usize {
        if (_.count == 0 || out_indices.len == 0) {
            return 0;
        }
        let pathBuf: [1024]u8 = undefined;
        let normPath = path;
        if (path.len > 0 && path.len <= GS_MAX_PATH_BUF) {
            let nLen = normalizePath(path, false, pathBuf[0..1024]);
            normPath = pathBuf[0..nLen];
        }
        let picoOpts = gsToPicoOptions(_.opts);
        let found: usize = 0;
        let i: usize = 0;
        while (i < _.count && found < out_indices.len) {
            let bLen = _.base_lens[i];
            if (bLen == 0 || gsHasPrefix(normPath, _.base_buf[i][0..bLen], _.opts.case_insensitive)) {
                if (gsMatchOne(normPath, _.patterns[i], _.opts, picoOpts)) {
                    out_indices[found] = i;
                    found = found + 1;
                }
            }
            i = i + 1;
        }
        return found;
    }

    /// Counts how many patterns in the set match `path`.
    pub fn matchCount(path: string): usize {
        if (_.count == 0) {
            return 0;
        }
        let hits: [32]usize = undefined;
        return _.matches(path, hits[0..32]);
    }
}

// ============================================================================
// Public Struct — GlobOverride (positive whitelist & `!` negative ignore rules)
// ============================================================================

/// Zero-allocation whitelist/blacklist override matcher (parity with `ignore::overrides::Override`).
/// Positive rules whitelist matching paths; rules prefixed with `!` ignore matching paths.
/// Later matching rules take precedence over earlier rules (last-match-wins).
pub struct GlobOverride {
    set: GlobSet,
    negated: [32]bool,
    has_whitelist: bool,

    pub static fn new(): GlobOverride {
        return GlobOverride.withOptions(GlobSetOptions.defaults());
    }

    pub static fn withOptions(opts: GlobSetOptions): GlobOverride {
        return GlobOverride {
            set: GlobSet.withOptions(opts),
            negated: undefined,
            has_whitelist: false
        };
    }

    /// Adds a whitelist (`pattern`) or ignore (`!pattern`) rule.
    pub fn add(*_, rule: string): bool {
        if (rule.len == 0 || _.set.len() >= GLOBSET_MAX_PATTERNS) {
            return false;
        }
        let isNeg = false;
        let pat = rule;
        if (rule[0] == 0x21) { // '!'
            isNeg = true;
            pat = rule[1..rule.len];
            if (pat.len == 0) {
                return false;
            }
        }
        let idx = _.set.len();
        if (!_.set.add(pat)) {
            return false;
        }
        _.negated[idx] = isNeg;
        if (!isNeg) {
            _.has_whitelist = true;
        }
        return true;
    }

    pub fn len(): usize {
        return _.set.len();
    }

    pub fn isEmpty(): bool {
        return _.set.isEmpty();
    }

    pub fn clear(*_) {
        _.set.clear();
        _.has_whitelist = false;
    }

    /// Evaluates override rules against `path`:
    /// Returns `OVERRIDE_WHITELIST`, `OVERRIDE_IGNORE`, or `OVERRIDE_NONE`.
    pub fn matched(path: string): i32 {
        let hits: [32]usize = undefined;
        let n = _.set.matches(path, hits[0..32]);
        if (n == 0) {
            return OVERRIDE_NONE;
        }
        let lastIdx = hits[n - 1];
        if (_.negated[lastIdx]) {
            return OVERRIDE_IGNORE;
        }
        return OVERRIDE_WHITELIST;
    }

    /// Returns `true` if `path` is permitted under the configured override rules.
    /// If any whitelist rule exists, unmatched paths are rejected; if only ignore rules exist,
    /// unmatched paths are allowed.
    pub fn isAllowed(path: string): bool {
        let res = _.matched(path);
        if (res == OVERRIDE_IGNORE) {
            return false;
        }
        if (res == OVERRIDE_WHITELIST) {
            return true;
        }
        return !_.has_whitelist;
    }
}

export {
    GLOBSET_MAX_PATTERNS,
    OVERRIDE_NONE,
    OVERRIDE_WHITELIST,
    OVERRIDE_IGNORE,
    GlobSetOptions,
    GlobSet,
    GlobOverride
};
