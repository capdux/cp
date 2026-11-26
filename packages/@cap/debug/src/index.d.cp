// Type definitions for CP

export fn isSep(c: u8): bool;
export fn globMatch(pat: string, text: string): bool;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn appendU64(out: []u8, pos: usize, val: u64): usize;
export fn isNamespaceEnabled(pattern: string, namespace: string): bool;
export fn selectColor(namespace: string): u8;
export fn formatDiffMs(diff_ms: i64, out: []u8): string;
export fn formatDebugLine(ns: string, msg: string, diff_ms: i64, use_color: bool, out: []u8): string;
export class DebugFilter {
    raw_pattern: string,
    fn from(pattern: string): DebugFilter;
    fn isEnabled(namespace: string): bool;
}

export class DebugLogger {
    namespace: string,
    color_code: u8,
    last_ms: i64,
    enabled: bool,
    fn create(pattern: string, namespace: string): DebugLogger;
    fn formatLine(msg: string, diff_ms: i64, use_color: bool, out: []u8): string;
}

