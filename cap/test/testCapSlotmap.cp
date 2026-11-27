// cap/test/testCapSlotmap.cp
// Comprehensive unit tests for @cap/slotmap

import { test, expect_eq } from "cap:test";
import {
    SlotKey,
    SlotMapI64,
    SecondaryMapI64,
} from "../../packages/slotmap/src/index.cp";

fn main() {
    test("1. @cap/slotmap — insert, get, set, remove & basic queries", () => {
        let sm = SlotMapI64.new();
        expect_eq(sm.len(), 0 as usize);
        expect_eq(sm.capacity(), 128 as usize);
        expect_eq(sm.isEmpty(), true);

        let k1 = sm.insert(100 as i64) ?? SlotKey.nullKey();
        let k2 = sm.insert(200 as i64) ?? SlotKey.nullKey();
        expect_eq(k1.isNull(), false);
        expect_eq(k2.isNull(), false);
        expect_eq(sm.len(), 2 as usize);
        expect_eq(sm.isEmpty(), false);
        expect_eq(sm.containsKey(k1), true);
        expect_eq(sm.get(k1) ?? (-1 as i64), 100 as i64);
        expect_eq(sm.get(k2) ?? (-1 as i64), 200 as i64);

        expect_eq(sm.set(k1, 150 as i64), true);
        expect_eq(sm.get(k1) ?? (-1 as i64), 150 as i64);

        expect_eq(sm.remove(k1) ?? (-1 as i64), 150 as i64);
        expect_eq(sm.len(), 1 as usize);
        expect_eq(sm.containsKey(k1), false);
        expect_eq(sm.get(k1) == null, true);
        expect_eq(sm.set(k1, 999 as i64), false);
        expect_eq(sm.remove(k1) == null, true);
    });

    test("2. @cap/slotmap — strict ABA dangling-key protection", () => {
        let sm = SlotMapI64.new();
        let k_old = sm.insert(42 as i64) ?? SlotKey.nullKey();
        expect_eq(sm.remove(k_old) ?? (-1 as i64), 42 as i64);

        // Re-inserting reuses the same slot index with a higher odd version.
        let k_new = sm.insert(777 as i64) ?? SlotKey.nullKey();
        expect_eq(k_new.idx, k_old.idx);
        expect_eq(k_old.eq(k_new), false);

        // Stale key must be rejected for all operations.
        expect_eq(sm.containsKey(k_old), false);
        expect_eq(sm.get(k_old) == null, true);
        expect_eq(sm.set(k_old, 123 as i64), false);
        expect_eq(sm.remove(k_old) == null, true);

        // New key remains completely valid and untouched.
        expect_eq(sm.containsKey(k_new), true);
        expect_eq(sm.get(k_new) ?? (-1 as i64), 777 as i64);
    });

    test("3. @cap/slotmap — slot reuse increments odd/even version monotonically", () => {
        let sm = SlotMapI64.new();
        let k1 = sm.insert(1 as i64) ?? SlotKey.nullKey();
        expect_eq(k1.idx, 0 as u32);
        expect_eq(k1.version, 1 as u32);

        * = sm.remove(k1);
        let k2 = sm.insert(2 as i64) ?? SlotKey.nullKey();
        expect_eq(k2.idx, 0 as u32);
        expect_eq(k2.version, 3 as u32);

        * = sm.remove(k2);
        let k3 = sm.insert(3 as i64) ?? SlotKey.nullKey();
        expect_eq(k3.idx, 0 as u32);
        expect_eq(k3.version, 5 as u32);
        expect_eq(sm.get(k3) ?? (-1 as i64), 3 as i64);
    });

    test("4. @cap/slotmap — SlotKey u64 encode/decode round-trip & nullKey", () => {
        let nk = SlotKey.nullKey();
        expect_eq(nk.isNull(), true);
        expect_eq(nk.toU64(), 0 as u64);
        expect_eq(SlotKey.fromU64(0 as u64).isNull(), true);

        let k = SlotKey{ idx: 123 as u32, version: 456789 as u32 };
        let raw = k.toU64();
        let decoded = SlotKey.fromU64(raw);
        expect_eq(decoded.idx, 123 as u32);
        expect_eq(decoded.version, 456789 as u32);
        expect_eq(decoded.eq(k), true);
        expect_eq(decoded.isNull(), false);
    });

    test("5. @cap/slotmap — SecondaryMapI64 paired with SlotKey & ABA rejection", () => {
        let sm = SlotMapI64.new();
        let sec = SecondaryMapI64.new();
        expect_eq(sec.len(), 0 as usize);

        let k1 = sm.insert(10 as i64) ?? SlotKey.nullKey();
        let k2 = sm.insert(20 as i64) ?? SlotKey.nullKey();
        expect_eq(sec.insert(k1, 1000 as i64), true);
        expect_eq(sec.insert(k2, 2000 as i64), true);
        expect_eq(sec.insert(SlotKey.nullKey(), 99 as i64), false);
        expect_eq(sec.len(), 2 as usize);
        expect_eq(sec.containsKey(k1), true);
        expect_eq(sec.get(k1) ?? (-1 as i64), 1000 as i64);

        // Remove k1 from primary and reuse slot; secondary rejects stale k1 after overwrite.
        * = sm.remove(k1);
        let k1_new = sm.insert(30 as i64) ?? SlotKey.nullKey();
        expect_eq(sec.containsKey(k1_new), false);
        expect_eq(sec.insert(k1_new, 3000 as i64), true);
        expect_eq(sec.len(), 2 as usize);
        expect_eq(sec.get(k1) == null, true);
        expect_eq(sec.get(k1_new) ?? (-1 as i64), 3000 as i64);

        expect_eq(sec.remove(k2) ?? (-1 as i64), 2000 as i64);
        expect_eq(sec.len(), 1 as usize);
        expect_eq(sec.get(k2) == null, true);

        sec.clear();
        expect_eq(sec.len(), 0 as usize);
        expect_eq(sec.containsKey(k1_new), false);
    });

    test("6. @cap/slotmap — full capacity (128 slots) & clear invalidates old keys", () => {
        let sm = SlotMapI64.new();
        let first_key = SlotKey.nullKey();
        let last_key = SlotKey.nullKey();

        let i: usize = 0;
        while (i < (128 as usize)) {
            let k = sm.insert(i as i64) ?? SlotKey.nullKey();
            expect_eq(k.isNull(), false);
            if (i == (0 as usize)) {
                first_key = k;
            }
            if (i == (127 as usize)) {
                last_key = k;
            }
            i += 1;
        }

        expect_eq(sm.len(), 128 as usize);
        expect_eq(sm.insert(999 as i64) == null, true);
        expect_eq(sm.get(first_key) ?? (-1 as i64), 0 as i64);
        expect_eq(sm.get(last_key) ?? (-1 as i64), 127 as i64);

        sm.clear();
        expect_eq(sm.len(), 0 as usize);
        expect_eq(sm.isEmpty(), true);
        // Old keys must be invalidated even after clear().
        expect_eq(sm.containsKey(first_key), false);
        expect_eq(sm.get(last_key) == null, true);

        // Fresh insert after clear gets bumped version.
        let fresh = sm.insert(888 as i64) ?? SlotKey.nullKey();
        expect_eq(fresh.idx, 0 as u32);
        expect_eq(fresh.version, 3 as u32);
        expect_eq(sm.get(fresh) ?? (-1 as i64), 888 as i64);
    });
}
