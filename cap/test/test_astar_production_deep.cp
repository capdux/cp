// Source: cp/cap/test/test_astar_production_deep.cp (Production deep tests for cap:astar)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    COST_CARDINAL,
    COST_DIAGONAL,
    GridPos,
    AstarResult,
    AstarWorkspace,
    manhattanHeuristic,
    octileHeuristic,
    astarFindPath,
    hasLineOfSight,
} from "cap:gfx";

fn absI32(v: i32): i32 {
    return if (v < 0) -v else v;
}

fn verifyValidPath(
    grid: []const u8,
    width: usize,
    path: []const GridPos,
    start: GridPos,
    goal: GridPos,
    allow_diag: bool,
): u32 {
    expect_eq(path.len >= 1, true);
    expect_eq(path[0].x, start.x);
    expect_eq(path[0].y, start.y);
    expect_eq(path[path.len - 1].x, goal.x);
    expect_eq(path[path.len - 1].y, goal.y);

    let cost: u32 = 0;
    let i: usize = 0;
    while (i < path.len) {
        const idx = (path[i].y as usize) * width + (path[i].x as usize);
        expect_eq(grid[idx], 0 as u8);
        if (i > 0) {
            const dx = absI32(path[i].x - path[i - 1].x);
            const dy = absI32(path[i].y - path[i - 1].y);
            expect_eq(dx <= 1 && dy <= 1 && (dx + dy > 0), true);
            if (!allow_diag) {
                expect_eq(dx + dy, 1 as i32);
            }
            if (dx == 1 && dy == 1) {
                // Verify no corner cutting along returned path
                const o1 = (path[i - 1].y as usize) * width + (path[i].x as usize);
                const o2 = (path[i].y as usize) * width + (path[i - 1].x as usize);
                expect_eq(grid[o1], 0 as u8);
                expect_eq(grid[o2], 0 as u8);
                cost += COST_DIAGONAL;
            } else {
                cost += COST_CARDINAL;
            }
        }
        i += 1;
    }
    return cost;
}

test("1. Open grid 4-neighbor (Manhattan) & 8-neighbor (Octile) optimal cost & path continuity", () => {
    expect_eq(COST_CARDINAL, 10 as u32);
    expect_eq(COST_DIAGONAL, 14 as u32);

    const w: usize = 10;
    const h: usize = 10;
    let grid: [100]u8 = std.mem.zeroes([100]u8);
    let g_buf: [100]u32 = std.mem.zeroes([100]u32);
    let p_buf: [100]i32 = std.mem.zeroes([100]i32);
    let c_buf: [100]u8 = std.mem.zeroes([100]u8);
    let hn_buf: [100]u32 = std.mem.zeroes([100]u32);
    let hf_buf: [100]u32 = std.mem.zeroes([100]u32);
    let path: [100]GridPos = std.mem.zeroes([100]GridPos);

    let ws = AstarWorkspace {
        g_score: g_buf[0..],
        parent: p_buf[0..],
        closed: c_buf[0..],
        heap_nodes: hn_buf[0..],
        heap_f: hf_buf[0..],
    };

    const start = GridPos { x: 0, y: 0 };
    const goal = GridPos { x: 9, y: 6 };

    expect_eq(manhattanHeuristic(start, goal), 150 as u32);
    expect_eq(octileHeuristic(start, goal), 114 as u32); // 6*14 + 3*10 = 114

    // 4-neighbor optimal path
    const r4 = astarFindPath(grid[0..], w, h, start, goal, false, &ws, path[0..]) ?? unreachable;
    expect_eq(r4.found, true);
    expect_eq(r4.total_cost, 150 as u32);
    expect_eq(r4.path_len, 16 as usize);
    expect_eq(verifyValidPath(grid[0..], w, path[0..r4.path_len], start, goal, false), r4.total_cost);

    // 8-neighbor optimal path
    const r8 = astarFindPath(grid[0..], w, h, start, goal, true, &ws, path[0..]) ?? unreachable;
    expect_eq(r8.found, true);
    expect_eq(r8.total_cost, 114 as u32);
    expect_eq(r8.path_len, 10 as usize);
    expect_eq(verifyValidPath(grid[0..], w, path[0..r8.path_len], start, goal, true), r8.total_cost);
});

test("2. U-shaped trap maze detour optimal pathfinding (4-neighbor & 8-neighbor)", () => {
    // 8x8 grid with a U-shaped wall blocking direct rightward path from (2,3) to (6,3)
    const w: usize = 8;
    const h: usize = 8;
    let grid: [64]u8 = std.mem.zeroes([64]u8);

    // Build U-trap: vertical wall at x=4, y=1..5; horizontal arms at y=1, x=2..4 and y=5, x=2..4
    let y: usize = 1;
    while (y <= 5) {
        grid[y * w + 4] = 1;
        y += 1;
    }
    grid[1 * w + 2] = 1;
    grid[1 * w + 3] = 1;
    grid[5 * w + 2] = 1;
    grid[5 * w + 3] = 1;

    let g_buf: [64]u32 = std.mem.zeroes([64]u32);
    let p_buf: [64]i32 = std.mem.zeroes([64]i32);
    let c_buf: [64]u8 = std.mem.zeroes([64]u8);
    let hn_buf: [64]u32 = std.mem.zeroes([64]u32);
    let hf_buf: [64]u32 = std.mem.zeroes([64]u32);
    let path: [64]GridPos = std.mem.zeroes([64]GridPos);

    let ws = AstarWorkspace {
        g_score: g_buf[0..],
        parent: p_buf[0..],
        closed: c_buf[0..],
        heap_nodes: hn_buf[0..],
        heap_f: hf_buf[0..],
    };

    const start = GridPos { x: 3, y: 3 };
    const goal = GridPos { x: 6, y: 3 };

    const r4 = astarFindPath(grid[0..], w, h, start, goal, false, &ws, path[0..]) ?? unreachable;
    expect_eq(r4.found, true);
    expect_eq(r4.total_cost > 30, true); // Must detour around the U-trap
    expect_eq(verifyValidPath(grid[0..], w, path[0..r4.path_len], start, goal, false), r4.total_cost);

    const r8 = astarFindPath(grid[0..], w, h, start, goal, true, &ws, path[0..]) ?? unreachable;
    expect_eq(r8.found, true);
    expect_eq(r8.total_cost < r4.total_cost, true);
    expect_eq(verifyValidPath(grid[0..], w, path[0..r8.path_len], start, goal, true), r8.total_cost);
});

test("3. Strict 8-neighbor no-corner-cutting across touching diagonal walls", () => {
    // 4x4 grid where (0,0) is sealed off by two touching diagonal walls at (1,0) and (0,1).
    // Even with allow_diagonal=true, agent MUST NOT squeeze between (1,0) and (0,1) to reach (1,1)!
    const w: usize = 4;
    const h: usize = 4;
    let grid: [16]u8 = std.mem.zeroes([16]u8);
    grid[0 * w + 1] = 1; // (1, 0)
    grid[1 * w + 0] = 1; // (0, 1)

    let g_buf: [16]u32 = std.mem.zeroes([16]u32);
    let p_buf: [16]i32 = std.mem.zeroes([16]i32);
    let c_buf: [16]u8 = std.mem.zeroes([16]u8);
    let hn_buf: [16]u32 = std.mem.zeroes([16]u32);
    let hf_buf: [16]u32 = std.mem.zeroes([16]u32);
    let path: [16]GridPos = std.mem.zeroes([16]GridPos);

    let ws = AstarWorkspace {
        g_score: g_buf[0..],
        parent: p_buf[0..],
        closed: c_buf[0..],
        heap_nodes: hn_buf[0..],
        heap_f: hf_buf[0..],
    };

    const r_sealed = astarFindPath(
        grid[0..], w, h,
        GridPos { x: 0, y: 0 }, GridPos { x: 1, y: 1 },
        true, &ws, path[0..],
    ) ?? unreachable;
    expect_eq(r_sealed.found, false);

    // Now place a single pillar at (1,0) only -> cannot cut diagonally from (0,0) to (1,1);
    // must go (0,0) -> (0,1) -> (1,1) with cost 20 (NOT diagonal cost 14!)
    grid[1 * w + 0] = 0;
    const r_detour = astarFindPath(
        grid[0..], w, h,
        GridPos { x: 0, y: 0 }, GridPos { x: 1, y: 1 },
        true, &ws, path[0..],
    ) ?? unreachable;
    expect_eq(r_detour.found, true);
    expect_eq(r_detour.total_cost, 20 as u32);
    expect_eq(verifyValidPath(grid[0..], w, path[0..r_detour.path_len], GridPos { x: 0, y: 0 }, GridPos { x: 1, y: 1 }, true), 20 as u32);
});

test("4. Unreachable walled-off target, blocked start/goal & trivial start == goal", () => {
    const w: usize = 5;
    const h: usize = 5;
    let grid: [25]u8 = std.mem.zeroes([25]u8);
    // Full vertical wall at x = 2
    let y: usize = 0;
    while (y < h) {
        grid[y * w + 2] = 1;
        y += 1;
    }

    let g_buf: [25]u32 = std.mem.zeroes([25]u32);
    let p_buf: [25]i32 = std.mem.zeroes([25]i32);
    let c_buf: [25]u8 = std.mem.zeroes([25]u8);
    let hn_buf: [25]u32 = std.mem.zeroes([25]u32);
    let hf_buf: [25]u32 = std.mem.zeroes([25]u32);
    let path: [25]GridPos = std.mem.zeroes([25]GridPos);

    let ws = AstarWorkspace {
        g_score: g_buf[0..],
        parent: p_buf[0..],
        closed: c_buf[0..],
        heap_nodes: hn_buf[0..],
        heap_f: hf_buf[0..],
    };

    // Walled-off goal
    const r_unreach = astarFindPath(grid[0..], w, h, GridPos { x: 0, y: 2 }, GridPos { x: 4, y: 2 }, true, &ws, path[0..]) ?? unreachable;
    expect_eq(r_unreach.found, false);
    expect_eq(r_unreach.path_len, 0 as usize);

    // Blocked start or blocked goal
    const r_blk_s = astarFindPath(grid[0..], w, h, GridPos { x: 2, y: 2 }, GridPos { x: 4, y: 2 }, true, &ws, path[0..]) ?? unreachable;
    expect_eq(r_blk_s.found, false);
    const r_blk_g = astarFindPath(grid[0..], w, h, GridPos { x: 0, y: 0 }, GridPos { x: 2, y: 0 }, true, &ws, path[0..]) ?? unreachable;
    expect_eq(r_blk_g.found, false);

    // Trivial start == goal
    const r_same = astarFindPath(grid[0..], w, h, GridPos { x: 0, y: 0 }, GridPos { x: 0, y: 0 }, false, &ws, path[0..]) ?? unreachable;
    expect_eq(r_same.found, true);
    expect_eq(r_same.path_len, 1 as usize);
    expect_eq(r_same.total_cost, 0 as u32);
});

test("5. Integer Bresenham hasLineOfSight unobstructed vs wall-blocked & bounds", () => {
    const w: usize = 8;
    const h: usize = 8;
    let grid: [64]u8 = std.mem.zeroes([64]u8);

    // Unobstructed horizontal, vertical, diagonal, and arbitrary slope
    expect_eq(hasLineOfSight(grid[0..], w, h, GridPos { x: 0, y: 0 }, GridPos { x: 7, y: 7 }), true);
    expect_eq(hasLineOfSight(grid[0..], w, h, GridPos { x: 7, y: 2 }, GridPos { x: 1, y: 5 }), true);

    // Place obstacle at (3, 3) on the main diagonal
    grid[3 * w + 3] = 1;
    expect_eq(hasLineOfSight(grid[0..], w, h, GridPos { x: 0, y: 0 }, GridPos { x: 7, y: 7 }), false);
    expect_eq(hasLineOfSight(grid[0..], w, h, GridPos { x: 7, y: 7 }, GridPos { x: 0, y: 0 }), false);
    // Offset parallel ray (0,1) -> (6,7) remains clear
    expect_eq(hasLineOfSight(grid[0..], w, h, GridPos { x: 0, y: 1 }, GridPos { x: 6, y: 7 }), true);

    // Out-of-bounds rejected
    expect_eq(hasLineOfSight(grid[0..], w, h, GridPos { x: -1, y: 0 }, GridPos { x: 2, y: 2 }), false);
    expect_eq(hasLineOfSight(grid[0..], w, h, GridPos { x: 0, y: 0 }, GridPos { x: 8, y: 0 }), false);
});

test("6. Strict null rejection for invalid dims, OOB coords, undersized workspace & small out_path", () => {
    const w: usize = 4;
    const h: usize = 4;
    let grid: [16]u8 = std.mem.zeroes([16]u8);
    let g_buf: [16]u32 = std.mem.zeroes([16]u32);
    let p_buf: [16]i32 = std.mem.zeroes([16]i32);
    let c_buf: [16]u8 = std.mem.zeroes([16]u8);
    let hn_buf: [16]u32 = std.mem.zeroes([16]u32);
    let hf_buf: [16]u32 = std.mem.zeroes([16]u32);
    let path: [16]GridPos = std.mem.zeroes([16]GridPos);

    let ws = AstarWorkspace {
        g_score: g_buf[0..],
        parent: p_buf[0..],
        closed: c_buf[0..],
        heap_nodes: hn_buf[0..],
        heap_f: hf_buf[0..],
    };

    // Zero dimensions & OOB coordinates
    expect_eq(astarFindPath(grid[0..], 0, 4, GridPos { x: 0, y: 0 }, GridPos { x: 1, y: 1 }, false, &ws, path[0..]) == null, true);
    expect_eq(astarFindPath(grid[0..], w, h, GridPos { x: -1, y: 0 }, GridPos { x: 1, y: 1 }, false, &ws, path[0..]) == null, true);
    expect_eq(astarFindPath(grid[0..], w, h, GridPos { x: 0, y: 0 }, GridPos { x: 4, y: 0 }, false, &ws, path[0..]) == null, true);

    // Undersized workspace slice
    let bad_ws = AstarWorkspace {
        g_score: g_buf[0..15],
        parent: p_buf[0..],
        closed: c_buf[0..],
        heap_nodes: hn_buf[0..],
        heap_f: hf_buf[0..],
    };
    expect_eq(astarFindPath(grid[0..], w, h, GridPos { x: 0, y: 0 }, GridPos { x: 3, y: 3 }, false, &bad_ws, path[0..]) == null, true);

    // Undersized out_path (4-neighbor (0,0)->(3,3) requires 7 cells)
    expect_eq(astarFindPath(grid[0..], w, h, GridPos { x: 0, y: 0 }, GridPos { x: 3, y: 3 }, false, &ws, path[0..6]) == null, true);
    const ok_fit = astarFindPath(grid[0..], w, h, GridPos { x: 0, y: 0 }, GridPos { x: 3, y: 3 }, false, &ws, path[0..7]) ?? unreachable;
    expect_eq(ok_fit.found, true);
    expect_eq(ok_fit.path_len, 7 as usize);
});
