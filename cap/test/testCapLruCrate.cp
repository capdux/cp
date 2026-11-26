import { test, expect_eq } from "cap:test";
import { LruCacheI64 } from "../../packages/@cap/lru-crate/src/index.cp";

fn main() {
    test("1. @cap/lru-crate — put, get, peek & MRU/LRU order", () => {
        let c = LruCacheI64.withCapacity(3);
        expect_eq(c.isEmpty(), true);
        expect_eq(c.put("a", 1) == null, true);
        expect_eq(c.put("b", 2) == null, true);
        expect_eq(c.put("c", 3) == null, true);
        expect_eq(c.len(), 3);
        expect_eq(c.peekMruKey() ?? "", "c");
        expect_eq(c.peekLruKey() ?? "", "a");
        // peek does not change recency
        expect_eq(c.peek("a") ?? -1, 1);
        expect_eq(c.peekLruKey() ?? "", "a");
    });

    test("2. @cap/lru-crate — get promotes entry to MRU and changes eviction victim", () => {
        let c = LruCacheI64.withCapacity(3);
        c.put("a", 1);
        c.put("b", 2);
        c.put("c", 3);
        expect_eq(c.get("a") ?? -1, 1); // order: a, c, b
        expect_eq(c.peekMruKey() ?? "", "a");
        expect_eq(c.peekLruKey() ?? "", "b");
        c.put("d", 4); // evicts "b"
        expect_eq(c.contains("b"), false);
        expect_eq(c.contains("a"), true);
        expect_eq(c.contains("d"), true);
        expect_eq(c.len(), 3);
    });

    test("3. @cap/lru-crate — Updating existing key returns old value and promotes", () => {
        let c = LruCacheI64.withCapacity(2);
        c.put("x", 10);
        c.put("y", 20);
        expect_eq(c.put("x", 99) ?? -1, 10);
        expect_eq(c.peek("x") ?? -1, 99);
        expect_eq(c.peekMruKey() ?? "", "x");
        expect_eq(c.len(), 2);
    });

    test("4. @cap/lru-crate — pop & popLru remove entries cleanly", () => {
        let c = LruCacheI64.withCapacity(4);
        c.put("k1", 100);
        c.put("k2", 200);
        c.put("k3", 300);
        expect_eq(c.pop("k2") ?? -1, 200);
        expect_eq(c.contains("k2"), false);
        expect_eq(c.popLru() ?? -1, 100);
        expect_eq(c.len(), 1);
        expect_eq(c.peekMruKey() ?? "", "k3");
    });

    test("5. @cap/lru-crate — Capacity-1 cache evicts on every new key", () => {
        let c = LruCacheI64.withCapacity(1);
        c.put("only", 1);
        c.put("next", 2);
        expect_eq(c.len(), 1);
        expect_eq(c.contains("only"), false);
        expect_eq(c.get("next") ?? -1, 2);
    });

    test("6. @cap/lru-crate — clear empties cache and allows full reuse", () => {
        let c = LruCacheI64.withCapacity(3);
        c.put("a", 1); c.put("b", 2);
        c.clear();
        expect_eq(c.isEmpty(), true);
        expect_eq(c.peekLruKey() == null, true);
        c.put("z", 42);
        expect_eq(c.get("z") ?? -1, 42);
    });
}
