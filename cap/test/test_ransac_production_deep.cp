// Source: cp/cap/test/test_ransac_production_deep.cp (Production deep tests for cap:ransac)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    RansacConfig,
    Point2,
    Point3,
    Line2Model,
    Circle2Model,
    Plane3Model,
    Linear1DModel,
    ransacFitLine2D,
    ransacFitCircle2D,
    ransacFitPlane3D,
    ransacFitLinear1D,
} from "cap:ml";

fn absDiff(a: f64, b: f64): f64 {
    const d = a - b;
    return if (d < 0.0) 0.0 - d else d;
}

test("1. 2D line RANSAC with 40% heavy outliers (oblique line & vertical line x = 5)", () => {
    // 15 inliers on 3x - 4y + 10 = 0 => canonical (0.6, -0.8, 2.0) + 10 heavy outliers (40% outliers)
    let pts: [25]Point2 = std.mem.zeroes([25]Point2);
    let i: usize = 0;
    while (i < 15) {
        const x = (i as f64) * 1.2 - 5.0;
        const y = (3.0 * x + 10.0) / 4.0;
        pts[i] = Point2 { x: x, y: y };
        i += 1;
    }
    // 10 severe outliers far off the line
    while (i < 25) {
        const k = (i - 15) as f64;
        pts[i] = Point2 { x: k * 2.0 - 8.0, y: if (i % 2 == 0) 40.0 + k * 3.0 else -35.0 - k * 2.0 };
        i += 1;
    }

    let mask: [25]u8 = std.mem.zeroes([25]u8);
    const cfg = RansacConfig {
        threshold: 0.05,
        max_iterations: 200,
        seed: 101,
        min_inliers: 10,
        refine: true,
    };
    const m = ransacFitLine2D(pts[0..], cfg, mask[0..]) ?? unreachable;
    expect_eq(m.inlier_count, 15 as usize);
    expect_eq(m.rmse < 1.0e-11, true);
    expect_eq(absDiff(m.a, 0.6) < 1.0e-11, true);
    expect_eq(absDiff(m.b, -0.8) < 1.0e-11, true);
    expect_eq(absDiff(m.c, 2.0) < 1.0e-11, true);

    i = 0;
    while (i < 25) {
        expect_eq(mask[i], if (i < 15) (1 as u8) else (0 as u8));
        i += 1;
    }

    // Vertical line x = 5.0 (canonical: a = 1, b = 0, c = -5) with 40% outliers
    let vpts: [15]Point2 = std.mem.zeroes([15]Point2);
    i = 0;
    while (i < 9) {
        vpts[i] = Point2 { x: 5.0, y: (i as f64) * 2.0 - 8.0 };
        i += 1;
    }
    while (i < 15) {
        vpts[i] = Point2 { x: if (i % 2 == 0) -20.0 else 30.0, y: (i as f64) * 3.0 };
        i += 1;
    }
    let vmask: [15]u8 = std.mem.zeroes([15]u8);
    const vm = ransacFitLine2D(vpts[0..], RansacConfig.default(0.05), vmask[0..]) ?? unreachable;
    expect_eq(vm.inlier_count, 9 as usize);
    expect_eq(absDiff(vm.a, 1.0) < 1.0e-11, true);
    expect_eq(absDiff(vm.b, 0.0) < 1.0e-11, true);
    expect_eq(absDiff(vm.c, -5.0) < 1.0e-11, true);
});

test("2. 2D circle RANSAC with 35% outliers recovers exact center, radius & inlier mask", () => {
    // 13 inliers on circle (cx=3.5, cy=-2.0, r=4.5) + 7 outliers (35%)
    let pts: [20]Point2 = std.mem.zeroes([20]Point2);
    const two_pi: f64 = 6.28318530717958647692;
    let i: usize = 0;
    while (i < 13) {
        const ang = two_pi * ((i as f64) / 13.0);
        pts[i] = Point2 {
            x: 3.5 + 4.5 * std.math.cos(ang),
            y: -2.0 + 4.5 * std.math.sin(ang),
        };
        i += 1;
    }
    pts[13] = Point2 { x: 3.5, y: -2.0 }; // center itself (radial err = 4.5)
    pts[14] = Point2 { x: 25.0, y: 18.0 };
    pts[15] = Point2 { x: -19.0, y: 12.0 };
    pts[16] = Point2 { x: 14.0, y: -22.0 };
    pts[17] = Point2 { x: -10.0, y: -15.0 };
    pts[18] = Point2 { x: 0.0, y: 20.0 };
    pts[19] = Point2 { x: 30.0, y: -30.0 };

    let mask: [20]u8 = std.mem.zeroes([20]u8);
    const cfg = RansacConfig {
        threshold: 0.05,
        max_iterations: 300,
        seed: 2026,
        min_inliers: 10,
        refine: true,
    };
    const cm = ransacFitCircle2D(pts[0..], cfg, mask[0..]) ?? unreachable;
    expect_eq(cm.inlier_count, 13 as usize);
    expect_eq(cm.rmse < 1.0e-11, true);
    expect_eq(absDiff(cm.cx, 3.5) < 1.0e-11, true);
    expect_eq(absDiff(cm.cy, -2.0) < 1.0e-11, true);
    expect_eq(absDiff(cm.radius, 4.5) < 1.0e-11, true);

    i = 0;
    while (i < 20) {
        expect_eq(mask[i], if (i < 13) (1 as u8) else (0 as u8));
        i += 1;
    }
});

test("3. 3D plane RANSAC with 35% outliers & 3x3 covariance eigenvector refinement", () => {
    // Plane 2x - y + 2z - 9 = 0 => canonical (2/3, -1/3, 2/3, -3.0)
    // 13 inliers + 7 outliers (35%)
    let pts: [20]Point3 = std.mem.zeroes([20]Point3);
    let i: usize = 0;
    while (i < 13) {
        const x = ((i % 5) as f64) * 2.0 - 3.0;
        const y = ((i / 3) as f64) * 2.5 - 2.0;
        const z = (9.0 - 2.0 * x + y) * 0.5;
        pts[i] = Point3 { x: x, y: y, z: z };
        i += 1;
    }
    while (i < 20) {
        const k = (i - 13) as f64;
        pts[i] = Point3 { x: k * 3.0 - 5.0, y: k * 2.0, z: if (i % 2 == 0) 40.0 + k else -30.0 - k };
        i += 1;
    }

    let mask: [20]u8 = std.mem.zeroes([20]u8);
    const cfg = RansacConfig {
        threshold: 0.05,
        max_iterations: 300,
        seed: 777,
        min_inliers: 10,
        refine: true,
    };
    const pm = ransacFitPlane3D(pts[0..], cfg, mask[0..]) ?? unreachable;
    expect_eq(pm.inlier_count, 13 as usize);
    expect_eq(pm.rmse < 1.0e-11, true);
    expect_eq(absDiff(pm.a, 2.0 / 3.0) < 1.0e-11, true);
    expect_eq(absDiff(pm.b, -1.0 / 3.0) < 1.0e-11, true);
    expect_eq(absDiff(pm.c, 2.0 / 3.0) < 1.0e-11, true);
    expect_eq(absDiff(pm.d, -3.0) < 1.0e-11, true);

    i = 0;
    while (i < 20) {
        expect_eq(mask[i], if (i < 13) (1 as u8) else (0 as u8));
        i += 1;
    }
});

test("4. 1D linear regression RANSAC resists extreme outliers & refines via OLS", () => {
    // True line: y = 2.5 * x - 4.0 with tiny balanced noise +/- 0.01 on 14 inliers + 6 extreme outliers
    let xs: [20]f64 = std.mem.zeroes([20]f64);
    let ys: [20]f64 = std.mem.zeroes([20]f64);
    let i: usize = 0;
    while (i < 14) {
        const x = (i as f64) - 6.5;
        const noise = if (i % 2 == 0) 0.01 else -0.01;
        xs[i] = x;
        ys[i] = 2.5 * x - 4.0 + noise;
        i += 1;
    }
    xs[14] = -3.0; ys[14] = 5000.0;
    xs[15] = 0.0;  ys[15] = -3000.0;
    xs[16] = 4.0;  ys[16] = 9000.0;
    xs[17] = -5.0; ys[17] = -8000.0;
    xs[18] = 2.0;  ys[18] = 1234.0;
    xs[19] = 6.0;  ys[19] = -4321.0;

    let mask: [20]u8 = std.mem.zeroes([20]u8);
    const cfg = RansacConfig {
        threshold: 0.1,
        max_iterations: 200,
        seed: 55,
        min_inliers: 10,
        refine: true,
    };
    const lm = ransacFitLinear1D(xs[0..], ys[0..], cfg, mask[0..]) ?? unreachable;
    expect_eq(lm.inlier_count, 14 as usize);
    expect_eq(absDiff(lm.slope, 2.5) < 0.005, true);
    expect_eq(absDiff(lm.intercept, -4.0) < 0.005, true);
    expect_eq(lm.rmse < 0.015, true);

    i = 0;
    while (i < 20) {
        expect_eq(mask[i], if (i < 14) (1 as u8) else (0 as u8));
        i += 1;
    }
});

test("5. Deterministic reproducibility with identical seed across repeated runs", () => {
    const pts: [8]Point2 = [
        Point2 { x: 0.0, y: 0.0 },
        Point2 { x: 1.0, y: 1.02 },
        Point2 { x: 2.0, y: 1.98 },
        Point2 { x: 3.0, y: 3.01 },
        Point2 { x: 4.0, y: 3.99 },
        Point2 { x: 1.0, y: 10.0 },
        Point2 { x: 2.0, y: -15.0 },
        Point2 { x: 3.0, y: 20.0 },
    ];
    let mask1: [8]u8 = std.mem.zeroes([8]u8);
    let mask2: [8]u8 = std.mem.zeroes([8]u8);
    const cfg = RansacConfig { threshold: 0.1, max_iterations: 50, seed: 314159, min_inliers: 3, refine: false };

    const m1 = ransacFitLine2D(pts[0..], cfg, mask1[0..]) ?? unreachable;
    const m2 = ransacFitLine2D(pts[0..], cfg, mask2[0..]) ?? unreachable;
    expect_eq(m1.a, m2.a);
    expect_eq(m1.b, m2.b);
    expect_eq(m1.c, m2.c);
    expect_eq(m1.inlier_count, m2.inlier_count);
    let i: usize = 0;
    while (i < 8) {
        expect_eq(mask1[i], mask2[i]);
        i += 1;
    }
});

test("6. Boundary guards: too few points, collinear degeneracy, bad threshold, undersized mask & NaN", () => {
    const ok_cfg = RansacConfig.default(0.1);
    const bad_thresh = RansacConfig.default(0.0);
    const bad_iter = RansacConfig { threshold: 0.1, max_iterations: 0, seed: 1, min_inliers: 0, refine: true };
    const high_min = RansacConfig { threshold: 0.1, max_iterations: 20, seed: 1, min_inliers: 10, refine: true };

    const one_p2: [1]Point2 = [Point2 { x: 0.0, y: 0.0 }];
    const two_p2: [2]Point2 = [Point2 { x: 0.0, y: 0.0 }, Point2 { x: 1.0, y: 1.0 }];
    const col_p2: [4]Point2 = [
        Point2 { x: 0.0, y: 0.0 },
        Point2 { x: 1.0, y: 1.0 },
        Point2 { x: 2.0, y: 2.0 },
        Point2 { x: 3.0, y: 3.0 },
    ];
    const nan_p2: [3]Point2 = [
        Point2 { x: 0.0, y: 0.0 },
        Point2 { x: std.math.nan(f64), y: 1.0 },
        Point2 { x: 2.0, y: 0.0 },
    ];
    const col_p3: [4]Point3 = [
        Point3 { x: 0.0, y: 0.0, z: 0.0 },
        Point3 { x: 1.0, y: 1.0, z: 1.0 },
        Point3 { x: 2.0, y: 2.0, z: 2.0 },
        Point3 { x: 3.0, y: 3.0, z: 3.0 },
    ];
    let mask4: [4]u8 = std.mem.zeroes([4]u8);
    let mask1: [1]u8 = std.mem.zeroes([1]u8);

    // Too few points / bad config / undersized mask / NaN
    expect_eq(ransacFitLine2D(one_p2[0..], ok_cfg, mask4[0..]) == null, true);
    expect_eq(ransacFitLine2D(two_p2[0..], bad_thresh, mask4[0..]) == null, true);
    expect_eq(ransacFitLine2D(two_p2[0..], bad_iter, mask4[0..]) == null, true);
    expect_eq(ransacFitLine2D(two_p2[0..], ok_cfg, mask1[0..]) == null, true);
    expect_eq(ransacFitLine2D(nan_p2[0..], ok_cfg, mask4[0..]) == null, true);
    expect_eq(ransacFitLine2D(col_p2[0..], high_min, mask4[0..]) == null, true);

    // Collinear points cannot form a circle or a 3D plane
    expect_eq(ransacFitCircle2D(two_p2[0..], ok_cfg, mask4[0..]) == null, true);
    expect_eq(ransacFitCircle2D(col_p2[0..], ok_cfg, mask4[0..]) == null, true);
    expect_eq(ransacFitPlane3D(col_p3[0..], ok_cfg, mask4[0..]) == null, true);

    // 1D regression mismatched lengths & vertical identical x values rejected
    const xs2: [2]f64 = [1.0, 2.0];
    const ys3: [3]f64 = [1.0, 2.0, 3.0];
    const same_x: [3]f64 = [5.0, 5.0, 5.0];
    expect_eq(ransacFitLinear1D(xs2[0..], ys3[0..], ok_cfg, mask4[0..]) == null, true);
    expect_eq(ransacFitLinear1D(same_x[0..], ys3[0..], ok_cfg, mask4[0..]) == null, true);
});
