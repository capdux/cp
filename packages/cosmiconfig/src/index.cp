// @cap/cosmiconfig — Pure CP zero-allocation config file discovery planner & loader detector.
// Clean-room MIT implementation inspired by npm `cosmiconfig` v9.

import { strEq, eqIgnoreCase, rfindByte } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub const LOADER_UNKNOWN: u8 = 0;
pub const LOADER_JSON: u8 = 1;
pub const LOADER_YAML: u8 = 2;
pub const LOADER_JS: u8 = 3;
pub const LOADER_PKG_JSON: u8 = 4;

pub const DEFAULT_SEARCH_PLACE_COUNT: usize = 8;

fn cosmiEndsWithIgnoreCase(s: string, suffix: string): bool {
    if (suffix.len > s.len) {
        return false;
    }
    return eqIgnoreCase(s[(s.len - suffix.len)..s.len], suffix);
}

fn cosmiBasename(filepath: string): string {
    let end = filepath.len;
    while (end > 0 && (filepath[end - 1] == 47 || filepath[end - 1] == 92)) {
        end = end - 1;
    }
    let start = end;
    while (start > 0 && filepath[start - 1] != 47 && filepath[start - 1] != 92) {
        start = start - 1;
    }
    return filepath[start..end];
}

fn cosmiTrimTrailingSlash(dir: string): string {
    let end = dir.len;
    while (end > 1 && dir[end - 1] == 47) {
        end = end - 1;
    }
    return dir[0..end];
}

/// Configuration explorer options (inspired by `cosmiconfig` v9).
pub struct ExplorerOptions {
    pub module_name: string,
    pub stop_dir: string,
    pub package_prop: string,
    pub ignore_empty: bool,

    pub static fn defaults(module_name: string): ExplorerOptions {
        return ExplorerOptions {
            module_name: module_name,
            stop_dir: "/",
            package_prop: module_name,
            ignore_empty: true
        };
    }
}

/// Detects the loader kind for `filepath` based on filename and extension.
/// - `package.json` -> `LOADER_PKG_JSON`
/// - `.json` -> `LOADER_JSON`
/// - `.yaml`, `.yml`, or extensionless rc files (e.g. `.apprc`, `.config/apprc`) -> `LOADER_YAML`
/// - `.js`, `.cjs`, `.mjs` -> `LOADER_JS`
/// - Otherwise -> `LOADER_UNKNOWN`
pub fn detectLoader(filepath: string): u8 {
    let base = cosmiBasename(filepath);
    if (base.len == 0) {
        return LOADER_UNKNOWN;
    }
    if (eqIgnoreCase(base, "package.json")) {
        return LOADER_PKG_JSON;
    }
    if (cosmiEndsWithIgnoreCase(base, ".json")) {
        return LOADER_JSON;
    }
    if (cosmiEndsWithIgnoreCase(base, ".yaml") || cosmiEndsWithIgnoreCase(base, ".yml")) {
        return LOADER_YAML;
    }
    if (cosmiEndsWithIgnoreCase(base, ".js") ||
        cosmiEndsWithIgnoreCase(base, ".cjs") ||
        cosmiEndsWithIgnoreCase(base, ".mjs")) {
        return LOADER_JS;
    }

    // Check for extensionless rc file (no '.' after index 0).
    let has_ext_dot = false;
    let i: usize = 1;
    while (i < base.len) {
        if (base[i] == 46) { // '.'
            has_ext_dot = true;
            break;
        }
        i = i + 1;
    }
    if (!has_ext_dot) {
        return LOADER_YAML;
    }
    return LOADER_UNKNOWN;
}

/// Formats one of the 8 default cosmiconfig search places for `module_name` into `out`.
/// Order:
/// 0: `package.json`
/// 1: `.<mod>rc`
/// 2: `.<mod>rc.json`
/// 3: `.<mod>rc.yaml`
/// 4: `.<mod>rc.yml`
/// 5: `.config/<mod>rc`
/// 6: `<mod>.config.js`
/// 7: `<mod>.config.cjs`
pub fn formatSearchPlace(module_name: string, place_idx: usize, out: []u8): usize {
    if (place_idx >= DEFAULT_SEARCH_PLACE_COUNT) {
        return 0;
    }
    if (place_idx == 0) {
        return writeStr("package.json", out, 0);
    }
    if (module_name.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    if (place_idx == 1) {
        pos = writeStr(".", out, pos);
        pos = writeStr(module_name, out, pos);
        pos = writeStr("rc", out, pos);
        return pos;
    }
    if (place_idx == 2) {
        pos = writeStr(".", out, pos);
        pos = writeStr(module_name, out, pos);
        pos = writeStr("rc.json", out, pos);
        return pos;
    }
    if (place_idx == 3) {
        pos = writeStr(".", out, pos);
        pos = writeStr(module_name, out, pos);
        pos = writeStr("rc.yaml", out, pos);
        return pos;
    }
    if (place_idx == 4) {
        pos = writeStr(".", out, pos);
        pos = writeStr(module_name, out, pos);
        pos = writeStr("rc.yml", out, pos);
        return pos;
    }
    if (place_idx == 5) {
        pos = writeStr(".config/", out, pos);
        pos = writeStr(module_name, out, pos);
        pos = writeStr("rc", out, pos);
        return pos;
    }
    if (place_idx == 6) {
        pos = writeStr(module_name, out, pos);
        pos = writeStr(".config.js", out, pos);
        return pos;
    }
    // place_idx == 7
    pos = writeStr(module_name, out, pos);
    pos = writeStr(".config.cjs", out, pos);
    return pos;
}

/// Joins `dir` and `place` into a normalized candidate file path in `out`.
/// Returns bytes written.
pub fn buildCandidatePath(dir: string, place: string, out: []u8): usize {
    if (dir.len == 0) {
        return writeStr(place, out, 0);
    }
    let clean_dir = cosmiTrimTrailingSlash(dir);
    let pos: usize = writeStr(clean_dir, out, 0);
    if (place.len == 0) {
        return pos;
    }
    if (clean_dir.len == 1 && clean_dir[0] == 47) {
        return writeStr(place, out, pos);
    }
    pos = writeStr("/", out, pos);
    return writeStr(place, out, pos);
}

/// Zero-heap upward directory search planner respecting `stop_dir`.
pub struct SearchPlanner {
    pub start_dir: string,
    pub stop_dir: string,
    pub current_end: usize,
    pub done: bool,

    pub static fn new(start_dir: string, stop_dir: string): SearchPlanner {
        let norm_start = cosmiTrimTrailingSlash(start_dir);
        let norm_stop = cosmiTrimTrailingSlash(stop_dir);
        let is_empty = norm_start.len == 0;
        return SearchPlanner {
            start_dir: norm_start,
            stop_dir: norm_stop,
            current_end: norm_start.len,
            done: is_empty
        };
    }

    /// Yields the next directory walking upward from `start_dir` until `stop_dir` or `/`.
    pub fn nextDir(*_): string? {
        if (_.done) {
            return null;
        }
        let dir = _.start_dir[0.._.current_end];
        if (_.stop_dir.len > 0 && strEq(dir, _.stop_dir)) {
            _.done = true;
            return dir;
        }
        if (_.current_end == 1 && _.start_dir[0] == 47) {
            _.done = true;
            return dir;
        }
        if (slash := rfindByte(dir, 47)) {
            if (slash == 0) {
                _.current_end = 1;
            } else {
                _.current_end = slash;
            }
        } else {
            _.done = true;
        }
        return dir;
    }

    /// Returns the total number of directories that will be visited from `start_dir` to `stop_dir`.
    pub fn dirCount(): usize {
        if (_.start_dir.len == 0) {
            return 0;
        }
        let count: usize = 0;
        let cur_end = _.start_dir.len;
        while (true) {
            let dir = _.start_dir[0..cur_end];
            count = count + 1;
            if (_.stop_dir.len > 0 && strEq(dir, _.stop_dir)) {
                break;
            }
            if (cur_end == 1 && _.start_dir[0] == 47) {
                break;
            }
            if (slash := rfindByte(dir, 47)) {
                if (slash == 0) {
                    cur_end = 1;
                } else {
                    cur_end = slash;
                }
            } else {
                break;
            }
        }
        return count;
    }
}

/// Resolution result for a discovered configuration file.
pub struct CosmiConfigResult {
    pub filepath: string,
    pub is_empty: bool,
    pub loader_kind: u8,
    pub found: bool,

    pub static fn notFound(): CosmiConfigResult {
        return CosmiConfigResult {
            filepath: "",
            is_empty: true,
            loader_kind: LOADER_UNKNOWN,
            found: false
        };
    }

    pub static fn hit(filepath: string, is_empty: bool): CosmiConfigResult {
        return CosmiConfigResult {
            filepath: filepath,
            is_empty: is_empty,
            loader_kind: detectLoader(filepath),
            found: true
        };
    }
}
