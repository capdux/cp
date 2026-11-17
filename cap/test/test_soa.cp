// cap/test/test_soa.cp
// Comprehensive regression test suite for SoA (Struct of Arrays) container: Soa<T>
// Underpinned by Zig native std.MultiArrayList(T) with SIMD-aligned contiguous column slices

import { test, expect_eq } from "cap:test";
import { Soa } from "cap:col";

pub struct Point {
    id: u32,
    x: f32,
    y: f32,
    active: bool,
}

pub struct NodeMetric {
    node_id: u64,
    cpu: f64,
    memory: f64,
    load: f32,
    online: bool,
}

// SIMD vector accumulation helper on contiguous column slice
fn sum_vector_simd(slice: []f32): f32 {
    let acc: f32 = 0.0;
    let i: usize = 0;
    while (i + 4 <= slice.len) {
        let chunk: @Vector(4, f32) = @splat(0.0);
        zig {
            chunk = slice[i..][0..4].*;
            acc += @reduce(.Add, chunk);
        }
        i += 4;
    }
    while (i < slice.len) {
        acc += slice[i];
        i += 1;
    }
    return acc;
}

fn main() {
    test("1. Soa<Point> basic lifecycle: init, append, get, set, len, capacity", () => {
        let points = Soa<Point>.init();
        expect_eq(points.len(), 0);
        expect_eq(points.isEmpty(), true);

        points.append(Point { id: 1, x: 10.0, y: 100.0, active: true });
        points.append(Point { id: 2, x: 20.0, y: 200.0, active: false });
        points.append(Point { id: 3, x: 30.0, y: 300.0, active: true });
        points.append(Point { id: 4, x: 40.0, y: 400.0, active: true });

        expect_eq(points.len(), 4);
        expect_eq(points.isEmpty(), false);
        expect_eq(points.capacity() >= 4, true);

        // Verify get(index)
        let p0 = points.get(0);
        expect_eq(p0.id, 1);
        expect_eq(p0.x, 10.0);
        expect_eq(p0.y, 100.0);
        expect_eq(p0.active, true);

        let p1 = points.get(1);
        expect_eq(p1.id, 2);
        expect_eq(p1.x, 20.0);
        expect_eq(p1.y, 200.0);
        expect_eq(p1.active, false);

        // Verify set(index, item)
        points.set(1, Point { id: 202, x: 25.5, y: 250.5, active: true });
        let p1_mod = points.get(1);
        expect_eq(p1_mod.id, 202);
        expect_eq(p1_mod.x, 25.5);
        expect_eq(p1_mod.y, 250.5);
        expect_eq(p1_mod.active, true);

        points.deinit();
    });

    test("2. Soa<Point> items(field) column slice extraction & SIMD vector reduction", () => {
        let points = Soa<Point>.initCapacity(8);
        expect_eq(points.capacity() >= 8, true);
        expect_eq(points.len(), 0);

        points.append(Point { id: 10, x: 1.0, y: 10.0, active: true });
        points.append(Point { id: 20, x: 2.0, y: 20.0, active: true });
        points.append(Point { id: 30, x: 3.0, y: 30.0, active: false });
        points.append(Point { id: 40, x: 4.0, y: 40.0, active: true });
        points.append(Point { id: 50, x: 5.0, y: 50.0, active: false });
        points.append(Point { id: 60, x: 6.0, y: 60.0, active: true });
        points.append(Point { id: 70, x: 7.0, y: 70.0, active: true });
        points.append(Point { id: 80, x: 8.0, y: 80.0, active: false });

        expect_eq(points.len(), 8);

        // Extract physical contiguous []f32 column slice for .x
        let xs: []f32 = points.items(.x);
        expect_eq(xs.len, 8);
        expect_eq(xs[0], 1.0);
        expect_eq(xs[1], 2.0);
        expect_eq(xs[2], 3.0);
        expect_eq(xs[3], 4.0);
        expect_eq(xs[4], 5.0);
        expect_eq(xs[5], 6.0);
        expect_eq(xs[6], 7.0);
        expect_eq(xs[7], 8.0);

        // Standard loop accumulation
        let sum_loop: f32 = 0.0;
        let i: usize = 0;
        while (i < xs.len) {
            sum_loop += xs[i];
            i += 1;
        }
        expect_eq(sum_loop, 36.0);

        // High-speed SIMD hardware vector reduction across contiguous slice
        let simd_sum = sum_vector_simd(xs);
        expect_eq(simd_sum, 36.0);

        // Extract contiguous []f32 column slice for .y
        let ys: []f32 = points.items(.y);
        expect_eq(ys.len, 8);
        let sum_y: f32 = 0.0;
        let yi: usize = 0;
        while (yi < ys.len) {
            sum_y += ys[yi];
            yi += 1;
        }
        expect_eq(sum_y, 360.0);

        // Extract contiguous []u32 column slice for .id
        let ids: []u32 = points.items(.id);
        expect_eq(ids.len, 8);
        expect_eq(ids[0], 10);
        expect_eq(ids[7], 80);

        // Extract contiguous []bool column slice for .active
        let actives: []bool = points.items(.active);
        expect_eq(actives.len, 8);
        expect_eq(actives[0], true);
        expect_eq(actives[2], false);

        points.deinit();
    });

    test("3. Soa<Point> removal operations: popOrNull, swapRemove, orderedRemove, clear", () => {
        let points = Soa<Point>.init();
        points.append(Point { id: 1, x: 1.0, y: 1.0, active: true });
        points.append(Point { id: 2, x: 2.0, y: 2.0, active: true });
        points.append(Point { id: 3, x: 3.0, y: 3.0, active: true });
        points.append(Point { id: 4, x: 4.0, y: 4.0, active: true });
        points.append(Point { id: 5, x: 5.0, y: 5.0, active: true });

        expect_eq(points.len(), 5);

        // popOrNull
        let popped = points.popOrNull();
        if (p := popped) {
            expect_eq(p.id, 5);
            expect_eq(p.x, 5.0);
        } else {
            expect_eq(1, 0);
        }
        expect_eq(points.len(), 4);

        // swapRemove(1) replaces index 1 with the last element (index 3, id: 4)
        points.swapRemove(1);
        expect_eq(points.len(), 3);
        expect_eq(points.get(0).id, 1);
        expect_eq(points.get(1).id, 4);
        expect_eq(points.get(2).id, 3);

        // orderedRemove(1) shifts remaining elements left
        points.orderedRemove(1);
        expect_eq(points.len(), 2);
        expect_eq(points.get(0).id, 1);
        expect_eq(points.get(1).id, 3);

        // clearRetainingCapacity
        const cap_before = points.capacity();
        points.clearRetainingCapacity();
        expect_eq(points.len(), 0);
        expect_eq(points.isEmpty(), true);
        expect_eq(points.capacity(), cap_before);

        // pop on empty Soa returns null
        expect_eq(points.popOrNull() == null, true);

        points.deinit();
    });

    test("4. Soa<NodeMetric> multi-type composite struct container", () => {
        let metrics = Soa<NodeMetric>.init();

        metrics.append(NodeMetric { node_id: 1001, cpu: 12.5, memory: 512.0, load: 0.25, online: true });
        metrics.append(NodeMetric { node_id: 1002, cpu: 85.0, memory: 4096.0, load: 3.50, online: true });
        metrics.append(NodeMetric { node_id: 1003, cpu: 4.0,  memory: 256.0,  load: 0.10, online: false });

        expect_eq(metrics.len(), 3);

        let cpus: []f64 = metrics.items(.cpu);
        expect_eq(cpus.len, 3);
        expect_eq(cpus[0], 12.5);
        expect_eq(cpus[1], 85.0);
        expect_eq(cpus[2], 4.0);

        let loads: []f32 = metrics.items(.load);
        expect_eq(loads.len, 3);
        expect_eq(loads[0], 0.25);
        expect_eq(loads[1], 3.50);

        let onlines: []bool = metrics.items(.online);
        expect_eq(onlines[0], true);
        expect_eq(onlines[1], true);
        expect_eq(onlines[2], false);

        metrics.deinit();
    });
}
