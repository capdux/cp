// cap/test/testCapSlab.cp
// Comprehensive unit tests for @cap/slab

import { test, expect_eq } from "cap:test";
import {
    SlabKey,
    SlabI64,
    SlabU64,
} from "../../packages/sharded-slab/src/index.cp";

fn main() {
    test("1. @cap/slab — O(1) insert, get, set, contains & count on SlabI64", () => {
        let s = SlabI64.init();
        expect_eq(s.isEmpty(), true);
        expect_eq(s.count(), 0 as usize);

        const k1 = s.insert(100 as i64) ?? SlabKey{ index: 999 as u32, generation: 0 as u32 };
        const k2 = s.insert(-250 as i64) ?? SlabKey{ index: 999 as u32, generation: 0 as u32 };
        expect_eq(s.count(), 2 as usize);
        expect_eq(s.isEmpty(), false);
        expect_eq(s.contains(k1), true);
        expect_eq(s.get(k1) ?? (0 as i64), 100 as i64);
        expect_eq(s.get(k2) ?? (0 as i64), -250 as i64);

        expect_eq(s.set(k1, 777 as i64), true);
        expect_eq(s.get(k1) ?? (0 as i64), 777 as i64);
    });

    test("2. @cap/slab — Slot reuse & generational ABA protection after remove", () => {
        let s = SlabI64.init();
        const kOld = s.insert(10 as i64) ?? SlabKey{ index: 999 as u32, generation: 0 as u32 };
        expect_eq(kOld.index, 0 as u32);
        expect_eq(kOld.generation, 1 as u32);

        const removed = s.remove(kOld);
        expect_eq(removed ?? (0 as i64), 10 as i64);
        expect_eq(s.contains(kOld), false);
        expect_eq(s.get(kOld) == null, true);
        expect_eq(s.remove(kOld) == null, true);

        // Next insert reuses slot 0 with generation 2
        const kNew = s.insert(20 as i64) ?? SlabKey{ index: 999 as u32, generation: 0 as u32 };
        expect_eq(kNew.index, 0 as u32);
        expect_eq(kNew.generation, 2 as u32);
        expect_eq(kOld.equals(kNew), false);

        // Old handle MUST be rejected even though slot 0 is occupied again
        expect_eq(s.contains(kOld), false);
        expect_eq(s.get(kOld) == null, true);
        expect_eq(s.set(kOld, 999 as i64), false);
        expect_eq(s.remove(kOld) == null, true);

        // New handle works normally
        expect_eq(s.get(kNew) ?? (0 as i64), 20 as i64);
    });

    test("3. @cap/slab — clear invalidates all previous handles", () => {
        let s = SlabI64.init();
        const ka = s.insert(1 as i64) ?? SlabKey{ index: 999 as u32, generation: 0 as u32 };
        const kb = s.insert(2 as i64) ?? SlabKey{ index: 999 as u32, generation: 0 as u32 };
        expect_eq(s.count(), 2 as usize);

        s.clear();
        expect_eq(s.isEmpty(), true);
        expect_eq(s.count(), 0 as usize);
        expect_eq(s.get(ka) == null, true);
        expect_eq(s.get(kb) == null, true);

        const kc = s.insert(3 as i64) ?? SlabKey{ index: 999 as u32, generation: 0 as u32 };
        expect_eq(kc.index, 0 as u32);
        expect_eq(s.get(ka) == null, true);
        expect_eq(s.get(kc) ?? (0 as i64), 3 as i64);
    });

    test("4. @cap/slab — Full capacity (256 slots) returns null on overflow", () => {
        let s = SlabU64.init();
        let i: usize = 0;
        while (i < 256) {
            const k = s.insert((i as u64) * (10 as u64));
            expect_eq(k != null, true);
            i += 1;
        }
        expect_eq(s.count(), 256 as usize);
        expect_eq(s.isFull(), true);
        expect_eq(s.insert(9999 as u64) == null, true);

        // Out-of-bounds handle safely rejected
        const badKey = SlabKey{ index: 300 as u32, generation: 1 as u32 };
        expect_eq(s.get(badKey) == null, true);
        expect_eq(s.set(badKey, 1 as u64), false);
        expect_eq(s.remove(badKey) == null, true);
    });

    test("5. @cap/slab — SlabU64 full lifecycle (insert, set, remove, clear)", () => {
        let s = SlabU64.init();
        const k1 = s.insert(123456789 as u64) ?? SlabKey{ index: 999 as u32, generation: 0 as u32 };
        expect_eq(s.get(k1) ?? (0 as u64), 123456789 as u64);
        expect_eq(s.set(k1, 987654321 as u64), true);
        expect_eq(s.get(k1) ?? (0 as u64), 987654321 as u64);
        expect_eq(s.remove(k1) ?? (0 as u64), 987654321 as u64);
        expect_eq(s.isEmpty(), true);
    });

    test("6. @cap/slab — High-churn insert/remove stress test (1000 cycles)", () => {
        let s = SlabI64.init();
        let round: i64 = 0;
        while (round < 1000) {
            const kA = s.insert(round) ?? SlabKey{ index: 999 as u32, generation: 0 as u32 };
            const kB = s.insert(round + (1 as i64)) ?? SlabKey{ index: 999 as u32, generation: 0 as u32 };
            expect_eq(s.get(kA) ?? (-1 as i64), round);
            expect_eq(s.remove(kA) ?? (-1 as i64), round);
            expect_eq(s.get(kA) == null, true);
            expect_eq(s.remove(kB) ?? (-1 as i64), round + (1 as i64));
            round += 1;
        }
        expect_eq(s.count(), 0 as usize);
        expect_eq(s.isEmpty(), true);
    });
}
