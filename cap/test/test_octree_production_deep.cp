// Source: cp/cap/test/test_octree_production_deep.cp (Production deep tests for cap:octree)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    OCTREE_NO_INDEX,
    OCTREE_DEFAULT_LEAF_CAP,
    OCTREE_DEFAULT_MAX_DEPTH,
    Vec3,
    Aabb3,
    Ray3,
    rayIntersectsAabb,
    OctreePoint,
    OctreeEntry,
    OctreeNode,
    Octree,
} from "cap:octree";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const d = a - b;
    return (if (d < 0.0) -d else d) <= eps;
}

fn sortU32(buf: *[]u32, len: usize) {
    let i: usize = 1;
    while (i < len) {
        const key = buf.*[i];
        let j: usize = i;
        while (j > 0 && buf.*[j - 1] > key) {
            buf.*[j] = buf.*[j - 1];
            j -= 1;
        }
        buf.*[j] = key;
        i += 1;
    }
}

fn nextLcg(state: *u32): f64 {
    state.* = state.* *% 1664525 +% 1013904223;
    return ((state.* >> 8) as f64) / 16777216.0;
}

test("1. Vec3 & Aabb3 geometry primitives and 8-octant subdivision invariants", () => {
    const box3 = Aabb3.init(Vec3.init(0.0, 0.0, 0.0), Vec3.init(10.0, 20.0, 30.0)) ?? unreachable;
    const c = box3.center();
    expect_eq(approxEq(c.x, 5.0, 1e-9), true);
    expect_eq(approxEq(c.y, 10.0, 1e-9), true);
    expect_eq(approxEq(c.z, 15.0, 1e-9), true);

    expect_eq(box3.containsPoint(Vec3.init(0.0, 0.0, 0.0)), true);
    expect_eq(box3.containsPoint(Vec3.init(10.0, 20.0, 30.0)), true);
    expect_eq(box3.containsPoint(Vec3.init(-0.1, 5.0, 5.0)), false);

    // Distance squared to interior, face, and corner points
    expect_eq(approxEq(box3.distanceSqToPoint(Vec3.init(5.0, 10.0, 15.0)), 0.0, 1e-9), true);
    expect_eq(approxEq(box3.distanceSqToPoint(Vec3.init(13.0, 10.0, 15.0)), 9.0, 1e-9), true);
    expect_eq(approxEq(box3.distanceSqToPoint(Vec3.init(-3.0, -4.0, 0.0)), 25.0, 1e-9), true);

    // AABB overlap tests
    const hit_box = Aabb3.init(Vec3.init(10.0, 0.0, 0.0), Vec3.init(15.0, 5.0, 5.0)) ?? unreachable;
    const miss_box = Aabb3.init(Vec3.init(10.1, 0.0, 0.0), Vec3.init(15.0, 5.0, 5.0)) ?? unreachable;
    expect_eq(box3.intersectsAabb(hit_box), true);
    expect_eq(box3.intersectsAabb(miss_box), false);

    // Verify all 8 octants map their child centers back to the exact octant index
    let oct: usize = 0;
    while (oct < 8) {
        const child = box3.childAabb(oct);
        expect_eq(box3.octantIndex(child.center()), oct);
        expect_eq(child.containsPoint(child.center()), true);
        oct += 1;
    }
});

test("2. Multi-level automatic subdivision on 200 3D points & clear() reuse", () => {
    const world = Aabb3.init(Vec3.init(0.0, 0.0, 0.0), Vec3.init(100.0, 100.0, 100.0)) ?? unreachable;
    let nodes: [512]OctreeNode = std.mem.zeroes([512]OctreeNode);
    let entries: [256]OctreeEntry = std.mem.zeroes([256]OctreeEntry);

    let tree = Octree.init(world, nodes[0..], entries[0..], 4, 6) ?? unreachable;
    expect_eq(tree.count(), 0 as usize);
    expect_eq(tree.nodeCount(), 1 as usize);

    let rng: u32 = 42;
    let i: u32 = 0;
    while (i < 200) {
        const px = nextLcg(&rng) * 100.0;
        const py = nextLcg(&rng) * 100.0;
        const pz = nextLcg(&rng) * 100.0;
        expect_eq(tree.insert(OctreePoint.init(px, py, pz, i)), true);
        i += 1;
    }

    expect_eq(tree.count(), 200 as usize);
    // Must have subdivided multiple levels beyond root (1 + 8*k)
    expect_eq(tree.nodeCount() > 9, true);
    expect_eq(((tree.nodeCount() - 1) % 8), 0 as usize);

    // clear() resets counts and allows full reuse
    tree.clear();
    expect_eq(tree.count(), 0 as usize);
    expect_eq(tree.nodeCount(), 1 as usize);
    expect_eq(tree.insert(OctreePoint.init(50.0, 50.0, 50.0, 999)), true);
    expect_eq(tree.count(), 1 as usize);
});

test("3. queryBox & querySphere match brute-force ground truth with 100% accuracy", () => {
    const world = Aabb3.init(Vec3.init(-50.0, -50.0, -50.0), Vec3.init(50.0, 50.0, 50.0)) ?? unreachable;
    let nodes: [512]OctreeNode = std.mem.zeroes([512]OctreeNode);
    let entries: [200]OctreeEntry = std.mem.zeroes([200]OctreeEntry);
    let pts: [180]OctreePoint = std.mem.zeroes([180]OctreePoint);

    let tree = Octree.init(world, nodes[0..], entries[0..], 4, OCTREE_DEFAULT_MAX_DEPTH) ?? unreachable;

    let rng: u32 = 1337;
    let i: usize = 0;
    while (i < 180) {
        const px = nextLcg(&rng) * 100.0 - 50.0;
        const py = nextLcg(&rng) * 100.0 - 50.0;
        const pz = nextLcg(&rng) * 100.0 - 50.0;
        pts[i] = OctreePoint.init(px, py, pz, i as u32);
        expect_eq(tree.insert(pts[i]), true);
        i += 1;
    }

    // Box query verification
    const qbox = Aabb3.init(Vec3.init(-15.0, -10.0, 5.0), Vec3.init(25.0, 30.0, 40.0)) ?? unreachable;
    let got_box: [180]u32 = std.mem.zeroes([180]u32);
    let exp_box: [180]u32 = std.mem.zeroes([180]u32);
    let got_box_s: []u32 = got_box[0..];
    let exp_box_s: []u32 = exp_box[0..];

    const n_got_box = tree.queryBox(qbox, &got_box_s);
    let n_exp_box: usize = 0;
    i = 0;
    while (i < 180) {
        if (qbox.containsPoint(pts[i].pos)) {
            exp_box[n_exp_box] = pts[i].id;
            n_exp_box += 1;
        }
        i += 1;
    }

    expect_eq(n_got_box, n_exp_box);
    expect_eq(n_got_box > 0, true);
    sortU32(&got_box_s, n_got_box);
    sortU32(&exp_box_s, n_exp_box);
    i = 0;
    while (i < n_exp_box) {
        expect_eq(got_box[i], exp_box[i]);
        i += 1;
    }

    // Sphere query verification
    const s_center = Vec3.init(10.0, -5.0, 12.0);
    const s_radius: f64 = 24.5;
    const s_r_sq = s_radius * s_radius;

    let got_sph: [180]u32 = std.mem.zeroes([180]u32);
    let exp_sph: [180]u32 = std.mem.zeroes([180]u32);
    let got_sph_s: []u32 = got_sph[0..];
    let exp_sph_s: []u32 = exp_sph[0..];

    const n_got_sph = tree.querySphere(s_center, s_radius, &got_sph_s);
    let n_exp_sph: usize = 0;
    i = 0;
    while (i < 180) {
        if (pts[i].pos.distanceSq(s_center) <= s_r_sq) {
            exp_sph[n_exp_sph] = pts[i].id;
            n_exp_sph += 1;
        }
        i += 1;
    }

    expect_eq(n_got_sph, n_exp_sph);
    expect_eq(n_got_sph > 0, true);
    sortU32(&got_sph_s, n_got_sph);
    sortU32(&exp_sph_s, n_exp_sph);
    i = 0;
    while (i < n_exp_sph) {
        expect_eq(got_sph[i], exp_sph[i]);
        i += 1;
    }
});

test("4. Pruned nearestPoint search matches brute-force across interior, exact & exterior targets", () => {
    const world = Aabb3.init(Vec3.init(0.0, 0.0, 0.0), Vec3.init(100.0, 100.0, 100.0)) ?? unreachable;
    let nodes: [512]OctreeNode = std.mem.zeroes([512]OctreeNode);
    let entries: [150]OctreeEntry = std.mem.zeroes([150]OctreeEntry);
    let pts: [150]OctreePoint = std.mem.zeroes([150]OctreePoint);

    let tree = Octree.init(world, nodes[0..], entries[0..], 3, 7) ?? unreachable;

    // Empty tree returns null
    expect_eq(tree.nearestPoint(Vec3.init(10.0, 10.0, 10.0)) == null, true);

    let rng: u32 = 2026;
    let i: usize = 0;
    while (i < 150) {
        pts[i] = OctreePoint.init(
            nextLcg(&rng) * 100.0,
            nextLcg(&rng) * 100.0,
            nextLcg(&rng) * 100.0,
            (i + 1) as u32,
        );
        expect_eq(tree.insert(pts[i]), true);
        i += 1;
    }

    // Test 25 query points including exterior points & exact hits
    let q: usize = 0;
    while (q < 25) {
        const target = if (q == 0)
            pts[73].pos
        else if (q == 1)
            Vec3.init(-20.0, 120.0, 50.0)
        else
            Vec3.init(nextLcg(&rng) * 120.0 - 10.0, nextLcg(&rng) * 120.0 - 10.0, nextLcg(&rng) * 120.0 - 10.0);

        const res = tree.nearestPoint(target) ?? unreachable;

        let best_d = pts[0].pos.distanceSq(target);
        let j: usize = 1;
        while (j < 150) {
            const d = pts[j].pos.distanceSq(target);
            if (d < best_d) {
                best_d = d;
            }
            j += 1;
        }

        expect_eq(approxEq(res.distance_sq, best_d, 1e-9), true);
        if (q == 0) {
            expect_eq(res.point.id, pts[73].id);
            expect_eq(approxEq(res.distance_sq, 0.0, 1e-12), true);
        }
        q += 1;
    }
});

test("5. Slab rayIntersectsAabb & octree queryRay / queryRayLeaves accuracy", () => {
    const box3 = Aabb3.init(Vec3.init(10.0, 10.0, 10.0), Vec3.init(20.0, 20.0, 20.0)) ?? unreachable;

    // Direct hit from outside along +X
    const r_hit = Ray3.init(Vec3.init(0.0, 15.0, 15.0), Vec3.init(1.0, 0.0, 0.0));
    const h1 = rayIntersectsAabb(r_hit, box3, 100.0) ?? unreachable;
    expect_eq(approxEq(h1.t_min, 10.0, 1e-9), true);
    expect_eq(approxEq(h1.t_max, 20.0, 1e-9), true);

    // Ray starting inside box: t_min clamped to 0.0, t_max == 5.0
    const r_in = Ray3.init(Vec3.init(15.0, 15.0, 15.0), Vec3.init(1.0, 0.0, 0.0));
    const h_in = rayIntersectsAabb(r_in, box3, 100.0) ?? unreachable;
    expect_eq(approxEq(h_in.t_min, 0.0, 1e-9), true);
    expect_eq(approxEq(h_in.t_max, 5.0, 1e-9), true);

    // Short max_t misses box
    expect_eq(rayIntersectsAabb(r_hit, box3, 9.5) == null, true);

    // Opposite direction misses box
    const r_opp = Ray3.init(Vec3.init(0.0, 15.0, 15.0), Vec3.init(-1.0, 0.0, 0.0));
    expect_eq(rayIntersectsAabb(r_opp, box3, 100.0) == null, true);

    // Parallel outside slab misses box
    const r_par = Ray3.init(Vec3.init(0.0, 25.0, 15.0), Vec3.init(1.0, 0.0, 0.0));
    expect_eq(rayIntersectsAabb(r_par, box3, 100.0) == null, true);

    // Build octree and verify queryRay against brute-force point-to-segment distance
    const world = Aabb3.init(Vec3.init(0.0, 0.0, 0.0), Vec3.init(100.0, 100.0, 100.0)) ?? unreachable;
    let nodes: [512]OctreeNode = std.mem.zeroes([512]OctreeNode);
    let entries: [128]OctreeEntry = std.mem.zeroes([128]OctreeEntry);
    let pts: [120]OctreePoint = std.mem.zeroes([120]OctreePoint);
    let tree = Octree.init(world, nodes[0..], entries[0..], 4, 6) ?? unreachable;

    let rng: u32 = 9001;
    let i: usize = 0;
    while (i < 120) {
        pts[i] = OctreePoint.init(
            nextLcg(&rng) * 100.0,
            nextLcg(&rng) * 100.0,
            nextLcg(&rng) * 100.0,
            i as u32,
        );
        expect_eq(tree.insert(pts[i]), true);
        i += 1;
    }

    // Diagonal ray through the volume
    const diag_ray = Ray3.init(Vec3.init(0.0, 0.0, 0.0), Vec3.init(1.0, 1.0, 1.0));
    const max_t: f64 = 100.0;
    const hit_radius: f64 = 12.0;

    let got_ray: [120]u32 = std.mem.zeroes([120]u32);
    let exp_ray: [120]u32 = std.mem.zeroes([120]u32);
    let got_ray_s: []u32 = got_ray[0..];
    let exp_ray_s: []u32 = exp_ray[0..];

    const n_got = tree.queryRay(diag_ray, max_t, hit_radius, &got_ray_s);

    const dir_sq = diag_ray.dir.dot(diag_ray.dir);
    let n_exp: usize = 0;
    i = 0;
    while (i < 120) {
        let t = pts[i].pos.sub(diag_ray.origin).dot(diag_ray.dir) / dir_sq;
        if (t < 0.0) {
            t = 0.0;
        } else if (t > max_t) {
            t = max_t;
        }
        if (pts[i].pos.distanceSq(diag_ray.pointAt(t)) <= hit_radius * hit_radius) {
            exp_ray[n_exp] = pts[i].id;
            n_exp += 1;
        }
        i += 1;
    }

    expect_eq(n_got, n_exp);
    expect_eq(n_got > 0, true);
    sortU32(&got_ray_s, n_got);
    sortU32(&exp_ray_s, n_exp);
    i = 0;
    while (i < n_exp) {
        expect_eq(got_ray[i], exp_ray[i]);
        i += 1;
    }

    // queryRayLeaves returns candidate points from pierced leaves (at least 1 hit along diagonal)
    let leaf_hits: [120]u32 = std.mem.zeroes([120]u32);
    let leaf_hits_s: []u32 = leaf_hits[0..];
    const n_leaf = tree.queryRayLeaves(diag_ray, max_t, &leaf_hits_s);
    expect_eq(n_leaf > 0, true);
    expect_eq(n_leaf <= 120, true);
});

test("6. Defensive limits: inverted AABB, out-of-bounds points, pool exhaustion & output truncation", () => {
    // Inverted AABB rejected
    expect_eq(Aabb3.init(Vec3.init(10.0, 0.0, 0.0), Vec3.init(5.0, 10.0, 10.0)) == null, true);
    expect_eq(OCTREE_NO_INDEX, -1 as i32);
    expect_eq(OCTREE_DEFAULT_LEAF_CAP, 8 as usize);

    const world = Aabb3.init(Vec3.init(0.0, 0.0, 0.0), Vec3.init(10.0, 10.0, 10.0)) ?? unreachable;
    let nodes: [9]OctreeNode = std.mem.zeroes([9]OctreeNode);
    let entries: [5]OctreeEntry = std.mem.zeroes([5]OctreeEntry);

    // Invalid init params rejected
    expect_eq(Octree.init(world, nodes[0..0], entries[0..], 2, 4) == null, true);
    expect_eq(Octree.init(world, nodes[0..], entries[0..0], 2, 4) == null, true);
    expect_eq(Octree.init(world, nodes[0..], entries[0..], 0, 4) == null, true);

    // Pool has room for 9 nodes (1 root + 1 subdivision) and 5 entries
    let tree = Octree.init(world, nodes[0..], entries[0..], 1, 4) ?? unreachable;

    // Out-of-bounds points gracefully rejected
    expect_eq(tree.insert(OctreePoint.init(-0.01, 5.0, 5.0, 100)), false);
    expect_eq(tree.insert(OctreePoint.init(5.0, 10.01, 5.0, 101)), false);
    expect_eq(tree.count(), 0 as usize);

    // Insert 5 points (node pool exhausts after first split, further splits gracefully degrade to leaf storage)
    expect_eq(tree.insert(OctreePoint.init(1.0, 1.0, 1.0, 1)), true);
    expect_eq(tree.insert(OctreePoint.init(1.1, 1.1, 1.1, 2)), true);
    expect_eq(tree.insert(OctreePoint.init(1.2, 1.2, 1.2, 3)), true);
    expect_eq(tree.insert(OctreePoint.init(8.0, 8.0, 8.0, 4)), true);
    expect_eq(tree.insert(OctreePoint.init(9.0, 9.0, 9.0, 5)), true);
    expect_eq(tree.count(), 5 as usize);
    expect_eq(tree.nodeCount(), 9 as usize);

    // 6th insert exceeds entry pool capacity (5) -> gracefully rejected
    expect_eq(tree.insert(OctreePoint.init(5.0, 5.0, 5.0, 6)), false);
    expect_eq(tree.count(), 5 as usize);

    // Output buffer truncation stays strictly within caller buffer bounds
    let tiny_out: [2]u32 = std.mem.zeroes([2]u32);
    let tiny_s: []u32 = tiny_out[0..];
    const capped = tree.queryBox(world, &tiny_s);
    expect_eq(capped, 2 as usize);
});
