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
// ============================================================================

fn gsToLower(b: u8): u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn gsByteEq(a: u8, b: u8, nocase: bool): bool {
    if (nocase) {
        return gsToLower(a) == gsToLower(b);
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
    while (i < prefix.len) {
        if (!gsByteEq(path[i], prefix[i], nocase)) {
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
            base_buf: [[0; 64]; 32],
            base_lens: [0; 32],
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
        let hits: [32]usize = [0; 32];
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
            negated: [false; 32],
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
        let hits: [32]usize = [0; 32];
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
