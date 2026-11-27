// Type definitions for CP

export class GridPos {
    x: u16,
    y: u16,
}

export class AStarResult {
    found: bool,
    cost: i64,
    path_len: usize,
}

export fn absDiffUsize(a: usize, b: usize): i64;
export fn heuristic(x1: usize, y1: usize, x2: usize, y2: usize, allow_diagonal: bool): i64;
export class GridMap {
    width: usize,
    height: usize,
    blocked: [1024]bool,
    fn init(width: usize, height: usize): GridMap;
    fn setBlocked(_: auto, x: usize, y: usize, is_blocked: bool);
    fn isBlocked(x: usize, y: usize): bool;
    fn findPath(start: GridPos, goal: GridPos, allow_diagonal: bool, out_path: []GridPos): AStarResult;
}

