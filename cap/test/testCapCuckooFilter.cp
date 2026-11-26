// cap/test/testCapCuckooFilter.cp
// Comprehensive unit tests for @cap/cuckoo-filter

import { test, expect_eq } from "cap:test";
import {
    TOTAL_SLOTS,
    CuckooFilter,
} from "../../packages/@cap/cuckoo-filter/src/index.cp";

fn formatKey(prefix: u8, id: usize, buf: []u8) -> string {
    buf[0] = prefix;
    buf[1] = '-';
    buf[2] = ('0' as usize + ((id / 1000) % 10)) as u8;
    buf[3] = ('0' as usize + ((id / 100) % 10)) as u8;
    buf[4] = ('0' as usize + ((id / 10) % 10)) as u8;
    buf[5] = ('0' as usize + (id % 10)) as u8;
    return buf[0..6];
}

fn main() {
    test("1. @cap/cuckoo-filter — Empty filter boundaries", () => {
        let cf = CuckooFilter.init();
        expect_eq(cf.isEmpty(), true);
        expect_eq(cf.len(), 0 as usize);
        expect_eq(cf.capacity(), TOTAL_SLOTS);
        expect_eq(cf.loadFactor(), 0.0);
        expect_eq(cf.contains("hello"), false);
        expect_eq(cf.delete("hello"), false);
    });

    test("2. @cap/cuckoo-filter — Insert & zero false-negative contains", () => {
        let cf = CuckooFilter.init();
        expect_eq(cf.insert("alice@capdux.org"), true);
        expect_eq(cf.insert("bob@capdux.org"), true);
        expect_eq(cf.insert("charlie@capdux.org"), true);
        expect_eq(cf.len(), 3 as usize);
        expect_eq(cf.isEmpty(), false);

        expect_eq(cf.contains("alice@capdux.org"), true);
        expect_eq(cf.contains("bob@capdux.org"), true);
        expect_eq(cf.contains("charlie@capdux.org"), true);
        expect_eq(cf.contains("never-inserted"), false);
    });

    test("3. @cap/cuckoo-filter — Exact delete without affecting sibling items", () => {
        let cf = CuckooFilter.init();
        expect_eq(cf.insert("item-1"), true);
        expect_eq(cf.insert("item-2"), true);
        expect_eq(cf.insert("item-3"), true);

        expect_eq(cf.delete("item-2"), true);
        expect_eq(cf.len(), 2 as usize);
        expect_eq(cf.contains("item-2"), false);
        expect_eq(cf.contains("item-1"), true);
        expect_eq(cf.contains("item-3"), true);
        expect_eq(cf.delete("item-2"), false);
    });

    test("4. @cap/cuckoo-filter — Multiplicity (duplicate inserts & step-by-step deletes)", () => {
        let cf = CuckooFilter.init();
        expect_eq(cf.insert("dup-key"), true);
        expect_eq(cf.insert("dup-key"), true);
        expect_eq(cf.insert("dup-key"), true);
        expect_eq(cf.len(), 3 as usize);

        expect_eq(cf.delete("dup-key"), true);
        expect_eq(cf.len(), 2 as usize);
        expect_eq(cf.contains("dup-key"), true);

        expect_eq(cf.delete("dup-key"), true);
        expect_eq(cf.len(), 1 as usize);
        expect_eq(cf.contains("dup-key"), true);

        expect_eq(cf.delete("dup-key"), true);
        expect_eq(cf.len(), 0 as usize);
        expect_eq(cf.contains("dup-key"), false);
        expect_eq(cf.delete("dup-key"), false);
    });

    test("5. @cap/cuckoo-filter — High-load 500-item insert (triggers kicks), zero FN & low FP", () => {
        let cf = CuckooFilter.init();
        let buf: [8]u8 = undefined;

        let i: usize = 0;
        while (i < 500) {
            const k_ins = formatKey('K', i, buf[0..8]);
            expect_eq(cf.insert(k_ins), true);
            i += 1;
        }
        expect_eq(cf.len(), 500 as usize);
        expect_eq(cf.loadFactor() > 0.48 && cf.loadFactor() < 0.50, true);

        // Zero false negatives across all 500 inserted keys
        i = 0;
        while (i < 500) {
            const k_chk = formatKey('K', i, buf[0..8]);
            expect_eq(cf.contains(k_chk), true);
            i += 1;
        }

        // Low false positive rate across 500 unseen keys (16-bit fp => theoretical ~0.01%)
        let fps: usize = 0;
        i = 0;
        while (i < 500) {
            const k_fp = formatKey('U', i, buf[0..8]);
            if (cf.contains(k_fp)) {
                fps += 1;
            }
            i += 1;
        }
        expect_eq(fps < (5 as usize), true);
    });

    test("6. @cap/cuckoo-filter — clear() resets all slots for full reuse", () => {
        let cf = CuckooFilter.init();
        expect_eq(cf.insert("alpha"), true);
        expect_eq(cf.insert("beta"), true);
        cf.clear();
        expect_eq(cf.isEmpty(), true);
        expect_eq(cf.len(), 0 as usize);
        expect_eq(cf.contains("alpha"), false);
        expect_eq(cf.insert("gamma"), true);
        expect_eq(cf.contains("gamma"), true);
    });
}
