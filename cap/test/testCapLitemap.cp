// cap/test/testCapLitemap.cp
// Comprehensive unit tests for @cap/litemap (Rust ICU4X litemap parity)

import { test, expect_eq } from "cap:test";
import {
    LiteMapStr,
    LiteMapU64,
} from "../../packages/litemap/src/index.cp";

fn main() {
    test("1. @cap/litemap — unordered insertions stay strictly sorted lexicographically", () => {
        let m = LiteMapStr.init();
        expect_eq(m.isEmpty(), true);
        expect_eq(m.len(), 0 as usize);

        * = m.insert("zh-CN", "Chinese");
        * = m.insert("en-US", "English");
        * = m.insert("de-DE", "German");
        * = m.insert("fr-FR", "French");

        expect_eq(m.len(), 4 as usize);
        expect_eq(m.isEmpty(), false);
        expect_eq(m.entryAt(0 as usize).?.key, "de-DE");
        expect_eq(m.entryAt(1 as usize).?.key, "en-US");
        expect_eq(m.entryAt(2 as usize).?.key, "fr-FR");
        expect_eq(m.entryAt(3 as usize).?.key, "zh-CN");
        expect_eq(m.entryAt(4 as usize) == null, true);
    });

    test("2. @cap/litemap — O(log N) binarySearch hit/miss & get/containsKey", () => {
        let m = LiteMapStr.init();
        * = m.insert("b", "2");
        * = m.insert("d", "4");
        * = m.insert("f", "6");

        const hit = m.binarySearch("d");
        expect_eq(hit.found, true);
        expect_eq(hit.index, 1 as usize);

        const miss_start = m.binarySearch("a");
        expect_eq(miss_start.found, false);
        expect_eq(miss_start.index, 0 as usize);

        const miss_mid = m.binarySearch("c");
        expect_eq(miss_mid.found, false);
        expect_eq(miss_mid.index, 1 as usize);

        const miss_end = m.binarySearch("z");
        expect_eq(miss_end.found, false);
        expect_eq(miss_end.index, 3 as usize);

        expect_eq(m.get("b").?, "2");
        expect_eq(m.get("x") == null, true);
        expect_eq(m.containsKey("f"), true);
        expect_eq(m.containsKey("e"), false);
    });

    test("3. @cap/litemap — overwrite existing key returns replaced=true & prev value", () => {
        let m = LiteMapStr.init();
        const r1 = m.insert("env", "dev");
        expect_eq(r1.ok, true);
        expect_eq(r1.replaced, false);

        const r2 = m.insert("env", "prod");
        expect_eq(r2.ok, true);
        expect_eq(r2.replaced, true);
        expect_eq(r2.prev, "dev");
        expect_eq(m.len(), 1 as usize);
        expect_eq(m.get("env").?, "prod");
    });

    test("4. @cap/litemap — remove preserves sorted order & clear resets count", () => {
        let m = LiteMapStr.init();
        * = m.insert("a", "1");
        * = m.insert("b", "2");
        * = m.insert("c", "3");
        * = m.insert("d", "4");

        expect_eq(m.remove("b").?, "2");
        expect_eq(m.remove("missing") == null, true);
        expect_eq(m.len(), 3 as usize);
        expect_eq(m.entryAt(0 as usize).?.key, "a");
        expect_eq(m.entryAt(1 as usize).?.key, "c");
        expect_eq(m.entryAt(2 as usize).?.key, "d");

        m.clear();
        expect_eq(m.isEmpty(), true);
        expect_eq(m.get("a") == null, true);
    });

    test("5. @cap/litemap — LiteMapU64 numeric map operations", () => {
        let m = LiteMapU64.init();
        * = m.insert(300 as u64, -30);
        * = m.insert(100 as u64, 10);
        * = m.insert(200 as u64, 20);

        expect_eq(m.len(), 3 as usize);
        expect_eq(m.entryAt(0 as usize).?.key, 100 as u64);
        expect_eq(m.entryAt(1 as usize).?.key, 200 as u64);
        expect_eq(m.entryAt(2 as usize).?.key, 300 as u64);

        const upd = m.insert(200 as u64, 2000);
        expect_eq(upd.ok, true);
        expect_eq(upd.replaced, true);
        expect_eq(upd.prev, 20 as i64);
        expect_eq(m.get(200 as u64).?, 2000 as i64);
        expect_eq(m.remove(100 as u64).?, 10 as i64);
        expect_eq(m.containsKey(100 as u64), false);
    });

    test("6. @cap/litemap — full capacity (64) boundary & in-place update while full", () => {
        let m = LiteMapU64.init();
        let i: u64 = 0;
        while (i < 64) {
            const res = m.insert(64 - i, i as i64);
            expect_eq(res.ok, true);
            i += 1;
        }
        expect_eq(m.isFull(), true);
        expect_eq(m.len(), 64 as usize);

        // New key rejected when full
        const overflow = m.insert(999 as u64, 1);
        expect_eq(overflow.ok, false);

        // Updating an existing key still succeeds when full
        const upd = m.insert(1 as u64, 777);
        expect_eq(upd.ok, true);
        expect_eq(upd.replaced, true);
        expect_eq(m.get(1 as u64).?, 777 as i64);
    });
}
