// Source: cp/cap/test/test_rrt_production_deep.cp (Production deep tests for cap:rrt)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    RRT_NO_PARENT,
    RrtPoint2 as Point2,
    RrtBounds2 as Bounds2,
    CircleObstacle,
    RectObstacle,
    RrtNode,
    RrtConfig,
    pointInCircle,
    pointInRect,
    segmentIntersectsCircle,
    segmentIntersectsRect,
    isSegmentCollisionFree,
    planRrt2D,
    smoothPath,
    pathLength,
} from "cap:graph";

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

fn verifyPathCollisionFree(
    path: []const Point2,
    circles: []const CircleObstacle,
    rects: []const RectObstacle,
): bool {
    if (path.len < 2) {
        return true;
    }
    let i: usize = 0;
    while (i + 1 < path.len) {
        if (!isSegmentCollisionFree(path[i], path[i + 1], circles, rects)) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("1. Unobstructed direct path & obstacle avoidance around large circle + rect wall", () => {
    const bounds = Bounds2 { min_x: 0.0, min_y: 0.0, max_x: 10.0, max_y: 10.0 };
    const start = Point2 { x: 0.5, y: 5.0 };
    const goal = Point2 { x: 9.5, y: 5.0 };
    const empty_c: [0]CircleObstacle = [];
    const empty_r: [0]RectObstacle = [];

    let pool_arr: [64]RrtNode = std.mem.zeroes([64]RrtNode);
    let path_arr: [64]Point2 = std.mem.zeroes([64]Point2);
    let pool: []RrtNode = pool_arr[0..];
    let path_out: []Point2 = path_arr[0..];

    // 1a. Unobstructed direct connection
    const direct = planRrt2D(
        start,
        goal,
        bounds,
        empty_c[0..],
        empty_r[0..],
        RrtConfig.defaultRrt(),
        &pool,
        &path_out,
    ) ?? unreachable;
    expect_eq(direct.found, true);
    expect_eq(direct.path_len, 2);
    expect_eq(absF64(direct.total_cost - 9.0) < 1.0e-12, true);
    expect_eq(path_arr[0].x == start.x && path_arr[0].y == start.y, true);
    expect_eq(path_arr[1].x == goal.x && path_arr[1].y == goal.y, true);

    // 1b. Large circle + vertical rectangular wall blocking the center
    const circles = [CircleObstacle { cx: 3.5, cy: 5.0, radius: 1.6 }];
    const rects = [RectObstacle { min_x: 6.0, min_y: 2.0, max_x: 7.0, max_y: 8.5 }];

    let big_pool_arr: [1024]RrtNode = std.mem.zeroes([1024]RrtNode);
    let big_path_arr: [256]Point2 = std.mem.zeroes([256]Point2);
    let big_pool: []RrtNode = big_pool_arr[0..];
    let big_path: []Point2 = big_path_arr[0..];

    let cfg = RrtConfig.defaultRrt();
    cfg.step_size = 0.6;
    cfg.goal_tolerance = 0.8;
    cfg.goal_bias = 0.12;
    cfg.max_iterations = 2000;
    cfg.seed = 101;

    const res = planRrt2D(start, goal, bounds, circles[0..], rects[0..], cfg, &big_pool, &big_path) ?? unreachable;
    expect_eq(res.found, true);
    expect_eq(res.path_len >= 3, true);
    expect_eq(big_pool_arr[0].parent, RRT_NO_PARENT);
    expect_eq(absF64(big_path_arr[0].x - start.x) < 1.0e-12, true);
    expect_eq(absF64(big_path_arr[0].y - start.y) < 1.0e-12, true);
    expect_eq(absF64(big_path_arr[res.path_len - 1].x - goal.x) < 1.0e-12, true);
    expect_eq(absF64(big_path_arr[res.path_len - 1].y - goal.y) < 1.0e-12, true);
    expect_eq(verifyPathCollisionFree(big_path_arr[0..res.path_len], circles[0..], rects[0..]), true);
    expect_eq(absF64(res.total_cost - pathLength(big_path_arr[0..res.path_len])) < 1.0e-9, true);
});

test("2. RRT* rewiring produces path cost <= naive RRT with identical seed", () => {
    const bounds = Bounds2 { min_x: 0.0, min_y: 0.0, max_x: 10.0, max_y: 10.0 };
    const start = Point2 { x: 1.0, y: 1.0 };
    const goal = Point2 { x: 9.0, y: 9.0 };
    const circles = [
        CircleObstacle { cx: 5.0, cy: 5.0, radius: 1.8 },
    ];
    const rects = [
        RectObstacle { min_x: 2.0, min_y: 6.0, max_x: 4.0, max_y: 7.5 },
    ];

    let pool_rrt_arr: [600]RrtNode = std.mem.zeroes([600]RrtNode);
    let path_rrt_arr: [256]Point2 = std.mem.zeroes([256]Point2);
    let pool_rrt: []RrtNode = pool_rrt_arr[0..];
    let path_rrt: []Point2 = path_rrt_arr[0..];

    let cfg_rrt = RrtConfig.defaultRrt();
    cfg_rrt.step_size = 0.7;
    cfg_rrt.goal_tolerance = 0.8;
    cfg_rrt.goal_bias = 0.1;
    cfg_rrt.max_iterations = 800;
    cfg_rrt.seed = 2026;

    const rrt_res = planRrt2D(start, goal, bounds, circles[0..], rects[0..], cfg_rrt, &pool_rrt, &path_rrt) ?? unreachable;
    expect_eq(rrt_res.found, true);
    expect_eq(verifyPathCollisionFree(path_rrt_arr[0..rrt_res.path_len], circles[0..], rects[0..]), true);

    let pool_star_arr: [600]RrtNode = std.mem.zeroes([600]RrtNode);
    let path_star_arr: [256]Point2 = std.mem.zeroes([256]Point2);
    let pool_star: []RrtNode = pool_star_arr[0..];
    let path_star: []Point2 = path_star_arr[0..];

    let cfg_star = RrtConfig.defaultRrtStar();
    cfg_star.step_size = 0.7;
    cfg_star.goal_tolerance = 0.8;
    cfg_star.goal_bias = 0.1;
    cfg_star.rewire_radius = 2.2;
    cfg_star.max_iterations = 800;
    cfg_star.seed = 2026;

    const star_res = planRrt2D(start, goal, bounds, circles[0..], rects[0..], cfg_star, &pool_star, &path_star) ?? unreachable;
    expect_eq(star_res.found, true);
    expect_eq(verifyPathCollisionFree(path_star_arr[0..star_res.path_len], circles[0..], rects[0..]), true);
    expect_eq(star_res.total_cost <= rrt_res.total_cost + 1.0e-9, true);
});

test("3. smoothPath significantly shortens zigzag path while staying strictly collision-free", () => {
    const circles = [CircleObstacle { cx: 5.0, cy: 5.0, radius: 1.5 }];
    const rects = [RectObstacle { min_x: 4.5, min_y: 0.0, max_x: 5.5, max_y: 2.5 }];

    // Artificial redundant zigzag path passing above the circle obstacle
    const zigzag = [
        Point2 { x: 1.0, y: 5.0 },
        Point2 { x: 1.5, y: 7.5 },
        Point2 { x: 2.5, y: 8.5 },
        Point2 { x: 3.5, y: 7.5 },
        Point2 { x: 5.0, y: 8.5 },
        Point2 { x: 6.5, y: 7.5 },
        Point2 { x: 8.0, y: 8.5 },
        Point2 { x: 9.0, y: 5.0 },
    ];
    expect_eq(verifyPathCollisionFree(zigzag[0..], circles[0..], rects[0..]), true);

    let smooth_arr: [16]Point2 = std.mem.zeroes([16]Point2);
    let smooth_out: []Point2 = smooth_arr[0..];
    const s_len = smoothPath(zigzag[0..], circles[0..], rects[0..], &smooth_out);

    expect_eq(s_len >= 2, true);
    expect_eq(s_len < zigzag.len, true);
    expect_eq(verifyPathCollisionFree(smooth_arr[0..s_len], circles[0..], rects[0..]), true);

    const raw_len = pathLength(zigzag[0..]);
    const new_len = pathLength(smooth_arr[0..s_len]);
    expect_eq(new_len < raw_len - 1.0, true);
    expect_eq(smooth_arr[0].x == zigzag[0].x && smooth_arr[0].y == zigzag[0].y, true);
    expect_eq(smooth_arr[s_len - 1].x == zigzag[7].x && smooth_arr[s_len - 1].y == zigzag[7].y, true);
});

test("4. Completely enclosed goal is unreachable and safely returns found == false", () => {
    const bounds = Bounds2 { min_x: 0.0, min_y: 0.0, max_x: 10.0, max_y: 10.0 };
    const start = Point2 { x: 1.0, y: 1.0 };
    const goal = Point2 { x: 8.0, y: 8.0 };
    const empty_c: [0]CircleObstacle = [];
    // Four overlapping rectangular walls enclosing [7.0, 9.0] x [7.0, 9.0]
    const walls = [
        RectObstacle { min_x: 6.5, min_y: 6.5, max_x: 9.5, max_y: 7.0 },
        RectObstacle { min_x: 6.5, min_y: 9.0, max_x: 9.5, max_y: 9.5 },
        RectObstacle { min_x: 6.5, min_y: 6.5, max_x: 7.0, max_y: 9.5 },
        RectObstacle { min_x: 9.0, min_y: 6.5, max_x: 9.5, max_y: 9.5 },
    ];

    let pool_arr: [300]RrtNode = std.mem.zeroes([300]RrtNode);
    let path_arr: [64]Point2 = std.mem.zeroes([64]Point2);
    let pool: []RrtNode = pool_arr[0..];
    let path_out: []Point2 = path_arr[0..];

    let cfg = RrtConfig.defaultRrt();
    cfg.max_iterations = 400;
    cfg.seed = 99;

    const res = planRrt2D(start, goal, bounds, empty_c[0..], walls[0..], cfg, &pool, &path_out) ?? unreachable;
    expect_eq(res.found, false);
    expect_eq(res.path_len, 0);
    expect_eq(res.node_count >= 1, true);
});

test("5. Deterministic reproduction with identical seed", () => {
    const bounds = Bounds2 { min_x: 0.0, min_y: 0.0, max_x: 10.0, max_y: 10.0 };
    const start = Point2 { x: 0.5, y: 5.0 };
    const goal = Point2 { x: 9.5, y: 5.0 };
    const circles = [CircleObstacle { cx: 5.0, cy: 5.0, radius: 2.0 }];
    const empty_r: [0]RectObstacle = [];

    let cfg = RrtConfig.defaultRrtStar();
    cfg.step_size = 0.6;
    cfg.rewire_radius = 1.8;
    cfg.max_iterations = 400;
    cfg.seed = 0x123456789ABCDEF0;

    let pool1_arr: [450]RrtNode = std.mem.zeroes([450]RrtNode);
    let path1_arr: [128]Point2 = std.mem.zeroes([128]Point2);
    let pool1: []RrtNode = pool1_arr[0..];
    let path1: []Point2 = path1_arr[0..];

    let pool2_arr: [450]RrtNode = std.mem.zeroes([450]RrtNode);
    let path2_arr: [128]Point2 = std.mem.zeroes([128]Point2);
    let pool2: []RrtNode = pool2_arr[0..];
    let path2: []Point2 = path2_arr[0..];

    const r1 = planRrt2D(start, goal, bounds, circles[0..], empty_r[0..], cfg, &pool1, &path1) ?? unreachable;
    const r2 = planRrt2D(start, goal, bounds, circles[0..], empty_r[0..], cfg, &pool2, &path2) ?? unreachable;

    expect_eq(r1.found, r2.found);
    expect_eq(r1.node_count, r2.node_count);
    expect_eq(r1.path_len, r2.path_len);
    expect_eq(r1.iterations, r2.iterations);
    expect_eq(r1.total_cost == r2.total_cost, true);

    let i: usize = 0;
    while (i < r1.path_len) {
        expect_eq(path1_arr[i].x == path2_arr[i].x, true);
        expect_eq(path1_arr[i].y == path2_arr[i].y, true);
        i += 1;
    }
});

test("6. Geometry primitives & defensive guards (in-obstacle, out-of-bounds, bad bounds/pool, NaN)", () => {
    const c = CircleObstacle { cx: 0.0, cy: 0.0, radius: 1.0 };
    const r = RectObstacle { min_x: 2.0, min_y: 2.0, max_x: 4.0, max_y: 4.0 };
    expect_eq(pointInCircle(Point2 { x: 0.5, y: 0.5 }, c), true);
    expect_eq(pointInCircle(Point2 { x: 1.1, y: 0.0 }, c), false);
    expect_eq(pointInRect(Point2 { x: 3.0, y: 3.0 }, r), true);
    expect_eq(pointInRect(Point2 { x: 1.9, y: 3.0 }, r), false);
    expect_eq(segmentIntersectsCircle(Point2 { x: -2.0, y: 0.0 }, Point2 { x: 2.0, y: 0.0 }, c), true);
    expect_eq(segmentIntersectsCircle(Point2 { x: -2.0, y: 2.0 }, Point2 { x: 2.0, y: 2.0 }, c), false);
    expect_eq(segmentIntersectsRect(Point2 { x: 1.0, y: 3.0 }, Point2 { x: 5.0, y: 3.0 }, r), true);
    expect_eq(segmentIntersectsRect(Point2 { x: 1.0, y: 1.0 }, Point2 { x: 5.0, y: 1.0 }, r), false);

    const bounds = Bounds2 { min_x: 0.0, min_y: 0.0, max_x: 10.0, max_y: 10.0 };
    const bad_bounds = Bounds2 { min_x: 5.0, min_y: 0.0, max_x: 5.0, max_y: 10.0 };
    const start = Point2 { x: 1.0, y: 1.0 };
    const goal = Point2 { x: 9.0, y: 9.0 };
    const circles = [CircleObstacle { cx: 1.0, cy: 1.0, radius: 0.5 }];
    const empty_c: [0]CircleObstacle = [];
    const empty_r: [0]RectObstacle = [];

    let pool_arr: [32]RrtNode = std.mem.zeroes([32]RrtNode);
    let path_arr: [32]Point2 = std.mem.zeroes([32]Point2);
    let pool: []RrtNode = pool_arr[0..];
    let path_out: []Point2 = path_arr[0..];
    let tiny_pool_arr: [1]RrtNode = std.mem.zeroes([1]RrtNode);
    let tiny_pool: []RrtNode = tiny_pool_arr[0..];
    let tiny_path_arr: [1]Point2 = std.mem.zeroes([1]Point2);
    let tiny_path: []Point2 = tiny_path_arr[0..];

    // Start inside obstacle
    expect_eq(planRrt2D(start, goal, bounds, circles[0..], empty_r[0..], RrtConfig.defaultRrt(), &pool, &path_out) == null, true);
    // Goal out of bounds
    expect_eq(planRrt2D(start, Point2 { x: 12.0, y: 5.0 }, bounds, empty_c[0..], empty_r[0..], RrtConfig.defaultRrt(), &pool, &path_out) == null, true);
    // Invalid bounds
    expect_eq(planRrt2D(start, goal, bad_bounds, empty_c[0..], empty_r[0..], RrtConfig.defaultRrt(), &pool, &path_out) == null, true);
    // Pool or path buffer too small
    expect_eq(planRrt2D(start, goal, bounds, empty_c[0..], empty_r[0..], RrtConfig.defaultRrt(), &tiny_pool, &path_out) == null, true);
    expect_eq(planRrt2D(start, goal, bounds, empty_c[0..], empty_r[0..], RrtConfig.defaultRrt(), &pool, &tiny_path) == null, true);
    // NaN coordinate
    const zero_f: f64 = 0.0;
    const nan_f: f64 = zero_f / zero_f;
    expect_eq(planRrt2D(Point2 { x: nan_f, y: 1.0 }, goal, bounds, empty_c[0..], empty_r[0..], RrtConfig.defaultRrt(), &pool, &path_out) == null, true);
    // Invalid config (step_size <= 0)
    let bad_cfg = RrtConfig.defaultRrt();
    bad_cfg.step_size = 0.0;
    expect_eq(planRrt2D(start, goal, bounds, empty_c[0..], empty_r[0..], bad_cfg, &pool, &path_out) == null, true);
});
