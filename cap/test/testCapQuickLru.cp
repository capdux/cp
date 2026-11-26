import { test, expect_eq } from "cap:test";
import { QuickLruI64 } from "../../packages/@cap/quick-lru/src/index.cp";

fn main() {
    test("1. @cap/quick-lru — set, get, has & peek basics", () => {
        let c = QuickLruI64.new(3);
        expect_eq(c.maxSize(), 3);
        c.set("a", 10);
        c.set("b", 20);
        expect_eq(c.size(), 2);
        expect_eq(c.has("a"), true);
        expect_eq(c.peek("b") ?? -1, 20);
        expect_eq(c.get("a") ?? -1, 10);
        expect_eq(c.get("missing") == null, true);
    });

    test("2. @cap/quick-lru — Generation rotation preserves recent old entries", () => {
        let c = QuickLruI64.new(2);
        c.set("a", 1);
        c.set("b", 2);
        // Triggers rotation: old={a,b}, cur={c}
        c.set("c", 3);
        expect_eq(c.has("a"), true);
        expect_eq(c.has("b"), true);
        expect_eq(c.has("c"), true);
        expect_eq(c.size(), 3);
    });

    test("3. @cap/quick-lru — get promotes from old to cur so it survives next rotation", () => {
        let c = QuickLruI64.new(2);
        c.set("a", 1);
        c.set("b", 2);
        c.set("c", 3); // old={a,b}, cur={c}
        expect_eq(c.get("a") ?? -1, 1); // promotes 'a' -> cur={c,a}, old={b}
        c.set("d", 4); // rotates: old={c,a}, cur={d}; 'b' evicted!
        expect_eq(c.has("b"), false);
        expect_eq(c.has("a"), true);
        expect_eq(c.has("c"), true);
        expect_eq(c.has("d"), true);
    });

    test("4. @cap/quick-lru — Updating existing key overwrites value without duplicating", () => {
        let c = QuickLruI64.new(3);
        c.set("k", 1);
        c.set("k", 42);
        expect_eq(c.size(), 1);
        expect_eq(c.get("k") ?? -1, 42);
    });

    test("5. @cap/quick-lru — delete removes from both generations", () => {
        let c = QuickLruI64.new(2);
        c.set("a", 1);
        c.set("b", 2);
        c.set("c", 3); // a,b in old; c in cur
        expect_eq(c.delete("a"), true);
        expect_eq(c.delete("c"), true);
        expect_eq(c.delete("nope"), false);
        expect_eq(c.has("a"), false);
        expect_eq(c.has("c"), false);
        expect_eq(c.has("b"), true);
    });

    test("6. @cap/quick-lru — clear empties both generations cleanly", () => {
        let c = QuickLruI64.new(2);
        c.set("x", 1); c.set("y", 2); c.set("z", 3);
        c.clear();
        expect_eq(c.size(), 0);
        expect_eq(c.has("x"), false);
        c.set("new", 99);
        expect_eq(c.get("new") ?? -1, 99);
    });
}
