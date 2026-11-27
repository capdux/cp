import { test, expect_eq } from "cap:test";
import { MAX_ITEMS, Aabb, BvhItem, Bvh } from "../../packages/bvh/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    let diff = a - b;
    if (diff < 0.0) {
        diff = -diff;
    }
    return diff <= eps;
}

fn makeBox(cx: f64, cy: f64, cz: f64, half: f64) -> Aabb {
    return Aabb {
        min_x: cx - half,
        min_y: cy - half,
        min_z: cz - half,
        max_x: cx + half,
        max_y: cy + half,
        max_z: cz + half,
    };
}

fn main() {
    test("1. @cap/bvh — Aabb merge, overlaps, containsPoint & surfaceArea", () => {
        expect_eq(MAX_ITEMS, 128 as usize);
        const a = Aabb { min_x: 0.0, min_y: 0.0, min_z: 0.0, max_x: 2.0, max_y: 3.0, max_z: 4.0 };
        const b = Aabb { min_x: 1.0, min_y: 2.0, min_z: 3.0, max_x: 5.0, max_y: 6.0, max_z: 7.0 };
        const c = Aabb { min_x: 10.0, min_y: 10.0, min_z: 10.0, max_x: 12.0, max_y: 12.0, max_z: 12.0 };

        expect_eq(a.overlaps(b), true);
        expect_eq(a.overlaps(c), false);
        expect_eq(a.containsPoint(1.0, 1.5, 2.0), true);
        expect_eq(a.containsPoint(2.5, 1.5, 2.0), false);
        // SA = 2*(2*3 + 3*4 + 4*2) = 2*(6+12+8) = 52
        expect_eq(approxEq(a.surfaceArea(), 52.0, 0.0001), true);

        const u = a.merge(b);
        expect_eq(approxEq(u.min_x, 0.0, 0.0001), true);
        expect_eq(approxEq(u.max_z, 7.0, 0.0001), true);
    });

    test("2. @cap/bvh — Empty BVH & single-item queries", () => {
        const one: [1]BvhItem = [BvhItem { bounds: makeBox(5.0, 0.0, 0.0, 1.0), id: 42 }];
        const empty = Bvh.build(one[0..0]);
        expect_eq(empty.isEmpty(), true);
        expect_eq(empty.len(), 0 as usize);

        let out: [4]u32 = undefined;
        expect_eq(empty.queryAabb(makeBox(5.0, 0.0, 0.0, 2.0), out[0..4]), 0 as usize);
        const miss0 = empty.raycast(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 100.0);
        expect_eq(miss0.hit, false);

        const single = Bvh.build(one[0..1]);
        expect_eq(single.isEmpty(), false);
        expect_eq(single.len(), 1 as usize);
        expect_eq(single.queryAabb(makeBox(5.5, 0.0, 0.0, 1.0), out[0..4]), 1 as usize);
        expect_eq(out[0], 42 as u32);
        const hit1 = single.raycast(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 100.0);
        expect_eq(hit1.hit, true);
        expect_eq(hit1.id, 42 as u32);
        expect_eq(approxEq(hit1.t, 4.0, 0.0001), true);
    });

    test("3. @cap/bvh — Multi-item queryAabb matches brute force", () => {
        const items: [6]BvhItem = [
            BvhItem { bounds: makeBox(0.0, 0.0, 0.0, 1.0), id: 1 },
            BvhItem { bounds: makeBox(2.0, 0.0, 0.0, 1.0), id: 2 },
            BvhItem { bounds: makeBox(5.0, 5.0, 5.0, 1.0), id: 3 },
            BvhItem { bounds: makeBox(-3.0, 0.0, 0.0, 1.0), id: 4 },
            BvhItem { bounds: makeBox(1.5, 0.5, 0.0, 0.5), id: 5 },
            BvhItem { bounds: makeBox(20.0, 20.0, 20.0, 2.0), id: 6 },
        ];
        const bvh = Bvh.build(items[0..6]);
        const query = makeBox(1.0, 0.0, 0.0, 1.0); // [0..2, -1..1, -1..1] overlaps 1, 2, 5

        let out: [6]u32 = undefined;
        const count = bvh.queryAabb(query, out[0..6]);
        expect_eq(count, 3 as usize);
    });

    test("4. @cap/bvh — Raycast occlusion returns closest item & exact t", () => {
        // Three boxes aligned along +X axis at x=10, x=5, x=20 (half=1)
        const items: [4]BvhItem = [
            BvhItem { bounds: makeBox(10.0, 0.0, 0.0, 1.0), id: 10 },
            BvhItem { bounds: makeBox(5.0, 0.0, 0.0, 1.0), id: 5 },
            BvhItem { bounds: makeBox(20.0, 0.0, 0.0, 1.0), id: 20 },
            BvhItem { bounds: makeBox(3.0, 5.0, 0.0, 1.0), id: 99 }, // off-axis
        ];
        const bvh = Bvh.build(items[0..4]);

        const hit = bvh.raycast(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 100.0);
        expect_eq(hit.hit, true);
        expect_eq(hit.id, 5 as u32);
        expect_eq(approxEq(hit.t, 4.0, 0.0001), true);

        // Cast from opposite side (-X direction from x=30): closest is id 20 at t = 30 - 21 = 9
        const hit_back = bvh.raycast(30.0, 0.0, 0.0, -1.0, 0.0, 0.0, 100.0);
        expect_eq(hit_back.hit, true);
        expect_eq(hit_back.id, 20 as u32);
        expect_eq(approxEq(hit_back.t, 9.0, 0.0001), true);
    });

    test("5. @cap/bvh — Reverse ray & max_t truncation miss", () => {
        const items: [2]BvhItem = [
            BvhItem { bounds: makeBox(10.0, 0.0, 0.0, 1.0), id: 1 },
            BvhItem { bounds: makeBox(20.0, 0.0, 0.0, 1.0), id: 2 },
        ];
        const bvh = Bvh.build(items[0..2]);

        // Ray pointing away (-X)
        const rev = bvh.raycast(0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 100.0);
        expect_eq(rev.hit, false);

        // Ray truncated before reaching x=9
        const short_ray = bvh.raycast(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 8.5);
        expect_eq(short_ray.hit, false);

        // Negative max_t
        const neg_ray = bvh.raycast(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, -1.0);
        expect_eq(neg_ray.hit, false);
    });

    test("6. @cap/bvh — 64 random boxes cross-checked vs brute-force AABB & raycast", () => {
        let items: [64]BvhItem = undefined;
        let state: u32 = 0x13579BDF;
        let i: usize = 0;
        while (i < 64) {
            state = state *% 1664525 +% 1013904223;
            const cx: f64 = ((state >> 16) as f64) / 655.36 - 50.0;
            state = state *% 1664525 +% 1013904223;
            const cy: f64 = ((state >> 16) as f64) / 655.36 - 50.0;
            state = state *% 1664525 +% 1013904223;
            const cz: f64 = ((state >> 16) as f64) / 655.36 - 50.0;
            items[i] = BvhItem {
                bounds: makeBox(cx, cy, cz, 3.5),
                id: i as u32,
            };
            i += 1;
        }

        const bvh = Bvh.build(items[0..64]);
        expect_eq(bvh.len(), 64 as usize);

        // Cross-check 10 AABB queries
        let out: [64]u32 = undefined;
        let q: usize = 0;
        while (q < 10) {
            state = state *% 1664525 +% 1013904223;
            const qx: f64 = ((state >> 16) as f64) / 655.36 - 50.0;
            state = state *% 1664525 +% 1013904223;
            const qy: f64 = ((state >> 16) as f64) / 655.36 - 50.0;
            const qbox = makeBox(qx, qy, 0.0, 12.0);

            let brute_cnt: usize = 0;
            let j: usize = 0;
            while (j < 64) {
                const box_j = items[j].bounds;
                if (box_j.overlaps(qbox)) {
                    brute_cnt += 1;
                }
                j += 1;
            }
            const bvh_cnt = bvh.queryAabb(qbox, out[0..64]);
            expect_eq(bvh_cnt, brute_cnt);
            q += 1;
        }

        // Cross-check 10 rays against brute force
        let r: usize = 0;
        while (r < 10) {
            const oy: f64 = -40.0 + (r as f64) * 8.0;
            const oz: f64 = -20.0 + (r as f64) * 4.0;
            let brute_hit = false;
            let brute_t: f64 = 200.0;
            let k: usize = 0;
            while (k < 64) {
                const box_k = items[k].bounds;
                const tk = box_k.rayHit(-80.0, oy, oz, 1.0, 0.0, 0.0, 200.0);
                if (tk >= 0.0 && (!brute_hit || tk < brute_t)) {
                    brute_hit = true;
                    brute_t = tk;
                }
                k += 1;
            }
            const res = bvh.raycast(-80.0, oy, oz, 1.0, 0.0, 0.0, 200.0);
            expect_eq(res.hit, brute_hit);
            if (brute_hit) {
                expect_eq(approxEq(res.t, brute_t, 0.00001), true);
            }
            r += 1;
        }
    });
}
