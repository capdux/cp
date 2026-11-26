// Type definitions for CP

export enum FlatNodeKind {
    Null,
    Bool,
    Int,
    Str,
    Array,
    Object
}

export class FlatNode {
    kind: FlatNodeKind,
    bool_val: bool,
    int_val: i64,
    str_val: string,
    first_edge: i32,
    last_edge: i32,
    edge_count: usize,
}

export class FlatEdge {
    key: string,
    child_id: usize,
    next: i32,
}

export fn fltEmptyNode(): FlatNode;
export fn fltEmptyEdge(): FlatEdge;
export fn fltStrEq(a: string, b: string): bool;
export class FlatArena {
    nodes: [64]FlatNode,
    node_count: usize,
    edges: [256]FlatEdge,
    edge_count: usize,
    fn init(): FlatArena;
    fn reset(_: auto);
    fn nodeCount(): usize;
    fn addNull(_: auto): usize;
    fn addBool(_: auto, v: bool): usize;
    fn addInt(_: auto, v: i64): usize;
    fn addStr(_: auto, s: string): usize;
    fn addArray(_: auto): usize;
    fn addObject(_: auto): usize;
    fn setField(_: auto, obj_id: usize, key: string, child_id: usize): bool;
    fn pushElem(_: auto, arr_id: usize, child_id: usize): bool;
    fn nodeKind(id: usize): FlatNodeKind;
    fn nodeBool(id: usize): bool;
    fn nodeInt(id: usize): i64;
    fn nodeStr(id: usize): string;
    fn elemCount(arr_id: usize): usize;
    fn getField(obj_id: usize, key: string): i64;
    fn getElem(arr_id: usize, idx: usize): i64;
}

export fn fltPush(out: []u8, pos: usize, b: u8): usize;
export fn fltPushSlice(out: []u8, pos: usize, s: string): usize;
export fn fltPushUsize(out: []u8, pos: usize, val: usize): usize;
export fn fltPushI64(out: []u8, pos: usize, val: i64): usize;
export fn fltPushJsonString(out: []u8, pos: usize, s: string): usize;
export fn fltNeedsSlot(kind: FlatNodeKind): bool;
export fn stringifyFlatted(arena: FlatArena, root_id: usize, out: []u8): usize;
export class FltSpan {
    ok: bool,
    start: usize,
    end: usize,
    next: usize,
}

export fn fltSkipWs(s: string, pos: usize): usize;
export fn fltParseStringSpan(s: string, pos: usize): FltSpan;
export fn fltSkipValue(s: string, pos: usize): i64;
export fn fltParseUsizeStr(s: string): i64;
export fn fltParseI64Token(s: string): i64?;
export fn fltParseInlineValue(json: string, start: usize, end: usize, slot_to_node: []usize, slot_count: usize, arena: &mut FlatArena): i64;
export fn parseFlatted(json: string, out_arena: &mut FlatArena): i64;
