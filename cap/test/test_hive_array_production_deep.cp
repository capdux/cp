import { test, expect_eq } from "cap:test";
import { HiveArray } from "cap:ds";

struct TaskSlot {
    id: u32;
    weight: i32;
}

test("1. 64-slot full lifecycle, reuse, full-pool rejection, and double-free guard", () => {
    const Pool64 = HiveArray(u32, 64);
    let pool = Pool64.init();

    expect_eq(pool.isEmpty(), true);
    expect_eq(pool.isFull(), false);
    expect_eq(pool.count(), u16(0));
    expect_eq(pool.freeCount(), u16(64));

    // 填满全部 64 个槽位
    let i: u16 = 0;
    while (i < 64) {
        const idx = pool.allocWith(u32(i) * 10 + 7) ?? unreachable;
        expect_eq(idx, i);
        expect_eq(pool.isOccupied(idx), true);
        i += 1;
    }

    expect_eq(pool.isFull(), true);
    expect_eq(pool.count(), u16(64));
    expect_eq(pool.freeCount(), u16(0));
    // 满池再分配返回 null
    expect_eq(pool.alloc() == null, true);
    expect_eq(pool.allocWith(999) == null, true);

    // 释放槽 0、17、63，并验证重复 free 与越界 free 被拦截
    expect_eq(pool.free(0), true);
    expect_eq(pool.free(0), false);
    expect_eq(pool.free(17), true);
    expect_eq(pool.free(17), false);
    expect_eq(pool.free(63), true);
    expect_eq(pool.free(64), false);
    expect_eq(pool.free(1000), false);
    expect_eq(pool.count(), u16(61));
    expect_eq(pool.freeCount(), u16(3));

    // 复用按最低空闲位顺序分配（0 -> 17 -> 63）
    expect_eq(pool.allocWith(111) ?? unreachable, u16(0));
    expect_eq(pool.allocWith(222) ?? unreachable, u16(17));
    expect_eq(pool.allocWith(333) ?? unreachable, u16(63));
    expect_eq(pool.isFull(), true);
    expect_eq(pool.get(0) ?? unreachable, u32(111));
    expect_eq(pool.get(17) ?? unreachable, u32(222));
    expect_eq(pool.get(63) ?? unreachable, u32(333));
});

test("2. multi-word non-multiple capacity (150 slots) boundary and tail masking", () => {
    const Pool150 = HiveArray(TaskSlot, 150);
    let pool = Pool150.init();

    // 填满 150 槽（跨 3 个 u64 word：0..63, 64..127, 128..149）
    let i: u16 = 0;
    while (i < 150) {
        const idx = pool.allocWith(TaskSlot{ id: u32(i) + 100, weight: i32(i) * -2 }) ?? unreachable;
        expect_eq(idx, i);
        i += 1;
    }

    // 第 151 次必须返回 null（绝不能越界分配到第 3 个 word 的 150..191 位）
    expect_eq(pool.isFull(), true);
    expect_eq(pool.alloc() == null, true);
    expect_eq(pool.isOccupied(150), false);
    expect_eq(pool.isOccupied(191), false);

    // 跨 word 释放（word0: 63, word1: 64 & 127, word2: 128 & 149）
    expect_eq(pool.free(63), true);
    expect_eq(pool.free(64), true);
    expect_eq(pool.free(127), true);
    expect_eq(pool.free(128), true);
    expect_eq(pool.free(149), true);
    expect_eq(pool.count(), u16(145));

    // 再次分配精确回收这 5 个跨界槽位
    expect_eq(pool.alloc() ?? unreachable, u16(63));
    expect_eq(pool.alloc() ?? unreachable, u16(64));
    expect_eq(pool.alloc() ?? unreachable, u16(127));
    expect_eq(pool.alloc() ?? unreachable, u16(128));
    expect_eq(pool.alloc() ?? unreachable, u16(149));
    expect_eq(pool.alloc() == null, true);
});

test("3. get, getPtr, and set mutation semantics on occupied vs free slots", () => {
    const Pool = HiveArray(TaskSlot, 128);
    let pool = Pool.init();

    // 未分配槽位访问全部安全拒绝
    expect_eq(pool.get(0) == null, true);
    expect_eq(pool.getPtr(0) == null, true);
    expect_eq(pool.set(0, TaskSlot{ id: 1, weight: 1 }), false);
    expect_eq(pool.get(200) == null, true);

    const s0 = pool.allocWith(TaskSlot{ id: 10, weight: 5 }) ?? unreachable;
    const s1 = pool.allocWith(TaskSlot{ id: 20, weight: 15 }) ?? unreachable;

    // 通过 getPtr 就地修改
    if (ptr := pool.getPtr(s0)) {
        ptr.weight = 99;
    } else {
        unreachable;
    }
    const v0 = pool.get(s0) ?? unreachable;
    expect_eq(v0.id, u32(10));
    expect_eq(v0.weight, i32(99));

    // 通过 set 整体覆盖
    expect_eq(pool.set(s1, TaskSlot{ id: 42, weight: -7 }), true);
    const v1 = pool.get(s1) ?? unreachable;
    expect_eq(v1.id, u32(42));
    expect_eq(v1.weight, i32(-7));

    // 释放后再次访问返回 null / false
    expect_eq(pool.free(s0), true);
    expect_eq(pool.get(s0) == null, true);
    expect_eq(pool.getPtr(s0) == null, true);
    expect_eq(pool.set(s0, TaskSlot{ id: 0, weight: 0 }), false);
});

test("4. nextOccupied sparse bitmask iteration across multiple words", () => {
    const Pool = HiveArray(u16, 200);
    let pool = Pool.init();

    // 空池迭代立即返回 null
    expect_eq(pool.nextOccupied(0) == null, true);

    // 分配全部 200 个，再只保留稀疏集合：[3, 63, 64, 129, 199]
    let i: u16 = 0;
    while (i < 200) {
        * = pool.allocWith(i);
        i += 1;
    }

    let j: u16 = 0;
    while (j < 200) {
        if (j != 3 && j != 63 && j != 64 && j != 129 && j != 199) {
            expect_eq(pool.free(j), true);
        }
        j += 1;
    }
    expect_eq(pool.count(), u16(5));

    // 使用 nextOccupied 稀疏跳跃遍历
    const expected: [5]u16 = [3, 63, 64, 129, 199];
    let cursor: u16 = 0;
    let seen: usize = 0;
    while (true) {
        if (idx := pool.nextOccupied(cursor)) {
            expect_eq(seen < expected.len, true);
            expect_eq(idx, expected[seen]);
            expect_eq(pool.get(idx) ?? unreachable, idx);
            seen += 1;
            cursor = idx + 1;
        } else {
            break;
        }
    }
    expect_eq(seen, usize(5));

    // 边界查询：从最后一个已占槽位之后开始以及越界查询均返回 null
    expect_eq(pool.nextOccupied(200) == null, true);
    expect_eq(pool.nextOccupied(65535) == null, true);
});

test("5. clear resets bitmap and allows fresh allocation from slot 0", () => {
    const Pool = HiveArray(u64, 256);
    let pool = Pool.init();

    let i: u16 = 0;
    while (i < 130) {
        * = pool.allocWith(u64(i) + 1);
        i += 1;
    }
    expect_eq(pool.count(), u16(130));

    pool.clear();
    expect_eq(pool.isEmpty(), true);
    expect_eq(pool.count(), u16(0));
    expect_eq(pool.freeCount(), u16(256));
    expect_eq(pool.nextOccupied(0) == null, true);
    expect_eq(pool.isOccupied(0), false);
    expect_eq(pool.isOccupied(129), false);
    expect_eq(pool.free(0), false);

    // clear 后再次分配应从 0 开始
    const first = pool.allocWith(777) ?? unreachable;
    expect_eq(first, u16(0));
    expect_eq(pool.get(0) ?? unreachable, u64(777));
});
