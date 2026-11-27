import { test, expect_eq } from "cap:test";
import {
    ARENA_CAP,
    ArenaId,
    IdArenaI64,
    IdArenaU64,
} from "../../packages/id-arena/src/index.cp";

fn main() {
    test("1. @cap/id-arena — IdArenaI64 alloc, get, set & metadata", () => {
        let a: IdArenaI64 = IdArenaI64.new(1);
        expect_eq(a.capacity(), ARENA_CAP);
        expect_eq(a.len(), 0 as usize);
        expect_eq(a.isEmpty(), true);

        const id0: ArenaId = a.alloc(10).?;
        const id1: ArenaId = a.alloc(-20).?;
        const id2: ArenaId = a.alloc(30).?;

        expect_eq(id0.index, 0 as usize);
        expect_eq(id0.arena_id, 1 as u32);
        expect_eq(a.len(), 3 as usize);
        expect_eq(a.isEmpty(), false);
        expect_eq(a.contains(id1), true);
        expect_eq(a.get(id0).?, 10 as i64);
        expect_eq(a.get(id1).?, -20 as i64);
        expect_eq(a.get(id2).?, 30 as i64);

        expect_eq(a.set(id1, 999), true);
        expect_eq(a.get(id1).?, 999 as i64);
    });

    test("2. @cap/id-arena — cross-arena ArenaId rejection (provenance safety)", () => {
        let a1: IdArenaI64 = IdArenaI64.new(101);
        let a2: IdArenaI64 = IdArenaI64.new(202);

        const id_from_1: ArenaId = a1.alloc(42).?;
        const id_from_2: ArenaId = a2.alloc(84).?;

        expect_eq(a1.contains(id_from_2), false);
        expect_eq(a1.get(id_from_2) == null, true);
        expect_eq(a1.set(id_from_2, 777), false);

        expect_eq(a2.contains(id_from_1), false);
        expect_eq(a2.get(id_from_1) == null, true);
        expect_eq(a2.set(id_from_1, 888), false);

        // Original values remain untouched
        expect_eq(a1.get(id_from_1).?, 42 as i64);
        expect_eq(a2.get(id_from_2).?, 84 as i64);
    });

    test("3. @cap/id-arena — capacity boundary (128 items) returns null when full", () => {
        let a: IdArenaI64 = IdArenaI64.new(7);
        let i: usize = 0;
        while (i < ARENA_CAP) : (i += 1) {
            const id_opt: ArenaId? = a.alloc(i64(i * 2));
            expect_eq(id_opt != null, true);
        }
        expect_eq(a.len(), ARENA_CAP);
        expect_eq(a.alloc(9999) == null, true);

        const last_id: ArenaId = ArenaId{ index: 127, arena_id: 7 };
        const oob_id: ArenaId = ArenaId{ index: 128, arena_id: 7 };
        expect_eq(a.get(last_id).?, 254 as i64);
        expect_eq(a.get(oob_id) == null, true);
    });

    test("4. @cap/id-arena — clear() invalidates old indices and permits reuse", () => {
        let a: IdArenaI64 = IdArenaI64.new(9);
        const id0: ArenaId = a.alloc(111).?;
        const id1: ArenaId = a.alloc(222).?;
        expect_eq(a.len(), 2 as usize);

        a.clear();
        expect_eq(a.len(), 0 as usize);
        expect_eq(a.isEmpty(), true);
        expect_eq(a.contains(id0), false);
        expect_eq(a.get(id1) == null, true);

        const new_id: ArenaId = a.alloc(333).?;
        expect_eq(new_id.index, 0 as usize);
        expect_eq(a.get(new_id).?, 333 as i64);
        expect_eq(a.contains(id1), false);
    });

    test("5. @cap/id-arena — IdArenaU64 alloc, get, set & high-bit u64 values", () => {
        let u: IdArenaU64 = IdArenaU64.new(55);
        const big: u64 = ((0xCAFEBABE as u64) << 32) | (0x12345678 as u64);

        const id0: ArenaId = u.alloc(big).?;
        const id1: ArenaId = u.alloc(0).?;
        expect_eq(u.len(), 2 as usize);
        expect_eq(u.get(id0).?, big);
        expect_eq(u.get(id1).?, 0 as u64);

        const updated: u64 = ((0xDEADBEEF as u64) << 32) | (0x87654321 as u64);
        expect_eq(u.set(id1, updated), true);
        expect_eq(u.get(id1).?, updated);
    });

    test("6. @cap/id-arena — IdArenaU64 cross-arena rejection & clear", () => {
        let u1: IdArenaU64 = IdArenaU64.new(1);
        let u2: IdArenaU64 = IdArenaU64.new(2);

        const id1: ArenaId = u1.alloc(1000).?;
        const id2: ArenaId = u2.alloc(2000).?;

        expect_eq(u1.contains(id2), false);
        expect_eq(u1.get(id2) == null, true);
        expect_eq(u2.set(id1, 3000), false);

        u1.clear();
        expect_eq(u1.isEmpty(), true);
        expect_eq(u1.get(id1) == null, true);
        expect_eq(u2.get(id2).?, 2000 as u64);
    });
}
