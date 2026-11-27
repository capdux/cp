// @cap/fast-glob — Pure CP zero-allocation glob pattern set & path matcher.
// Clean-room MIT implementation inspired by npm `fast-glob` v3.
// Reuses `@cap/glob-match` for wildcard & brace matching.
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning, comptime 256-entry LUTs, @memcpy, and undefined stack buffers.

import { globMatch, isStaticGlob } from "../../glob-match/src/index.cp";

pub const FGL_MAX_PATTERNS: usize = 16;
const FGL_CASE_BUF_LEN: usize = 512;

type FglVec16 = @Vector(16, u8);

const FGL_CLS_META: u8 = 1;
const FGL_CLS_BS: u8 = 2;
const FGL_CLS_DYNAMIC: u8 = 4;

fn fglBuildLowerLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = undefined;
    let c: usize = 0;
    while (c < 256) {
        if (c >= 65 && c <= 90) {
            lut[c] = (c + 32) as u8;
        } else {
            lut[c] = c as u8;
        }
        c += 1;
    }
    return lut;
}

fn fglBuildMetaLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut[42] = FGL_CLS_META | FGL_CLS_DYNAMIC;  // '*'
    lut[63] = FGL_CLS_META | FGL_CLS_DYNAMIC;  // '?'
    lut[91] = FGL_CLS_META | FGL_CLS_DYNAMIC;  // '['
    lut[93] = FGL_CLS_META;                    // ']'
    lut[123] = FGL_CLS_META | FGL_CLS_DYNAMIC; // '{'
    lut[125] = FGL_CLS_META;                   // '}'
    lut[40] = FGL_CLS_META;                    // '('
    lut[41] = FGL_CLS_META;                    // ')'
    lut[33] = FGL_CLS_META | FGL_CLS_DYNAMIC;  // '!'
    lut[64] = FGL_CLS_META;                    // '@'
    lut[43] = FGL_CLS_META;                    // '+'
    lut[92] = FGL_CLS_BS;                      // '\'
    return lut;
}

const FGL_LOWER_LUT: [256]u8 = fglBuildLowerLut();
const FGL_META_LUT: [256]u8 = fglBuildMetaLut();

fn fglToLower(b: u8): u8 {
    return FGL_LOWER_LUT[b as usize];
}

fn fglIsGlobMeta(b: u8): bool {
    return (FGL_META_LUT[b as usize] & FGL_CLS_META) != 0;
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
    const len = path.len;
    const v_slash: FglVec16 = @splat(47);
    while (i + 16 <= len) {
        const v: FglVec16 = path[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_slash)) {
            let k: usize = 0;
            while (k < 16) {
                if (path[i + k] == 47) {
                    depth = depth + 1;
                }
                k = k + 1;
            }
        }
        i = i + 16;
    }
    while (i < len) {
        if (path[i] == 47) { // '/'
            depth = depth + 1;
        }
        i = i + 1;
    }
    return depth;
}

fn fglHasDotSegment(path: string): bool {
    let i: usize = 0;
    const len = path.len;
    let seg_start = true;
    const v_slash: FglVec16 = @splat(47);
    const v_dot: FglVec16 = @splat(46);
    while (i < len) {
        if (!seg_start && i + 16 <= len) {
            const v: FglVec16 = path[i .. i + 16][0..16].*;
            if (!@reduce(.Or, v == v_slash) && !@reduce(.Or, v == v_dot)) {
                i = i + 16;
                continue;
            }
        }
        const b = path[i];
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
    const v_a: FglVec16 = @splat(65);
    while (i + 16 <= n) {
        const v: FglVec16 = src[i .. i + 16][0..16].*;
        const off: FglVec16 = v -% v_a;
        if (@reduce(.Min, off) > 25) {
            @memcpy(dst[i .. i + 16], src[i .. i + 16]);
        } else {
            const arr: [16]u8 = v;
            let k: usize = 0;
            while (k < 16) {
                dst[i + k] = FGL_LOWER_LUT[arr[k] as usize];
                k = k + 1;
            }
        }
        i = i + 16;
    }
    while (i < n) {
        dst[i] = FGL_LOWER_LUT[src[i] as usize];
        i = i + 1;
    }
    return n;
}

fn fglMatchOne(pattern: string, path: string, case_sensitive: bool): bool {
    if (case_sensitive) {
        return globMatch(pattern, path);
    }
    let pbuf: [512]u8 = undefined;
    let sbuf: [512]u8 = undefined;
    const plen = fglLowerInto(pattern, pbuf[0..FGL_CASE_BUF_LEN]);
    const slen = fglLowerInto(path, sbuf[0..FGL_CASE_BUF_LEN]);
    const pstr: string = pbuf[0..plen];
    const sstr: string = sbuf[0..slen];
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
        const cls = FGL_META_LUT[pattern[i] as usize];
        if ((cls & FGL_CLS_BS) != 0) { // '\'
            i = i + 2;
            continue;
        }
        if ((cls & FGL_CLS_DYNAMIC) != 0) {
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
    const len = path.len;
    while (i < len && pos < out.len) {
        const run_start = i;
        while (i < len && (FGL_META_LUT[path[i] as usize] & (FGL_CLS_META | FGL_CLS_BS)) == 0) {
            i = i + 1;
        }
        if (i > run_start) {
            const avail = out.len - pos;
            const run_len = i - run_start;
            const copy_n = if (run_len < avail) { run_len } else { avail };
            @memcpy(out[pos .. pos + copy_n], path[run_start .. run_start + copy_n]);
            pos = pos + copy_n;
            if (copy_n < run_len || pos >= out.len) {
                break;
            }
        }
        if (i < len) {
            if (pos + 2 > out.len) {
                break;
            }
            out[pos] = 92; // '\'
            out[pos + 1] = path[i];
            pos = pos + 2;
            i = i + 1;
        }
    }
    return pos;
}

/// Converts a filesystem path into a POSIX glob pattern by normalizing `\` to `/`
/// and escaping glob special characters (equivalent to `fast-glob.convertPathToPattern`).
/// Writes into `out` and returns the number of bytes written.
pub fn convertPathToPattern(path: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    const len = path.len;
    while (i < len && pos < out.len) {
        const run_start = i;
        while (i < len && (FGL_META_LUT[path[i] as usize] & (FGL_CLS_META | FGL_CLS_BS)) == 0) {
            i = i + 1;
        }
        if (i > run_start) {
            const avail = out.len - pos;
            const run_len = i - run_start;
            const copy_n = if (run_len < avail) { run_len } else { avail };
            @memcpy(out[pos .. pos + copy_n], path[run_start .. run_start + copy_n]);
            pos = pos + copy_n;
            if (copy_n < run_len || pos >= out.len) {
                break;
            }
        }
        if (i < len) {
            const b = path[i];
            if (b == 92) { // '\' -> '/'
                out[pos] = 47;
                pos = pos + 1;
            } else {
                if (pos + 2 > out.len) {
                    break;
                }
                out[pos] = 92; // '\'
                out[pos + 1] = b;
                pos = pos + 2;
            }
            i = i + 1;
        }
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
        const norm = fglNormalizeRelPath(rel_path);
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
