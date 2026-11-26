// Type definitions for CP

export class ParamSlice {
    name_start: usize,
    name_len: usize,
    val_start: usize,
    val_len: usize,
}

export class MatchResult {
    matched: bool,
    param_count: usize,
    params: [8]ParamSlice,
}

export class Segment {
    kind: u8,
    start: usize,
    len: usize,
}

export fn emptyMatchResult(): MatchResult;
export class RoutePattern {
    pat_buf: [256]u8,
    pat_len: usize,
    segments: [16]Segment,
    seg_count: usize,
    param_count: usize,
    fn compile(pattern: &[]u8): RoutePattern;
    fn segmentCount(): usize;
    fn paramCount(): usize;
    fn matchPath(path: &[]u8): MatchResult;
}

export fn buildPath(pattern: &[]u8, param_values: &[]&[]u8, dst: []u8): i64;
