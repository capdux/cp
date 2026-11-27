// Type definitions for CP

import { eqIgnoreCase, strEq } from "cap:ascii";

export class PathSpan {
    start: usize,
    len: usize,
}

export class EnvPair {
    key: string,
    value: string,
}

export class PathKeyOptions {
    platform: string,
    static fn posix(): PathKeyOptions;
    static fn win32(): PathKeyOptions;
}

export fn isWindowsPlatform(platform: string): bool;
export fn isPathKeyName(key: string): bool;
export fn findPathKeyIndex(keys: []string, is_windows: bool): i64;
export fn defaultPathKey(is_windows: bool): string;
export fn pathDelimiter(is_windows: bool): u8;
export fn pathKey(keys: []string, platform: string): string;
export fn pathKeyFromPairs(env: []EnvPair, platform: string): string;
export fn findPathValue(env: []EnvPair, platform: string): string?;
export fn splitPathEntries(path_val: string, is_windows: bool, out: []PathSpan): usize;
export fn joinPathEntries(entries: []string, is_windows: bool, out: []u8): usize;
export fn prependPathEntry(entry: string, existing_path: string, is_windows: bool, out: []u8): usize;
