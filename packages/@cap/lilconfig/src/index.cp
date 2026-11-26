// @cap/lilconfig — Pure CP zero-allocation lightweight config explorer.
// Clean-room MIT implementation inspired by npm `lilconfig` v3.
// Reuses `@cap/cosmiconfig` for upward SearchPlanner, candidate path building, and loader detection.

import {
    LOADER_UNKNOWN,
    LOADER_JSON,
    LOADER_YAML,
    LOADER_JS,
    LOADER_PKG_JSON,
    SearchPlanner,
    CosmiConfigResult,
    detectLoader,
    buildCandidatePath
} from "../../cosmiconfig/src/index.cp";

pub const LIL_DEFAULT_PLACE_COUNT: usize = 6;
pub const LIL_MAX_CUSTOM_LOADERS: usize = 8;

fn lilToLower(b: u8): u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn lilEqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (lilToLower(a[i]) != lilToLower(b[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn lilEndsWithIgnoreCase(s: string, suffix: string): bool {
    if (suffix.len > s.len) {
        return false;
    }
    let offset = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (lilToLower(s[offset + i]) != lilToLower(suffix[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn lilWriteStr(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn lilBasename(filepath: string): string {
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

fn lilIsNoExt(filepath: string): bool {
    let base = lilBasename(filepath);
    if (base.len == 0) {
        return false;
    }
    let i: usize = 1;
    while (i < base.len) {
        if (base[i] == 46) { // '.'
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// Configuration options for `LilExplorer` (inspired by `lilconfig` v3).
pub struct LilconfigOptions {
    pub module_name: string,
    pub stop_dir: string,
    pub package_prop: string,
    pub ignore_empty_search_places: bool,

    pub static fn defaults(module_name: string): LilconfigOptions {
        return LilconfigOptions {
            module_name: module_name,
            stop_dir: "/",
            package_prop: module_name,
            ignore_empty_search_places: true
        };
    }
}

/// Formats one of the 6 default `lilconfig` search places (no YAML by default) into `out`.
/// Order:
/// 0: `package.json`
/// 1: `.<mod>rc`
/// 2: `.<mod>rc.json`
/// 3: `.<mod>rc.js`
/// 4: `.<mod>rc.cjs`
/// 5: `<mod>.config.js`
pub fn formatLilSearchPlace(module_name: string, idx: usize, out: []u8): usize {
    if (idx >= LIL_DEFAULT_PLACE_COUNT) {
        return 0;
    }
    if (idx == 0) {
        return lilWriteStr(out, 0, "package.json");
    }
    if (module_name.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    if (idx == 1) {
        pos = lilWriteStr(out, pos, ".");
        pos = lilWriteStr(out, pos, module_name);
        pos = lilWriteStr(out, pos, "rc");
        return pos;
    }
    if (idx == 2) {
        pos = lilWriteStr(out, pos, ".");
        pos = lilWriteStr(out, pos, module_name);
        pos = lilWriteStr(out, pos, "rc.json");
        return pos;
    }
    if (idx == 3) {
        pos = lilWriteStr(out, pos, ".");
        pos = lilWriteStr(out, pos, module_name);
        pos = lilWriteStr(out, pos, "rc.js");
        return pos;
    }
    if (idx == 4) {
        pos = lilWriteStr(out, pos, ".");
        pos = lilWriteStr(out, pos, module_name);
        pos = lilWriteStr(out, pos, "rc.cjs");
        return pos;
    }
    // idx == 5
    pos = lilWriteStr(out, pos, module_name);
    pos = lilWriteStr(out, pos, ".config.js");
    return pos;
}

/// Returns `true` if `filepath` is supported by `lilconfig`'s built-in loaders
/// (JSON, JS/CJS/MJS, `package.json`, and extensionless rc files; rejects unregistered YAML).
pub fn isSupportedByDefault(filepath: string): bool {
    let base = lilBasename(filepath);
    if (base.len == 0) {
        return false;
    }
    let kind = detectLoader(filepath);
    if (kind == LOADER_PKG_JSON || kind == LOADER_JSON || kind == LOADER_JS) {
        return true;
    }
    if (lilIsNoExt(filepath)) {
        return true;
    }
    return false;
}

/// A custom extension-to-loader registration entry.
pub struct CustomLoaderEntry {
    pub ext: string,
    pub loader_id: u8,
}

/// Zero-heap lightweight configuration explorer with upward `SearchPlanner` and
/// up to 8 custom extension loader registrations.
pub struct LilExplorer {
    pub opts: LilconfigOptions,
    pub planner: SearchPlanner,
    pub custom_loaders: [8]CustomLoaderEntry,
    pub custom_count: usize,

    pub static fn new(start_dir: string, opts: LilconfigOptions): LilExplorer {
        let blank = CustomLoaderEntry { ext: "", loader_id: LOADER_UNKNOWN };
        let arr: [8]CustomLoaderEntry = [blank; 8];
        let p = SearchPlanner.new(start_dir, opts.stop_dir);
        return LilExplorer {
            opts: opts,
            planner: p,
            custom_loaders: arr,
            custom_count: 0
        };
    }

    /// Registers or updates a custom loader ID for an extension (e.g. `".yaml"`, `".toml"`, or `"noExt"`).
    /// Returns `false` if `ext` is empty or the 8-slot registry is full.
    pub fn registerLoader(*_, ext: string, loader_id: u8): bool {
        if (ext.len == 0) {
            return false;
        }
        let i: usize = 0;
        while (i < _.custom_count) {
            if (lilEqIgnoreCase(_.custom_loaders[i].ext, ext)) {
                _.custom_loaders[i].loader_id = loader_id;
                return true;
            }
            i = i + 1;
        }
        if (_.custom_count >= LIL_MAX_CUSTOM_LOADERS) {
            return false;
        }
        _.custom_loaders[_.custom_count] = CustomLoaderEntry { ext: ext, loader_id: loader_id };
        _.custom_count = _.custom_count + 1;
        return true;
    }

    /// Resolves the loader ID for `filepath`, checking custom registrations before defaults.
    pub fn resolveLoader(filepath: string): u8 {
        let base = lilBasename(filepath);
        if (base.len == 0) {
            return LOADER_UNKNOWN;
        }
        let no_ext = lilIsNoExt(filepath);

        let i: usize = 0;
        while (i < _.custom_count) {
            let ext = _.custom_loaders[i].ext;
            if (no_ext && lilEqIgnoreCase(ext, "noExt")) {
                return _.custom_loaders[i].loader_id;
            }
            if (!no_ext && lilEndsWithIgnoreCase(base, ext)) {
                return _.custom_loaders[i].loader_id;
            }
            i = i + 1;
        }

        let kind = detectLoader(filepath);
        if (kind == LOADER_PKG_JSON || kind == LOADER_JSON || kind == LOADER_JS) {
            return kind;
        }
        if (no_ext) {
            return LOADER_JSON;
        }
        return LOADER_UNKNOWN;
    }

    /// Yields the next directory walking upward via the embedded `SearchPlanner`.
    pub fn nextSearchDir(*_): string? {
        return _.planner.nextDir();
    }

    /// Builds the candidate path for `(dir, place_idx)` into `out` using `@cap/cosmiconfig`'s `buildCandidatePath`.
    pub fn candidatePath(dir: string, place_idx: usize, out: []u8): usize {
        let pbuf: [64]u8 = [0 as u8; 64];
        let plen = formatLilSearchPlace(_.opts.module_name, place_idx, pbuf[0..64]);
        if (plen == 0) {
            return 0;
        }
        let place: string = pbuf[0..plen];
        return buildCandidatePath(dir, place, out);
    }

    /// Constructs a `CosmiConfigResult` using this explorer's resolved loader ID.
    pub fn makeResult(filepath: string, is_empty: bool): CosmiConfigResult {
        return CosmiConfigResult {
            filepath: filepath,
            is_empty: is_empty,
            loader_kind: _.resolveLoader(filepath),
            found: true
        };
    }
}
