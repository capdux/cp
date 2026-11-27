// @cap/path-key — Zero-heap cross-platform PATH env key resolver (matching npm path-key 250M+/wk)
// Clean-room MIT implementation in pure CP.

import { eqIgnoreCase, strEq } from "cap:ascii";

pub struct PathSpan {
    pub start: usize,
    pub len: usize,
}

/// Environment key-value pair for zero-heap env table lookups.
pub struct EnvPair {
    pub key: string,
    pub value: string,
}

/// Options mirroring npm `path-key` (`platform` override such as `"win32"`, `"linux"`, `"darwin"`).
pub struct PathKeyOptions {
    pub platform: string,

    pub static fn posix(): PathKeyOptions {
        return PathKeyOptions { platform: "linux" };
    }

    pub static fn win32(): PathKeyOptions {
        return PathKeyOptions { platform: "win32" };
    }
}

/// Returns `true` if `platform` refers to Windows (`"win32"` or `"windows"`, case-insensitive).
pub fn isWindowsPlatform(platform: string): bool {
    return eqIgnoreCase(platform, "win32") || eqIgnoreCase(platform, "windows");
}

pub fn isPathKeyName(key: string): bool {
    return eqIgnoreCase(key, "path");
}

/// Given a list of environment variable names and whether platform is Windows,
/// returns the index of the effective PATH key (last case-insensitive match on Windows,
/// exact `"PATH"` on POSIX), or `-1` if none found.
pub fn findPathKeyIndex(keys: []string, is_windows: bool): i64 {
    if (!is_windows) {
        let i: usize = 0;
        while (i < keys.len) : (i += 1) {
            if (strEq(keys[i], "PATH")) {
                return i as i64;
            }
        }
        return -1;
    }
    let found: i64 = -1;
    let i: usize = 0;
    while (i < keys.len) : (i += 1) {
        if (isPathKeyName(keys[i])) {
            found = i as i64;
        }
    }
    return found;
}

pub fn defaultPathKey(is_windows: bool): string {
    if (is_windows) { return "Path"; }
    return "PATH";
}

pub fn pathDelimiter(is_windows: bool): u8 {
    if (is_windows) { return 59; } // ';'
    return 58; // ':'
}

/// Full npm `path-key` API: resolves the effective PATH key name for `keys` and `platform`.
/// On non-Windows platforms always returns `"PATH"`.
/// On Windows (`"win32"` / `"windows"`), returns the last case-insensitive match in `keys`,
/// or `"Path"` if no PATH key exists.
pub fn pathKey(keys: []string, platform: string): string {
    const is_win = isWindowsPlatform(platform);
    if (!is_win) {
        return "PATH";
    }
    const idx = findPathKeyIndex(keys, true);
    if (idx >= 0) {
        return keys[idx as usize];
    }
    return "Path";
}

/// Resolves the effective PATH key name from an array of `EnvPair` entries.
pub fn pathKeyFromPairs(env: []EnvPair, platform: string): string {
    const is_win = isWindowsPlatform(platform);
    if (!is_win) {
        return "PATH";
    }
    let found: i64 = -1;
    let i: usize = 0;
    while (i < env.len) : (i += 1) {
        if (isPathKeyName(env[i].key)) {
            found = i as i64;
        }
    }
    if (found >= 0) {
        return env[found as usize].key;
    }
    return "Path";
}

/// Finds the value of the effective PATH environment variable in `env`, or `null` if absent.
pub fn findPathValue(env: []EnvPair, platform: string): string? {
    const is_win = isWindowsPlatform(platform);
    if (!is_win) {
        let i: usize = 0;
        while (i < env.len) : (i += 1) {
            if (strEq(env[i].key, "PATH")) {
                return env[i].value;
            }
        }
        return null;
    }
    let found: i64 = -1;
    let i: usize = 0;
    while (i < env.len) : (i += 1) {
        if (isPathKeyName(env[i].key)) {
            found = i as i64;
        }
    }
    if (found >= 0) {
        return env[found as usize].value;
    }
    return null;
}

/// Split a PATH string into non-empty entry spans using platform delimiter (`':'` or `';'`).
pub fn splitPathEntries(path_val: string, is_windows: bool, out: []PathSpan): usize {
    const delim = pathDelimiter(is_windows);
    const n = path_val.len;
    let count: usize = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= n) : (i += 1) {
        const at_sep = (i == n) || (path_val[i] == delim);
        if (at_sep) {
            const seg_len = i - start;
            if (seg_len > 0 && count < out.len) {
                out[count] = PathSpan { start: start, len: seg_len };
                count += 1;
            }
            start = i + 1;
        }
    }
    return count;
}

fn pkCopyInto(out: []u8, pos: usize, s: string): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

/// Join non-empty PATH entries with platform delimiter (`':'` or `';'`) into `out`.
pub fn joinPathEntries(entries: []string, is_windows: bool, out: []u8): usize {
    const delim = pathDelimiter(is_windows);
    let pos: usize = 0;
    let wrote_any: bool = false;
    let i: usize = 0;
    while (i < entries.len && pos < out.len) : (i += 1) {
        const e = entries[i];
        if (e.len == 0) {
            continue;
        }
        if (wrote_any) {
            if (pos >= out.len) { break; }
            out[pos] = delim;
            pos += 1;
        }
        pos = pkCopyInto(out, pos, e);
        wrote_any = true;
    }
    return pos;
}

/// Prepend `entry` to `existing_path` using the platform delimiter, writing into `out`.
pub fn prependPathEntry(entry: string, existing_path: string, is_windows: bool, out: []u8): usize {
    if (entry.len == 0) {
        return pkCopyInto(out, 0, existing_path);
    }
    if (existing_path.len == 0) {
        return pkCopyInto(out, 0, entry);
    }
    let pos: usize = pkCopyInto(out, 0, entry);
    if (pos < out.len) {
        out[pos] = pathDelimiter(is_windows);
        pos += 1;
    }
    return pkCopyInto(out, pos, existing_path);
}

export {
    PathSpan,
    EnvPair,
    PathKeyOptions,
    isWindowsPlatform,
    isPathKeyName,
    findPathKeyIndex,
    defaultPathKey,
    pathDelimiter,
    pathKey,
    pathKeyFromPairs,
    findPathValue,
    splitPathEntries,
    joinPathEntries,
    prependPathEntry,
};
