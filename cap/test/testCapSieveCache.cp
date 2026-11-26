// cap/test/testCapSieveCache.cp
// Comprehensive unit tests for @cap/sieve-cache

import { test, expect_eq } from "cap:test";
import {
    MAX_CAP,
    SieveCache,
} from "../../packages/@cap/sieve-cache/src/index.cp";

fn main() {
    test("1. @cap/sieve-cache — Basic put, get, peek, contains & stats", () => {
        let c = SieveCache.init(4);
        expect_eq(c.isEmpty(), true);
        expect_eq(c.capacity(), 4 as usize);

        expect_eq(c.put(10, 100), true);
        expect_eq(c.put(20, 200), true);
        expect_eq(c.len(), 2 as usize);

        // peek & contains do not alter hit/miss counters
        expect_eq(c.contains(10), true);
        expect_eq(c.peek(10), 100 as i64);
        expect_eq(c.peek(99), -1 as i64);
        expect_eq(c.hits(), 0 as u64);
        expect_eq(c.misses(), 0 as u64);

        expect_eq(c.get(10), 100 as i64);
        expect_eq(c.get(99), -1 as i64);
        expect_eq(c.hits(), 1 as u64);
        expect_eq(c.misses(), 1 as u64);
    });

    test("2. @cap/sieve-cache — Updating existing key never triggers eviction", () => {
        let c = SieveCache.init(2);
        expect_eq(c.put(1, 10), true);
        expect_eq(c.put(2, 20), true);
        expect_eq(c.put(1, 11), true);
        expect_eq(c.len(), 2 as usize);
        expect_eq(c.evictions(), 0 as u64);
        expect_eq(c.peek(1), 11 as i64);
        expect_eq(c.peek(2), 20 as i64);
    });

    test("3. @cap/sieve-cache — SIEVE hand second-chance & exact eviction order", () => {
        // Capacity 3: insert 1, 2, 3 => queue head->tail: [3(0), 2(0), 1(0)]
        let c = SieveCache.init(3);
        expect_eq(c.put(1, 10), true);
        expect_eq(c.put(2, 20), true);
        expect_eq(c.put(3, 30), true);

        // Touch key 1 => visited=1: [3(0), 2(0), 1(1)]
        expect_eq(c.get(1), 10 as i64);

        // Insert 4: hand starts at tail(1), clears 1's visited, moves to 2(0), evicts 2!
        // Remaining: [4(0), 3(0), 1(0)], hand points to 3
        expect_eq(c.put(4, 40), true);
        expect_eq(c.evictions(), 1 as u64);
        expect_eq(c.contains(1), true);
        expect_eq(c.contains(2), false);
        expect_eq(c.contains(3), true);
        expect_eq(c.contains(4), true);

        // Insert 5: hand is at 3(0) => immediately evicts 3!
        expect_eq(c.put(5, 50), true);
        expect_eq(c.evictions(), 2 as u64);
        expect_eq(c.contains(3), false);
        expect_eq(c.contains(5), true);
    });

    test("4. @cap/sieve-cache — Hot key scan-resistance protection", () => {
        let c = SieveCache.init(4);
        expect_eq(c.put(1, 100), true);
        expect_eq(c.put(2, 200), true);
        expect_eq(c.put(3, 300), true);
        expect_eq(c.put(4, 400), true);

        // Mark hot keys 1 and 2 as visited
        expect_eq(c.get(1), 100 as i64);
        expect_eq(c.get(2), 200 as i64);

        // One-hit-wonder scan of 2 new cold keys should evict cold keys 3 & 4, sparing 1 & 2!
        expect_eq(c.put(100, 1), true);
        expect_eq(c.put(101, 2), true);
        expect_eq(c.contains(1), true);
        expect_eq(c.contains(2), true);
        expect_eq(c.contains(3), false);
        expect_eq(c.contains(4), false);
    });

    test("5. @cap/sieve-cache — remove() on hand, head, and tail boundaries", () => {
        let c = SieveCache.init(3);
        expect_eq(c.put(1, 10), true);
        expect_eq(c.put(2, 20), true);
        expect_eq(c.put(3, 30), true);
        // Trigger one eviction so hand points to a live node
        expect_eq(c.get(1), 10 as i64);
        expect_eq(c.put(4, 40), true); // evicts 2, hand -> 3, list: [4, 3, 1]

        // Remove node currently pointed to by hand (3), then head (4), then tail (1)
        expect_eq(c.remove(3), true);
        expect_eq(c.remove(3), false);
        expect_eq(c.remove(4), true);
        expect_eq(c.remove(1), true);
        expect_eq(c.isEmpty(), true);

        // Cache remains completely usable after removing all boundary nodes
        expect_eq(c.put(77, 770), true);
        expect_eq(c.get(77), 770 as i64);
    });

    test("6. @cap/sieve-cache — clear(), capacity clamping & stats reset", () => {
        let c = SieveCache.init(999);
        expect_eq(c.capacity(), MAX_CAP);

        let k: u64 = 0;
        while (k < 200) {
            expect_eq(c.put(k, (k * 2) as i64), true);
            k += 1;
        }
        expect_eq(c.len(), MAX_CAP);
        expect_eq(c.evictions(), (200 - MAX_CAP) as u64);

        c.clear();
        expect_eq(c.isEmpty(), true);
        expect_eq(c.hits(), 0 as u64);
        expect_eq(c.misses(), 0 as u64);
        expect_eq(c.evictions(), 0 as u64);
        expect_eq(c.put(42, 420), true);
        expect_eq(c.get(42), 420 as i64);
    });
}
