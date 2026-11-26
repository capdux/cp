// Type definitions for CP

export class PathSpan {
    start: usize,
    len: usize,
}

export fn toLowerAscii(c: u8): u8;
export fn isPathKeyName(key: string): bool;
export fn findPathKeyIndex(keys: []string, is_windows: bool): i64;
export fn defaultPathKey(is_windows: bool): string;
export fn pathDelimiter(is_windows: bool): u8;
export fn splitPathEntries(path_val: string, is_windows: bool, out: []PathSpan): usize;
