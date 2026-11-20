// Source: cp/cap/test/test_pathfinding_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:pathfinding & cap.pathfinding

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    PathError,
    HeuristicKind,
    GridCoord,
    PathResult,
    GridMap,
} from "cap:pathfinding";

const SQRT2: f64 = 1.4142135623730950488;

fn approxEq(a: f64, b: f64, eps: f64 = 0.000001): bool {
    return @abs(a - b) <= eps;
}

fn testMazeAStarAndBoundaryErrors(): !void {
    const alloc = std.testing.allocator;

    // Invalid dimensions guard
    let bad_dim = false;
    * = GridMap.init(alloc, 0, 5) catch e => {
        if (e == PathError.InvalidDimensions) bad_dim = true;
    };
    expect_eq(bad_dim, true);

    // 6x5 maze:
    // S . # . . .
    // # . # . # .
    // . . # . # .
    // . # . . # .
    // . . . # G .
    let grid = GridMap.init(alloc, 6, 5)?;
    defer grid.deinit();

    grid.setBlocked(2, 0, true)?;
    grid.setBlocked(0, 1, true)?;
    grid.setBlocked(2, 1, true)?;
    grid.setBlocked(4, 1, true)?;
    grid.setBlocked(2, 2, true)?;
    grid.setBlocked(4, 2, true)?;
    grid.setBlocked(1, 3, true)?;
    grid.setBlocked(4, 3, true)?;
    grid.setBlocked(3, 4, true)?;

    // Out-of-bounds guard
    let oob_err = false;
    grid.setBlocked(6, 0, true) catch e => {
        if (e == PathError.OutOfBounds) oob_err = true;
    };
    expect_eq(oob_err, true);

    // 4-neighbor A* must wind around walls:
    // (0,0)->(1,0)->(1,1)->(1,2)->(0,2)->(0,3)->(0,4)->(1,4)->(2,4)->(2,3)->(3,3)->(3,2)->(3,1)->(3,0)->(4,0)->(5,0)->(5,1)->(5,2)->(5,3)->(5,4)->(4,4)
    // Total steps = 20
    let res = grid.findPathAStar(
        alloc,
        GridCoord.init(0, 0),
        GridCoord.init(4, 4),
        false,
        HeuristicKind.Manhattan,
    )?;
    defer res.deinit();

    expect_eq(res.coords[0].eql(GridCoord.init(0, 0)), true);
    expect_eq(res.coords[res.coords.len - 1].eql(GridCoord.init(4, 4)), true);
    expect_eq(res.coords.len, 21);
    expect_eq(approxEq(res.total_cost, 20.0), true);
    expect_eq(res.nodes_expanded > 0, true);

    // Verify every step is valid, unblocked, and 4-connected
    let i: usize = 0;
    while (i < res.coords.len) : (i += 1) {
        const c = res.coords[i];
        expect_eq(grid.isBlocked(c.x, c.y)?, false);
        if (i + 1 < res.coords.len) {
            const n = res.coords[i + 1];
            const dx = if (n.x > c.x) n.x - c.x else c.x - n.x;
            const dy = if (n.y > c.y) n.y - c.y else c.y - n.y;
            expect_eq(dx + dy, 1);
        }
    }

    // BlockedEndpoint when start or goal is blocked
    let blocked_start = false;
    * = grid.findPathAStar(alloc, GridCoord.init(2, 0), GridCoord.init(4, 4), false, HeuristicKind.Manhattan) catch e => {
        if (e == PathError.BlockedEndpoint) blocked_start = true;
    };
    expect_eq(blocked_start, true);

    let blocked_goal = false;
    * = grid.findPathAStar(alloc, GridCoord.init(0, 0), GridCoord.init(3, 4), false, HeuristicKind.Manhattan) catch e => {
        if (e == PathError.BlockedEndpoint) blocked_goal = true;
    };
    expect_eq(blocked_goal, true);

    // Completely isolate (5, 4) and (4, 4) by blocking (5, 3)
    grid.setBlocked(5, 3, true)?;
    let no_path = false;
    * = grid.findPathAStar(alloc, GridCoord.init(0, 0), GridCoord.init(4, 4), false, HeuristicKind.Manhattan) catch e => {
        if (e == PathError.NoPathFound) no_path = true;
    };
    expect_eq(no_path, true);
}

fn testCornerCuttingPreventionAndSwampWeights(): !void {
    const alloc = std.testing.allocator;

    // 1. Strict Corner-Cutting Prevention on a 3x3 grid:
    // . # .
    // # . .
    // . . .
    // From (0, 0) to (1, 1): diagonal move is FORBIDDEN because both (1, 0) and (0, 1) are blocked.
    // Even if only (1, 0) is blocked, diagonal (0, 0) -> (1, 1) MUST be forbidden.
    let g_corner = GridMap.init(alloc, 3, 3)?;
    defer g_corner.deinit();
    g_corner.setBlocked(1, 0, true)?;

    let res_corner = g_corner.findPathAStar(
        alloc,
        GridCoord.init(0, 0),
        GridCoord.init(1, 1),
        true,
        HeuristicKind.Octile,
    )?;
    defer res_corner.deinit();

    // Must go (0,0) -> (0,1) -> (1,1) with cost 2.0 (NOT direct diagonal cost SQRT2)
    expect_eq(res_corner.coords.len, 3);
    expect_eq(res_corner.coords[1].eql(GridCoord.init(0, 1)), true);
    expect_eq(approxEq(res_corner.total_cost, 2.0), true);

    // Now block (0, 1) too -> (0, 0) is completely sealed off even with allow_diagonal = true!
    g_corner.setBlocked(0, 1, true)?;
    let corner_sealed = false;
    * = g_corner.findPathAStar(alloc, GridCoord.init(0, 0), GridCoord.init(1, 1), true, HeuristicKind.Octile) catch e => {
        if (e == PathError.NoPathFound) corner_sealed = true;
    };
    expect_eq(corner_sealed, true);

    // 2. High-cost swamp terrain avoidance:
    // 5x3 grid: direct horizontal row y=1 has swamp cells (1,1), (2,1), (3,1) with weight 10.0.
    // Route from (0,1) to (4,1) should detour via y=0 (cost ~ 2*SQRT2 + 2 = 4.828) instead of through swamp (~31.0).
    let g_swamp = GridMap.init(alloc, 5, 3)?;
    defer g_swamp.deinit();

    // InvalidCost guard (< 1.0)
    let bad_cost = false;
    g_swamp.setCost(1, 1, 0.5) catch e => {
        if (e == PathError.InvalidCost) bad_cost = true;
    };
    expect_eq(bad_cost, true);

    g_swamp.setCost(1, 1, 10.0)?;
    g_swamp.setCost(2, 1, 10.0)?;
    g_swamp.setCost(3, 1, 10.0)?;
    expect_eq(approxEq(g_swamp.getCost(2, 1)?, 10.0), true);

    let res_swamp = g_swamp.findPathAStar(
        alloc,
        GridCoord.init(0, 1),
        GridCoord.init(4, 1),
        true,
        HeuristicKind.Euclidean,
    )?;
    defer res_swamp.deinit();

    // Verify none of the swamp cells were stepped on
    for (pt of res_swamp.coords) {
        const is_swamp = pt.y == 1 && (pt.x == 1 || pt.x == 2 || pt.x == 3);
        expect_eq(is_swamp, false);
    }
    expect_eq(res_swamp.total_cost < 6.0, true);
}

fn testLineOfSightAndSmoothPath(): !void {
    const alloc = std.testing.allocator;

    // 8x6 open grid with a single obstacle block at (3, 2)
    let grid = GridMap.init(alloc, 8, 6)?;
    defer grid.deinit();
    grid.setBlocked(3, 2, true)?;

    // Unobstructed diagonal/arbitrary LOS
    expect_eq(grid.hasLineOfSight(GridCoord.init(0, 0), GridCoord.init(7, 0)), true);
    expect_eq(grid.hasLineOfSight(GridCoord.init(0, 0), GridCoord.init(2, 2)), true);

    // Obstructed LOS passing through (3, 2)
    expect_eq(grid.hasLineOfSight(GridCoord.init(0, 2), GridCoord.init(6, 2)), false);
    expect_eq(grid.hasLineOfSight(GridCoord.init(1, 0), GridCoord.init(5, 4)), false);

    // Corner-grazing check: block (1, 0), check LOS from (0, 0) to (2, 2) or (0, 0) to (1, 1)
    let g2 = GridMap.init(alloc, 4, 4)?;
    defer g2.deinit();
    g2.setBlocked(1, 0, true)?;
    expect_eq(g2.hasLineOfSight(GridCoord.init(0, 0), GridCoord.init(1, 1)), false);
    expect_eq(g2.hasLineOfSight(GridCoord.init(0, 0), GridCoord.init(2, 2)), false);

    // Staircase path on an open 8x8 grid should smooth to just [start, goal]
    let open_grid = GridMap.init(alloc, 8, 8)?;
    defer open_grid.deinit();

    let raw_res = open_grid.findPathAStar(
        alloc,
        GridCoord.init(0, 0),
        GridCoord.init(6, 4),
        false,
        HeuristicKind.Manhattan,
    )?;
    defer raw_res.deinit();
    expect_eq(raw_res.coords.len, 11);

    const smoothed = open_grid.smoothPath(alloc, raw_res.coords)?;
    defer alloc.free(smoothed);

    expect_eq(smoothed.len, 2);
    expect_eq(smoothed[0].eql(GridCoord.init(0, 0)), true);
    expect_eq(smoothed[1].eql(GridCoord.init(6, 4)), true);

    // Path around a wall should smooth to minimal turning waypoints with valid LOS per segment
    let wall_grid = GridMap.init(alloc, 8, 6)?;
    defer wall_grid.deinit();
    wall_grid.setBlocked(3, 0, true)?;
    wall_grid.setBlocked(3, 1, true)?;
    wall_grid.setBlocked(3, 2, true)?;

    let wall_res = wall_grid.findPathAStar(
        alloc,
        GridCoord.init(0, 0),
        GridCoord.init(6, 0),
        false,
        HeuristicKind.Manhattan,
    )?;
    defer wall_res.deinit();

    const wall_smoothed = wall_grid.smoothPath(alloc, wall_res.coords)?;
    defer alloc.free(wall_smoothed);

    expect_eq(wall_smoothed.len < wall_res.coords.len, true);
    expect_eq(wall_smoothed[0].eql(GridCoord.init(0, 0)), true);
    expect_eq(wall_smoothed[wall_smoothed.len - 1].eql(GridCoord.init(6, 0)), true);

    let seg: usize = 0;
    while (seg + 1 < wall_smoothed.len) : (seg += 1) {
        expect_eq(wall_grid.hasLineOfSight(wall_smoothed[seg], wall_smoothed[seg + 1]), true);
    }
}

fn testDijkstraDistanceFieldZeroLeak(): !void {
    const alloc = std.testing.allocator;

    // 5x5 grid with a sealed pocket at (4, 4)
    let grid = GridMap.init(alloc, 5, 5)?;
    defer grid.deinit();

    grid.setBlocked(2, 0, true)?;
    grid.setBlocked(2, 1, true)?;
    grid.setBlocked(2, 2, true)?;
    grid.setBlocked(3, 3, true)?;
    grid.setBlocked(4, 3, true)?;
    grid.setBlocked(3, 4, true)?;

    const goal = GridCoord.init(0, 0);
    const field = grid.computeDistanceField(alloc, goal, true)?;
    defer alloc.free(field);

    // Goal cell has distance 0
    expect_eq(approxEq(field[0], 0.0), true);

    // Sealed pocket (4, 4) and blocked cell (2, 0) must be +inf
    expect_eq(std.math.isInf(field[4 * 5 + 4]), true);
    expect_eq(std.math.isInf(field[0 * 5 + 2]), true);

    // Multi-origin optimality check: distance field at multiple reachable starts
    // must match A* total_cost identically
    const starts: [3]GridCoord = [
        GridCoord.init(4, 0),
        GridCoord.init(1, 4),
        GridCoord.init(2, 4),
    ];

    for (s of starts) {
        let astar_res = grid.findPathAStar(alloc, s, goal, true, HeuristicKind.Octile)?;
        defer astar_res.deinit();
        const field_dist = field[s.y * 5 + s.x];
        expect_eq(approxEq(field_dist, astar_res.total_cost), true);
    }
}

fn testCapPathfindingLazyMount(): !void {
    const alloc = std.testing.allocator;

    let grid = cap.pathfinding.GridMap.init(alloc, 6, 6)?;
    defer grid.deinit();

    grid.setBlocked(2, 1, true)?;
    grid.setBlocked(2, 2, true)?;
    grid.setBlocked(2, 3, true)?;

    let res: cap.pathfinding.PathResult = grid.findPathAStar(
        alloc,
        cap.pathfinding.GridCoord.init(0, 2),
        cap.pathfinding.GridCoord.init(4, 2),
        true,
        cap.pathfinding.HeuristicKind.Octile,
    )?;
    defer res.deinit();

    expect_eq(res.coords.len > 2, true);
    expect_eq(res.coords[0].eql(cap.pathfinding.GridCoord.init(0, 2)), true);
    expect_eq(res.coords[res.coords.len - 1].eql(cap.pathfinding.GridCoord.init(4, 2)), true);

    const smoothed = grid.smoothPath(alloc, res.coords)?;
    defer alloc.free(smoothed);
    expect_eq(smoothed.len <= res.coords.len, true);
}

test("1. GridMap Maze A* Optimal Path, BlockedEndpoint, NoPathFound & Boundary Guards") {
    testMazeAStarAndBoundaryErrors() catch unreachable;
}

test("2. 8-Direction Corner-Cutting Prevention & High-Cost Swamp Detour") {
    testCornerCuttingPreventionAndSwampWeights() catch unreachable;
}

test("3. Supercover Bresenham Line-of-Sight & Greedy smoothPath Straightening") {
    testLineOfSightAndSmoothPath() catch unreachable;
}

test("4. Dijkstra Distance Field Multi-Origin Optimality & Zero-Leak Verification") {
    testDijkstraDistanceFieldZeroLeak() catch unreachable;
}

test("5. cap.pathfinding Automatic Dynamic Lazy Mounting Verification") {
    testCapPathfindingLazyMount() catch unreachable;
}
