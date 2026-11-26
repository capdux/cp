// Type definitions for CP

export class FlatResult {
    pool: [MAX_FLAT_POOL]u8,
    pool_len: usize,
    key_starts: [MAX_FLAT_ENTRIES]usize,
    key_lens: [MAX_FLAT_ENTRIES]usize,
    val_starts: [MAX_FLAT_ENTRIES]usize,
    val_lens: [MAX_FLAT_ENTRIES]usize,
    count: usize,
    ok: bool,
    fn empty(): FlatResult;
    fn pushEntry(_: auto, key: string, val: string): bool;
    fn keyAt(idx: usize): string;
    fn valAt(idx: usize): string;
    fn get(path: string): string?;
    fn has(path: string): bool;
}

export fn strEq(a: string, b: string): bool;
export fn isWs(c: u8): bool;
export fn skipWs(s: string, pos: usize): usize;
export fn parseJsonStringEnd(s: string, open_quote: usize): usize;
export fn formatUsize(v: usize, buf: []u8): string;
export fn parseValue(s: string, start_pos: usize, prefix: []u8, prefix_len: usize, sep: u8, depth: usize, res: &mut FlatResult): usize;
export fn flattenJsonWith(json_str: string, sep: u8): FlatResult;
export fn flattenJson(json_str: string): FlatResult;
export fn getByPath(json_str: string, dot_path: string, out: []u8): string?;
export fn hasPath(json_str: string, dot_path: string): bool;
export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn formatFlatJsonObject(flat: FlatResult, out: []u8): string;
