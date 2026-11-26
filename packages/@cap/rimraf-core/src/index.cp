// packages/@cap/rimraf-core/src/index.cp
// Zero-allocation recursive delete safety validator and bottom-up planner for CP.
// Clean-room MIT pure CP implementation inspired by npm `rimraf` v6, reusing `@cap/fast-glob`.

import {
    FastGlobOptions,
    GlobPatternSet,
} from "../../fast-glob/src/index.cp";

pub const RIMRAF_MAX_PLAN: usize = 32;

/// Configuration options controlling recursive deletion safety and retry policy.
pub struct RimrafOptions {
    pub preserve_root: bool,
    pub max_retries: u8,
    pub retry_delay_ms: u32,
    pub is_windows: bool,

    pub static fn defaults(): RimrafOptions {
        return RimrafOptions {
            preserve_root: true,
            max_retries: 3,
            retry_delay_ms: 100,
            is_windows: false,
        };
    }
}

/// Zero-heap bottom-up deletion plan holding up to 32 target paths ordered deepest-first.
pub struct RimrafPlan {
    pub valid: bool,
    pub paths: [32]string,
    pub count: usize,
}

fn rmfIsAsciiAlpha(b: u8): bool {
    return (b >= 65 && b <= 90) || (b >= 97 && b <= 122);
}

fn rmfIsSep(b: u8, is_windows: bool): bool {
    if (b == 47) { // '/'
        return true;
    }
    return is_windows && b == 92; // '\'
}

fn rmfHasNul(s: string): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 0) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn rmfPathDepth(path: string, is_windows: bool): usize {
    let depth: usize = 0;
    let i: usize = 0;
    let in_seg: bool = false;
    while (i < path.len) {
        if (rmfIsSep(path[i], is_windows)) {
            in_seg = false;
        } else {
            if (!in_seg) {
                depth += 1;
                in_seg = true;
            }
        }
        i += 1;
    }
    return depth;
}

/// Returns `true` if `path` refers to a dangerous filesystem root (such as `/`, `//`,
/// empty string, or Windows drive roots like `C:\`, `C:/`, `C:`).
pub fn isRootPath(path: string, is_windows: bool): bool {
    if (path.len == 0) {
        return true;
    }

    // All separators: "/", "//", "\", "\\" (on Windows)
    let all_seps: bool = true;
    let i: usize = 0;
    while (i < path.len) {
        if (!rmfIsSep(path[i], is_windows)) {
            all_seps = false;
            break;
        }
        i += 1;
    }
    if (all_seps) {
        return true;
    }

    // Windows drive root: "C:", "C:\", "C:/", "C:\\"
    if (is_windows && path.len >= 2 && rmfIsAsciiAlpha(path[0]) && path[1] == 58) { // ':'
        let j: usize = 2;
        let rest_seps: bool = true;
        while (j < path.len) {
            if (!rmfIsSep(path[j], true)) {
                rest_seps = false;
                break;
            }
            j += 1;
        }
        if (rest_seps) {
            return true;
        }
    }

    return false;
}

/// Validates whether `target` is safe to delete under `opts`.
/// Rejects empty strings, paths containing NUL bytes, and (when `preserve_root` is true) root paths.
pub fn validateRimrafTarget(target: string, opts: RimrafOptions): bool {
    if (target.len == 0 || rmfHasNul(target)) {
        return false;
    }
    if (opts.preserve_root && isRootPath(target, opts.is_windows)) {
        return false;
    }
    return true;
}

fn rmfSortByDepthDesc(paths: []string, count: usize, is_windows: bool) {
    let i: usize = 1;
    while (i < count) {
        let key_path: string = paths[i];
        let key_depth: usize = rmfPathDepth(key_path, is_windows);
        let j: usize = i;
        while (j > 0) {
            let prev_depth: usize = rmfPathDepth(paths[j - 1], is_windows);
            if (prev_depth < key_depth) {
                paths[j] = paths[j - 1];
                j -= 1;
            } else {
                break;
            }
        }
        paths[j] = key_path;
        i += 1;
    }
}

/// Builds a zero-heap bottom-up deletion plan from `entries`, skipping unsafe/root targets
/// and sorting remaining paths in descending order of directory depth so children precede parents.
pub fn planBottomUpDelete(entries: []string, opts: RimrafOptions): RimrafPlan {
    let out_paths: [32]string = [""; 32];
    let count: usize = 0;
    let i: usize = 0;
    while (i < entries.len && count < RIMRAF_MAX_PLAN) {
        if (validateRimrafTarget(entries[i], opts)) {
            out_paths[count] = entries[i];
            count += 1;
        }
        i += 1;
    }

    if (count > 1) {
        rmfSortByDepthDesc(out_paths[0..32], count, opts.is_windows);
    }

    let is_valid: bool = entries.len == 0 || count > 0;
    return RimrafPlan {
        valid: is_valid,
        paths: out_paths,
        count: count,
    };
}

/// Filters `entries` using `@cap/fast-glob` (`GlobPatternSet`) against `pattern`, skips unsafe
/// targets, and returns a bottom-up deletion plan sorted deepest-first.
pub fn planFilteredDelete(entries: []string, pattern: string, opts: RimrafOptions): RimrafPlan {
    let out_paths: [32]string = [""; 32];
    if (pattern.len == 0) {
        return RimrafPlan {
            valid: false,
            paths: out_paths,
            count: 0,
        };
    }

    let fg_opts: FastGlobOptions = FastGlobOptions.defaults();
    fg_opts.only_files = false;
    fg_opts.only_directories = false;
    fg_opts.dot = true;
    fg_opts.case_sensitive = !opts.is_windows;

    let pat_set: GlobPatternSet = GlobPatternSet.new(fg_opts);
    if (!pat_set.addPattern(pattern)) {
        return RimrafPlan {
            valid: false,
            paths: out_paths,
            count: 0,
        };
    }

    let count: usize = 0;
    let i: usize = 0;
    while (i < entries.len && count < RIMRAF_MAX_PLAN) {
        let entry: string = entries[i];
        if (validateRimrafTarget(entry, opts) && pat_set.matches(entry, false)) {
            out_paths[count] = entry;
            count += 1;
        }
        i += 1;
    }

    if (count > 1) {
        rmfSortByDepthDesc(out_paths[0..32], count, opts.is_windows);
    }

    return RimrafPlan {
        valid: true,
        paths: out_paths,
        count: count,
    };
}
