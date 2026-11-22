// Source: cp/cap/test/test_ecs_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:ecs & cap.ecs

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    EcsError,
    EntityId,
    EntityRegistry,
    ComponentPool,
    EcsQuery,
} from "cap:gfx";

struct Position {
    x: f32;
    y: f32;
}

struct Velocity {
    vx: f32;
    vy: f32;
}

struct Health {
    hp: i32;
    max_hp: i32;
}

fn testRegistrySpawnDespawnAndAba(): ! {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    const alloc = gpa.allocator();
    {
        let reg = EntityRegistry.init(alloc);
        defer reg.deinit();

        expect_eq(reg.aliveCount(), 0 as usize);
        expect_eq(EntityId.INVALID.isValid(), false);

        const e0 = reg.spawn()?;
        const e1 = reg.spawn()?;
        const e2 = reg.spawn()?;
        expect_eq(reg.aliveCount(), 3 as usize);
        expect_eq(e0.index, 0 as u32);
        expect_eq(e0.generation, 1 as u32);
        expect_eq(e1.index, 1 as u32);
        expect_eq(e2.index, 2 as u32);

        // Round-trip raw 64-bit encoding
        const raw1 = e1.raw();
        const decoded1 = EntityId.fromRaw(raw1);
        expect_eq(decoded1.eql(e1), true);

        // Despawn e1: old handle must immediately become dead
        expect_eq(reg.despawn(e1), true);
        expect_eq(reg.despawn(e1), false);
        expect_eq(reg.isAlive(e1), false);
        expect_eq(reg.aliveCount(), 2 as usize);

        // Next spawn must recycle index 1 with incremented generation 2 (ABA prevention)
        const e1_new = reg.spawn()?;
        expect_eq(e1_new.index, 1 as u32);
        expect_eq(e1_new.generation, 2 as u32);
        expect_eq(e1_new.eql(e1), false);
        expect_eq(reg.isAlive(e1_new), true);
        expect_eq(reg.isAlive(e1), false);

        // Attempting to attach a component to stale e1 must fail with EcsError.DeadEntity
        let pool = ComponentPool(Position).init(alloc);
        defer pool.deinit();

        let got_dead = false;
        pool.set(e1, &reg, Position{ x: 1.0, y: 2.0 }) catch |err_val| {
            if (std.mem.eql(u8, @errorName(err_val), "DeadEntity")) {
                got_dead = true;
            }
        };
        expect_eq(got_dead, true);
        expect_eq(pool.has(e1), false);

        // New entity on recycled slot succeeds
        pool.set(e1_new, &reg, Position{ x: 10.0, y: 20.0 })?;
        expect_eq(pool.has(e1_new), true);
        expect_eq(pool.has(e1), false);
        expect_eq(pool.getConst(e1) == null, true);
        expect_eq(pool.getConst(e1_new).?.x, 10.0 as f32);
    }
    expect_eq(gpa.deinit() == .ok, true);
}

fn testComponentPoolSwapAndPopIntegrity(): ! {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    const alloc = gpa.allocator();
    {
        let reg = EntityRegistry.init(alloc);
        defer reg.deinit();

        let pool = ComponentPool(Health).init(alloc);
        defer pool.deinit();

        const e0 = reg.spawn()?;
        const e1 = reg.spawn()?;
        const e2 = reg.spawn()?;
        const e3 = reg.spawn()?;

        pool.set(e0, &reg, Health{ hp: 100, max_hp: 100 })?;
        pool.set(e1, &reg, Health{ hp: 200, max_hp: 200 })?;
        pool.set(e2, &reg, Health{ hp: 300, max_hp: 300 })?;
        pool.set(e3, &reg, Health{ hp: 400, max_hp: 400 })?;
        expect_eq(pool.len(), 4 as usize);

        // Overwrite e2 in place
        pool.set(e2, &reg, Health{ hp: 350, max_hp: 500 })?;
        expect_eq(pool.len(), 4 as usize);
        expect_eq(pool.getConst(e2).?.hp, 350 as i32);

        // Remove middle element e1 -> e3 (last) must swap into dense slot 1
        expect_eq(pool.remove(e1), true);
        expect_eq(pool.remove(e1), false);
        expect_eq(pool.has(e1), false);
        expect_eq(pool.get(e1) == null, true);
        expect_eq(pool.len(), 3 as usize);

        // Remaining entities must still map to their exact components without corruption
        expect_eq(pool.getConst(e0).?.hp, 100 as i32);
        expect_eq(pool.getConst(e2).?.hp, 350 as i32);
        expect_eq(pool.getConst(e3).?.hp, 400 as i32);

        // Mutate e3 via mutable pointer and verify contiguous items() slice
        if (h3 := pool.get(e3)) {
            h3.hp -= 50;
        }
        expect_eq(pool.getConst(e3).?.hp, 350 as i32);

        // Remove e0 (dense slot 0) -> e2 (now last) swaps into slot 0
        expect_eq(pool.remove(e0), true);
        expect_eq(pool.len(), 2 as usize);
        expect_eq(pool.has(e0), false);
        expect_eq(pool.getConst(e2).?.hp, 350 as i32);
        expect_eq(pool.getConst(e3).?.hp, 350 as i32);

        let sum: i32 = 0;
        for (item of pool.items()) {
            sum += item.hp;
        }
        expect_eq(sum, 700 as i32);
    }
    expect_eq(gpa.deinit() == .ok, true);
}

fn testEcsQueryCollectJoined2(): ! {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    const alloc = gpa.allocator();
    {
        let reg = EntityRegistry.init(alloc);
        defer reg.deinit();

        let pos = ComponentPool(Position).init(alloc);
        defer pos.deinit();
        let vel = ComponentPool(Velocity).init(alloc);
        defer vel.deinit();

        const e0 = reg.spawn()?; // Position only
        const e1 = reg.spawn()?; // Position + Velocity
        const e2 = reg.spawn()?; // Velocity only
        const e3 = reg.spawn()?; // Position + Velocity
        const e4 = reg.spawn()?; // Position only

        pos.set(e0, &reg, Position{ x: 1.0, y: 1.0 })?;
        pos.set(e1, &reg, Position{ x: 10.0, y: 20.0 })?;
        pos.set(e3, &reg, Position{ x: 30.0, y: 40.0 })?;
        pos.set(e4, &reg, Position{ x: 5.0, y: 5.0 })?;

        vel.set(e1, &reg, Velocity{ vx: 2.0, vy: -3.0 })?;
        vel.set(e2, &reg, Velocity{ vx: 9.0, vy: 9.0 })?;
        vel.set(e3, &reg, Velocity{ vx: -5.0, vy: 4.0 })?;

        // vel.len() (3) < pos.len() (4) -> drives from vel
        const joined = EcsQuery.collectJoined2(alloc, Position, Velocity, &pos, &vel)?;
        defer alloc.free(joined);
        expect_eq(joined.len, 2 as usize);

        for (e of joined) {
            let p = pos.get(e).?;
            const v = vel.getConst(e).?;
            p.x += v.vx;
            p.y += v.vy;
        }

        expect_eq(pos.getConst(e1).?.x, 12.0 as f32);
        expect_eq(pos.getConst(e1).?.y, 17.0 as f32);
        expect_eq(pos.getConst(e3).?.x, 25.0 as f32);
        expect_eq(pos.getConst(e3).?.y, 44.0 as f32);
        // Unjoined entity e0 remains untouched
        expect_eq(pos.getConst(e0).?.x, 1.0 as f32);

        // Reverse argument order (drives from smaller pool either way)
        const joined_rev = EcsQuery.collectJoined2(alloc, Velocity, Position, &vel, &pos)?;
        defer alloc.free(joined_rev);
        expect_eq(joined_rev.len, 2 as usize);
    }
    expect_eq(gpa.deinit() == .ok, true);
}

fn testLargeScaleLifecycleZeroLeak(): ! {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    const alloc = gpa.allocator();
    {
        let reg = EntityRegistry.init(alloc);
        defer reg.deinit();

        let pos = ComponentPool(Position).init(alloc);
        defer pos.deinit();
        let hp = ComponentPool(Health).init(alloc);
        defer hp.deinit();

        let ids: [1200]EntityId = [EntityId.INVALID; 1200];
        for (i of 0..1200) {
            const e = reg.spawn()?;
            ids[usize(i)] = e;
            pos.set(e, &reg, Position{ x: f32(i), y: f32(i) * 2.0 })?;
            if (usize(i) % 3 == 0) {
                hp.set(e, &reg, Health{ hp: i32(i), max_hp: 1200 })?;
            }
        }
        expect_eq(reg.aliveCount(), 1200 as usize);
        expect_eq(pos.len(), 1200 as usize);
        expect_eq(hp.len(), 400 as usize);

        // Despawn every even entity and remove its components
        for (i of 0..1200) {
            if (usize(i) % 2 == 0) {
                expect_eq(reg.despawn(ids[usize(i)]), true);
                * = pos.remove(ids[usize(i)]);
                * = hp.remove(ids[usize(i)]);
            }
        }
        expect_eq(reg.aliveCount(), 600 as usize);
        expect_eq(pos.len(), 600 as usize);

        // Respawn 600 entities reusing recycled slots with generation == 2
        for (i of 0..600) {
            const recycled = reg.spawn()?;
            expect_eq(recycled.generation, 2 as u32);
            pos.set(recycled, &reg, Position{ x: -1.0, y: -1.0 })?;
            hp.set(recycled, &reg, Health{ hp: 50, max_hp: 100 })?;
        }
        expect_eq(reg.aliveCount(), 1200 as usize);
        expect_eq(reg.capacity(), 1200 as usize);
        expect_eq(pos.len(), 1200 as usize);

        const joined = EcsQuery.collectJoined2(alloc, Position, Health, &pos, &hp)?;
        defer alloc.free(joined);
        // 200 odd original entities with i%3==0 (i%6==3) + 600 newly spawned = 800
        expect_eq(joined.len, 800 as usize);

        pos.clear();
        expect_eq(pos.isEmpty(), true);
    }
    expect_eq(gpa.deinit() == .ok, true);
}

fn testCapEcsDynamicLazyMount(): ! {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    const alloc = gpa.allocator();
    {
        let reg = cap.gfx.EntityRegistry.init(alloc);
        defer reg.deinit();

        let pool = cap.gfx.ComponentPool(Position).init(alloc);
        defer pool.deinit();
        let hp_pool = cap.gfx.ComponentPool(Health).init(alloc);
        defer hp_pool.deinit();

        const e = reg.spawn()?;
        pool.set(e, &reg, Position{ x: 42.0, y: 84.0 })?;
        hp_pool.set(e, &reg, Health{ hp: 99, max_hp: 100 })?;

        const hits = cap.gfx.EcsQuery.collectJoined2(alloc, Position, Health, &pool, &hp_pool)?;
        defer alloc.free(hits);
        expect_eq(hits.len, 1 as usize);
        expect_eq(hits[0].eql(e), true);

        expect_eq(reg.despawn(e), true);
        expect_eq(reg.isAlive(e), false);
    }
    expect_eq(gpa.deinit() == .ok, true);
}

fn main() {
    test("1. EntityRegistry Batch Spawn, Despawn Slot Recycling & Generation ABA Prevention", () => {
        testRegistrySpawnDespawnAndAba()?;
    });
    test("2. ComponentPool<T> O(1) Set/Get/Has & Swap-and-Pop Remove Integrity", () => {
        testComponentPoolSwapAndPopIntegrity()?;
    });
    test("3. EcsQuery.collectJoined2 Smaller-Pool Driven Intersection & Update", () => {
        testEcsQueryCollectJoined2()?;
    });
    test("4. 1200-Entity Lifecycle Stress Test & DebugAllocator Zero-Leak Check", () => {
        testLargeScaleLifecycleZeroLeak()?;
    });
    test("5. import cap from \"cap\"; cap.ecs Dynamic Lazy Mounting", () => {
        testCapEcsDynamicLazyMount()?;
    });
}
