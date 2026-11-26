import { test, expect_eq } from "cap:test";
import { NONE_IDX, SlabEntry, LruSlab } from "../../packages/@cap/lru-slab/src/index.cp";

fn emptyEntry(): SlabEntry {
    return SlabEntry { occupied: false, key: 0, val: 0, prev: NONE_IDX, next: NONE_IDX };
}

fn main() {
    test("insert, MRU/LRU order & O(1) eviction", () => {
        let pool: [4]SlabEntry = [emptyEntry(); 4];
        let slab = LruSlab.new();
        expect_eq(slab.init(pool[0..4]), true);
        let a = slab.insert(pool[0..4], 10, 100);
        let b = slab.insert(pool[0..4], 20, 200);
        let c = slab.insert(pool[0..4], 30, 300);
        expect_eq(slab.len(), 3 as usize);
        expect_eq(slab.peekMru(), c);
        expect_eq(slab.peekLru(), a);
        let ev = slab.evictLru(pool[0..4]);
        expect_eq(ev.idx, a);
        expect_eq(ev.key, 10 as u64);
        expect_eq(ev.val, 100 as u64);
        expect_eq(slab.peekLru(), b);
    });

    test("get & touch promote entry to MRU head", () => {
        let pool: [3]SlabEntry = [emptyEntry(); 3];
        let slab = LruSlab.new();
        * = slab.init(pool[0..3]);
        let a = slab.insert(pool[0..3], 1, 11);
        let b = slab.insert(pool[0..3], 2, 22);
        let c = slab.insert(pool[0..3], 3, 33);
        let g = slab.get(pool[0..3], a);
        expect_eq(g.ok, true);
        expect_eq(g.val, 11 as u64);
        expect_eq(slab.peekMru(), a);
        expect_eq(slab.peekLru(), b);
        expect_eq(c != NONE_IDX, true);
    });

    test("full slab rejects insert until remove or evict", () => {
        let pool: [2]SlabEntry = [emptyEntry(); 2];
        let slab = LruSlab.new();
        * = slab.init(pool[0..2]);
        let a = slab.insert(pool[0..2], 1, 10);
        * = slab.insert(pool[0..2], 2, 20);
        expect_eq(slab.isFull(), true);
        expect_eq(slab.insert(pool[0..2], 3, 30), NONE_IDX);
        expect_eq(slab.remove(pool[0..2], a), true);
        expect_eq(slab.isFull(), false);
        let d = slab.insert(pool[0..2], 3, 30);
        expect_eq(d != NONE_IDX, true);
        expect_eq(slab.len(), 2 as usize);
    });

    test("remove middle, head & tail cleanly", () => {
        let pool: [3]SlabEntry = [emptyEntry(); 3];
        let slab = LruSlab.new();
        * = slab.init(pool[0..3]);
        let a = slab.insert(pool[0..3], 1, 1);
        let b = slab.insert(pool[0..3], 2, 2);
        let c = slab.insert(pool[0..3], 3, 3);
        expect_eq(slab.remove(pool[0..3], b), true);
        expect_eq(slab.peekMru(), c);
        expect_eq(slab.peekLru(), a);
        expect_eq(slab.remove(pool[0..3], c), true);
        expect_eq(slab.remove(pool[0..3], a), true);
        expect_eq(slab.isEmpty(), true);
        expect_eq(slab.peekMru(), NONE_IDX);
        expect_eq(slab.peekLru(), NONE_IDX);
    });

    test("invalid index & double-remove safety", () => {
        let pool: [2]SlabEntry = [emptyEntry(); 2];
        let slab = LruSlab.new();
        * = slab.init(pool[0..2]);
        let a = slab.insert(pool[0..2], 5, 50);
        expect_eq(slab.remove(pool[0..2], a), true);
        expect_eq(slab.remove(pool[0..2], a), false);
        expect_eq(slab.remove(pool[0..2], -1), false);
        expect_eq(slab.remove(pool[0..2], 99), false);
        expect_eq(slab.get(pool[0..2], a).ok, false);
    });

    test("zero capacity slab boundary safety", () => {
        let pool: [1]SlabEntry = [emptyEntry(); 1];
        let slab = LruSlab.new();
        * = slab.init(pool[0..0]);
        expect_eq(slab.isEmpty(), true);
        expect_eq(slab.isFull(), true);
        expect_eq(slab.insert(pool[0..0], 1, 1), NONE_IDX);
        expect_eq(slab.evictLru(pool[0..0]).idx, NONE_IDX);
    });
}
