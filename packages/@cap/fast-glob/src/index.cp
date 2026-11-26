// @cap/fast-glob — Pure CP zero-allocation glob pattern set & path matcher.
// Clean-room MIT implementation inspired by npm `fast-glob` v3.
// Reuses `@cap/glob-match` for wildcard & brace matching.

import { globMatch, isStaticGlob } from "../../glob-match/src/index.cp";

pub const FGL_MAX_PATTERNS: usize = 16;
const FGL_CASE_BUF_LEN: usize = 512;

fn fglToLower(b: u8): u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn fglIsGlobMeta(b: u8): bool {
    return b == 42 || b == 63 || b == 91 || b == 93 ||
        b == 123 || b == 125 || b == 40 || b == 41 ||
        b == 33 || b == 64 || b == 43;
}

fn fglNormalizeRelPath(raw: string): string {
    let start: usize = 0;
    let end: usize = raw.len;
    while (start + 1 < end && raw[start] == 46 && raw[start + 1] == 47) { // "./"
        start = start + 2;
    }
    while (end > start && raw[end - 1] == 47) { // trailing '/'
        end = end - 1;
    }
    return raw[start..end];
}

fn fglPathDepth(path: string): usize {
    if (path.len == 0) {
        return 0;
    }
    let depth: usize = 1;
    let i: usize = 0;
    while (i < path.len) {
        if (path[i] == 47) { // '/'
            depth = depth + 1;
        }
        i = i + 1;
    }
    return depth;
}

fn fglHasDotSegment(path: string): bool {
    let i: usize = 0;
    let seg_start = true;
    while (i < path.len) {
        let b = path[i];
        if (seg_start && b == 46) { // '.'
            return true;
        }
        seg_start = (b == 47);
        i = i + 1;
    }
    return false;
}

fn fglLowerInto(src: string, dst: []u8): usize {
    let n = src.len;
    if (n > dst.len) {
        n = dst.len;
    }
    let i: usize = 0;
    while (i < n) {
        dst[i] = fglToLower(src[i]);
        i = i + 1;
    }
    return n;
}

fn fglMatchOne(pattern: string, path: string, case_sensitive: bool): bool {
    if (case_sensitive) {
        return globMatch(pattern, path);
    }
    let pbuf: [512]u8 = [0 as u8; 512];
    let sbuf: [512]u8 = [0 as u8; 512];
    let plen = fglLowerInto(pattern, pbuf[0..FGL_CASE_BUF_LEN]);
    let slen = fglLowerInto(path, sbuf[0..FGL_CASE_BUF_LEN]);
    let pstr: string = pbuf[0..plen];
    let sstr: string = sbuf[0..slen];
    return globMatch(pstr, sstr);
}

/// Options controlling path filtering and matching (inspired by `fast-glob` v3).
pub struct FastGlobOptions {
    pub dot: bool,
    pub deep: usize,
    pub only_files: bool,
    pub only_directories: bool,
    pub case_sensitive: bool,

    pub static fn defaults(): FastGlobOptions {
        return FastGlobOptions {
            dot: false,
            deep: 0,
            only_files: true,
            only_directories: false,
            case_sensitive: true
        };
    }
}

/// Returns `true` if `pattern` contains dynamic glob syntax (`*`, `?`, `[`, `{`, `!`, etc.).
pub fn isDynamicPattern(pattern: string): bool {
    if (pattern.len == 0) {
        return false;
    }
    if (pattern[0] == 33) { // leading '!' negation
        return true;
    }
    if (!isStaticGlob(pattern)) {
        return true;
    }
    let i: usize = 0;
    while (i < pattern.len) {
        let b = pattern[i];
        if (b == 92) { // '\'
            i = i + 2;
            continue;
        }
        if (b == 42 || b == 63 || b == 91 || b == 123 || b == 33) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

/// Escapes glob special characters in `path` (equivalent to `fast-glob.escapePath`).
/// Writes into `out` and returns the number of bytes written.
pub fn escapePath(path: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < path.len && pos < out.len) {
        let b = path[i];
        if (fglIsGlobMeta(b) || b == 92) {
            if (pos + 2 > out.len) {
                break;
            }
            out[pos] = 92; // '\'
            out[pos + 1] = b;
            pos = pos + 2;
        } else {
            out[pos] = b;
            pos = pos + 1;
        }
        i = i + 1;
    }
    return pos;
}

/// Converts a filesystem path into a POSIX glob pattern by normalizing `\` to `/`
/// and escaping glob special characters (equivalent to `fast-glob.convertPathToPattern`).
/// Writes into `out` and returns the number of bytes written.
pub fn convertPathToPattern(path: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < path.len && pos < out.len) {
        let b = path[i];
        if (b == 92) { // '\' -> '/'
            out[pos] = 47;
            pos = pos + 1;
        } else if (fglIsGlobMeta(b)) {
            if (pos + 2 > out.len) {
                break;
            }
            out[pos] = 92; // '\'
            out[pos + 1] = b;
            pos = pos + 2;
        } else {
            out[pos] = b;
            pos = pos + 1;
        }
        i = i + 1;
    }
    return pos;
}

/// Zero-heap fixed-capacity glob pattern set with positive include patterns and negative ignore patterns.
pub struct GlobPatternSet {
    pub opts: FastGlobOptions,
    pub includes: [16]string,
    pub include_count: usize,
    pub ignores: [16]string,
    pub ignore_count: usize,

    pub static fn new(opts: FastGlobOptions): GlobPatternSet {
        let inc_arr: [16]string = [""; 16];
        let ign_arr: [16]string = [""; 16];
        return GlobPatternSet {
            opts: opts,
            includes: inc_arr,
            include_count: 0,
            ignores: ign_arr,
            ignore_count: 0
        };
    }

    /// Adds a pattern to the set. Leading `!` automatically registers it as an ignore pattern.
    /// Returns `false` if empty or the corresponding 16-slot table is full.
    pub fn addPattern(*_, raw: string): bool {
        if (raw.len == 0) {
            return false;
        }
        if (raw[0] == 33) { // '!'
            if (raw.len == 1) {
                return false;
            }
            return _.addIgnore(raw[1..raw.len]);
        }
        if (_.include_count >= FGL_MAX_PATTERNS) {
            return false;
        }
        _.includes[_.include_count] = raw;
        _.include_count = _.include_count + 1;
        return true;
    }

    /// Explicitly adds an ignore pattern (without leading `!`).
    pub fn addIgnore(*_, pat: string): bool {
        if (pat.len == 0 || _.ignore_count >= FGL_MAX_PATTERNS) {
            return false;
        }
        _.ignores[_.ignore_count] = pat;
        _.ignore_count = _.ignore_count + 1;
        return true;
    }

    /// Evaluates whether `rel_path` matches the pattern set under `_.opts`.
    pub fn matches(rel_path: string, is_dir: bool): bool {
        let norm = fglNormalizeRelPath(rel_path);
        if (norm.len == 0) {
            return false;
        }

        if (_.opts.only_directories) {
            if (!is_dir) {
                return false;
            }
        } else if (_.opts.only_files) {
            if (is_dir) {
                return false;
            }
        }

        if (_.opts.deep > 0 && fglPathDepth(norm) > _.opts.deep) {
            return false;
        }

        if (!_.opts.dot && fglHasDotSegment(norm)) {
            return false;
        }

        // Must match at least one positive include pattern.
        let matched = false;
        let i: usize = 0;
        while (i < _.include_count) {
            if (fglMatchOne(_.includes[i], norm, _.opts.case_sensitive)) {
                matched = true;
                break;
            }
            i = i + 1;
        }
        if (!matched) {
            return false;
        }

        // Must not match any ignore pattern.
        let j: usize = 0;
        while (j < _.ignore_count) {
            if (fglMatchOne(_.ignores[j], norm, _.opts.case_sensitive)) {
                return false;
            }
            j = j + 1;
        }

        return true;
    }
}
