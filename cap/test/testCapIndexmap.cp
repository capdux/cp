// cap/test/testCapIndexmap.cp
// Comprehensive unit tests for @cap/indexmap (Rust indexmap v2 parity)

import { test, expect_eq } from "cap:test";
import {
    IndexMapU64,
    IndexMapStr
} from "../../packages/@cap/indexmap/src/index.cp";

fn imTestSliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/indexmap — Insertion order preservation & in-place update keeps original index", () => {
        let m = IndexMapU64.new();
        expect_eq(m.isEmpty(), true);
        expect_eq(m.capacity(), 64 as usize);

        let r1 = m.insert(30, 300);
        let r2 = m.insert(10, 100);
        let r3 = m.insert(20, 200);
        expect_eq(r1.ok, true);
        expect_eq(r1.replaced, false);
        expect_eq(r1.index, 0 as usize);
        expect_eq(r2.index, 1 as usize);
        expect_eq(r3.index, 2 as usize);
        expect_eq(m.len(), 3 as usize);

        // Update existing key 10 -> keeps index 1, returns old_val 100
        let upd = m.insert(10, 999);
        expect_eq(upd.ok, true);
        expect_eq(upd.replaced, true);
        expect_eq(upd.old_val, 100 as i64);
        expect_eq(upd.index, 1 as usize);
        expect_eq(m.len(), 3 as usize);

        // Order remains [30, 10, 20]
        expect_eq(m.getIndex(0).key, 30 as u64);
        expect_eq(m.getIndex(1).key, 10 as u64);
        expect_eq(m.getIndex(1).val, 999 as i64);
        expect_eq(m.getIndex(2).key, 20 as u64);
    });

    test("2. @cap/indexmap — get, getIndexOf, containsKey, first, last & pop", () => {
        let m = IndexMapU64.new();
        * = m.insert(100, 1);
        * = m.insert(200, 2);
        * = m.insert(300, 3);

        expect_eq(m.containsKey(200), true);
        expect_eq(m.containsKey(999), false);
        expect_eq(m.getIndexOf(100), 0 as i64);
        expect_eq(m.getIndexOf(300), 2 as i64);
        expect_eq(m.getIndexOf(999), -1 as i64);

        let g = m.get(200);
        expect_eq(g.found, true);
        expect_eq(g.val, 2 as i64);

        expect_eq(m.first().key, 100 as u64);
        expect_eq(m.last().key, 300 as u64);

        let p = m.pop();
        expect_eq(p.found, true);
        expect_eq(p.key, 300 as u64);
        expect_eq(p.val, 3 as i64);
        expect_eq(m.len(), 2 as usize);
        expect_eq(m.containsKey(300), false);
        expect_eq(m.last().key, 200 as u64);
    });

    test("3. @cap/indexmap — O(1) swapRemove swaps last element & keeps hash buckets consistent", () => {
        let m = IndexMapU64.new();
        * = m.insert(1, 10);
        * = m.insert(2, 20);
        * = m.insert(3, 30);
        * = m.insert(4, 40);

        // Remove key 2 (index 1) -> last entry (4, 40) moves to index 1
        let rem = m.swapRemove(2);
        expect_eq(rem.found, true);
        expect_eq(rem.val, 20 as i64);
        expect_eq(m.len(), 3 as usize);
        expect_eq(m.containsKey(2), false);

        expect_eq(m.getIndex(0).key, 1 as u64);
        expect_eq(m.getIndex(1).key, 4 as u64);
        expect_eq(m.getIndex(1).val, 40 as i64);
        expect_eq(m.getIndex(2).key, 3 as u64);
        expect_eq(m.getIndexOf(4), 1 as i64);
        expect_eq(m.get(4).val, 40 as i64);

        // Removing missing key returns found=false
        expect_eq(m.swapRemove(2).found, false);
    });

    test("4. @cap/indexmap — shiftRemove preserves exact order & synchronizes all subsequent indices", () => {
        let m = IndexMapU64.new();
        * = m.insert(10, 1);
        * = m.insert(20, 2);
        * = m.insert(30, 3);
        * = m.insert(40, 4);

        let rem = m.shiftRemove(20);
        expect_eq(rem.found, true);
        expect_eq(rem.val, 2 as i64);
        expect_eq(m.len(), 3 as usize);
        expect_eq(m.containsKey(20), false);

        // Remaining order must be [10, 30, 40]
        expect_eq(m.getIndex(0).key, 10 as u64);
        expect_eq(m.getIndex(1).key, 30 as u64);
        expect_eq(m.getIndex(2).key, 40 as u64);
        expect_eq(m.getIndexOf(10), 0 as i64);
        expect_eq(m.getIndexOf(30), 1 as i64);
        expect_eq(m.getIndexOf(40), 2 as i64);
        expect_eq(m.get(30).val, 3 as i64);
        expect_eq(m.get(40).val, 4 as i64);
    });

    test("5. @cap/indexmap — Full 64-entry stress test, overflow guard & tombstone reuse after delete", () => {
        let m = IndexMapU64.new();
        let i: u64 = 0;
        while (i < 64) {
            let r = m.insert(i * 13 + 7, (i * 100) as i64);
            expect_eq(r.ok, true);
            expect_eq(r.index, i as usize);
            i = i + 1;
        }
        expect_eq(m.len(), 64 as usize);

        // 65th distinct key rejected cleanly when full
        let over = m.insert(999999, 1);
        expect_eq(over.ok, false);

        // Updating existing key still succeeds when full
        let upd = m.insert(7, 777);
        expect_eq(upd.ok, true);
        expect_eq(upd.replaced, true);
        expect_eq(m.get(7).val, 777 as i64);

        // Delete 10 keys via swapRemove, then insert 10 new keys reusing tombstone slots
        let d: u64 = 0;
        while (d < 10) {
            expect_eq(m.swapRemove(d * 13 + 7).found, true);
            d = d + 1;
        }
        expect_eq(m.len(), 54 as usize);

        let n: u64 = 0;
        while (n < 10) {
            let r_new = m.insert(10000 + n, (n + 1) as i64);
            expect_eq(r_new.ok, true);
            n = n + 1;
        }
        expect_eq(m.len(), 64 as usize);
        expect_eq(m.get(10009).val, 10 as i64);

        m.clear();
        expect_eq(m.isEmpty(), true);
        expect_eq(m.containsKey(10009), false);
    });

    test("6. @cap/indexmap — IndexMapStr ordered string-keyed mapping, update & removal", () => {
        let m = IndexMapStr.new();
        expect_eq(m.capacity(), 32 as usize);

        * = m.insert("content-type", 1);
        * = m.insert("authorization", 2);
        * = m.insert("accept", 3);
        expect_eq(m.len(), 3 as usize);

        // In-place update preserves order
        let upd = m.insert("authorization", 22);
        expect_eq(upd.replaced, true);
        expect_eq(upd.old_val, 2 as i64);
        expect_eq(upd.index, 1 as usize);

        let e0 = m.getIndex(0);
        let e1 = m.getIndex(1);
        let e2 = m.getIndex(2);
        expect_eq(imTestSliceEq(e0.keyStr(), "content-type"), true);
        expect_eq(imTestSliceEq(e1.keyStr(), "authorization"), true);
        expect_eq(e1.val, 22 as i64);
        expect_eq(imTestSliceEq(e2.keyStr(), "accept"), true);

        // shiftRemove preserves remaining order
        let rem = m.shiftRemove("authorization");
        expect_eq(rem.found, true);
        expect_eq(rem.val, 22 as i64);
        expect_eq(m.len(), 2 as usize);
        expect_eq(m.containsKey("authorization"), false);
        expect_eq(imTestSliceEq(m.getIndex(0).keyStr(), "content-type"), true);
        expect_eq(imTestSliceEq(m.getIndex(1).keyStr(), "accept"), true);

        // pop removes last ("accept")
        let popped = m.pop();
        expect_eq(popped.found, true);
        expect_eq(imTestSliceEq(popped.keyStr(), "accept"), true);
        expect_eq(m.len(), 1 as usize);
    });
}
