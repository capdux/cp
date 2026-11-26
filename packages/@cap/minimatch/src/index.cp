// @cap/minimatch — Glob matching utility compatible with npm `minimatch` (400M+/wk)
// Clean-room MIT CP implementation, zero-heap, reuses `@cap/micromatch` & `@cap/picomatch`.

import { mmIsMatch, mmFilter, mmNot } from "../../micromatch/src/index.cp";
import { PicoOptions } from "../../picomatch/src/index.cp";

pub struct MinimatchOptions {
    pub dot: bool,
    pub nocase: bool,
    pub match_base: bool,
    pub noglobstar: bool,
    pub nonegate: bool,
    pub nocomment: bool,
    pub flip_negate: bool,

    pub static fn defaults(): MinimatchOptions {
        return MinimatchOptions {
            dot: false,
            nocase: false,
            match_base: false,
            noglobstar: false,
            nonegate: false,
            nocomment: false,
            flip_negate: false,
        };
    }
}

fn miniToPico(opts: MinimatchOptions): PicoOptions {
    return PicoOptions {
        dot: opts.dot,
        nocase: opts.nocase,
        basename: opts.match_base,
        noglobstar: opts.noglobstar,
    };
}

/// Returns true if `pattern` is a comment line (`#...`) when `nocomment` is false.
pub fn isCommentPattern(pattern: string, opts: MinimatchOptions): bool {
    return !opts.nocomment && pattern.len > 0 && pattern[0] == 35; // '#'
}

/// Returns true if `pattern` is a negated glob (`!...`) when `nonegate` is false.
pub fn isNegatedPattern(pattern: string, opts: MinimatchOptions): bool {
    return !opts.nonegate && pattern.len > 0 && pattern[0] == 33; // '!'
}

/// Tests a `path` against a `pattern` using `minimatch` semantics (supports `#` comments,
/// `!` negation, `flip_negate`, `match_base`, `dot`, `nocase`, `noglobstar`, `{a,b}`, `{1..5}`, `**`).
pub fn minimatchWithOptions(path: string, pattern: string, opts: MinimatchOptions): bool {
    if (pattern.len == 0) { return path.len == 0; }
    if (isCommentPattern(pattern, opts)) { return false; }

    let pico = miniToPico(opts);
    if (opts.nonegate && pattern[0] == 33) {
        // Treat leading '!' as literal when nonegate is enabled
        if (path.len == 0 || path[0] != 33) { return false; }
        return mmIsMatch(path[1..path.len], pattern[1..pattern.len], pico);
    }

    let negated = isNegatedPattern(pattern, opts);
    let matched = mmIsMatch(path, pattern, pico);
    if (negated && opts.flip_negate) {
        return !matched;
    }
    return matched;
}

/// Default `minimatch(path, pattern)` with standard options.
pub fn minimatch(path: string, pattern: string): bool {
    return minimatchWithOptions(path, pattern, MinimatchOptions.defaults());
}

/// Filters `paths` matching `pattern`, writing matching indices into `out_indices`.
pub fn minimatchList(
    paths: []const string,
    pattern: string,
    opts: MinimatchOptions,
    out_indices: []usize
): usize {
    if (isCommentPattern(pattern, opts)) { return 0; }
    let count: usize = 0;
    let i: usize = 0;
    while (i < paths.len) {
        if (minimatchWithOptions(paths[i], pattern, opts)) {
            if (count < out_indices.len) {
                out_indices[count] = i;
            }
            count = count + 1;
        }
        i = i + 1;
    }
    if (count > out_indices.len) { return out_indices.len; }
    return count;
}

/// Batch filter against multiple patterns via `@cap/micromatch::mmFilter`.
pub fn minimatchFilterAll(
    paths: []const string,
    patterns: []const string,
    opts: MinimatchOptions,
    out_indices: []usize
): usize {
    return mmFilter(paths, patterns, miniToPico(opts), out_indices);
}

/// Batch inverse filter via `@cap/micromatch::mmNot`.
pub fn minimatchRejectAll(
    paths: []const string,
    patterns: []const string,
    opts: MinimatchOptions,
    out_indices: []usize
): usize {
    return mmNot(paths, patterns, miniToPico(opts), out_indices);
}
