// Type definitions for CP

export class FlatOptions {
    delimiter: u8,
    max_depth: usize,
    safe: bool,
    fn defaults(): FlatOptions;
}

export class FlatEntry {
    key: [96]u8,
    key_len: usize,
    val: [128]u8,
    val_len: usize,
}

export fn ufStrEq(a: string, b: string): bool;
export fn ufCopyBuf(dst: []u8, src: string): usize;
export fn ufIsWs(ch: u8): bool;
export fn ufIsDigit(ch: u8): bool;
export fn ufIsHex(ch: u8): bool;
export fn ufSkipWs(s: string, pos: usize): usize;
export fn ufFormatUsize(val_in: usize, out: []u8): usize;
export fn ufEmptyEntry(): FlatEntry;
export class FlatTable {
    entries: [48]FlatEntry,
    len: usize,
    fn new(): FlatTable;
    fn set(_: auto, key: string, raw_json_val: string): bool;
    fn get(key: string, out: []u8): i64;
    fn count(): usize;
}

export fn ufSkipString(s: string, pos: usize): i64;
export class UfStrRes {
    ok: bool,
    next_pos: usize,
    out_len: usize,
}

export fn ufDecodeString(s: string, pos: usize, out: []u8): UfStrRes;
export fn ufSkipNumber(s: string, pos: usize): i64;
export fn ufSkipValue(s: string, pos: usize, depth: usize): i64;
export fn ufJoinKey(prefix: string, delim: u8, seg: string, out: []u8): i64;
export fn ufFlattenValue(s: string, pos: usize, prefix: string, depth: usize, opts: FlatOptions, out_table: &mut FlatTable): i64;
export fn flattenJson(json_in: string, opts: FlatOptions, out_table: &mut FlatTable): i64;
export fn ufEmitByte(out: []u8, pos: usize, b: u8): i64;
export fn ufEmitBytes(out: []u8, pos: usize, s: string): i64;
export fn ufEmitJsonString(out: []u8, pos: usize, s: string): i64;
export fn flattenToJson(json_in: string, opts: FlatOptions, out: []u8): i64;
export class UfNode {
    kind: u8,
    name: [48]u8,
    name_len: usize,
    val: [128]u8,
    val_len: usize,
    first_child: i32,
    last_child: i32,
    next_sibling: i32,
}

export class UfPool {
    nodes: [96]UfNode,
    count: usize,
}

export fn ufAllocNode(pool: &mut UfPool, kind: u8, name: string): i32;
export fn ufFindChild(pool: &mut UfPool, parent_idx: i32, seg: string): i32;
export fn ufAppendChild(pool: &mut UfPool, parent_idx: i32, child_idx: i32);
export fn ufInsertPath(pool: &mut UfPool, path: string, delim: u8, raw_val: string): bool;
export fn ufIsSequentialArray(pool: &mut UfPool, node_idx: i32): bool;
export fn ufEmitNode(pool: &mut UfPool, node_idx: i32, out: []u8, pos: usize, is_root: bool): i64;
export fn unflattenJson(flat_json: string, opts: FlatOptions, out: []u8): i64;
