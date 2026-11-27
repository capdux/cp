import { test, expect_eq } from "cap:test";
import {
    IntHasher,
    NoHashHasher,
    BuildNoHashHasher,
    newIntHasher,
    newNoHashHasher,
    nohashU8,
    nohashU16,
    nohashU32,
    nohashU64,
    nohashUsize,
    nohashI8,
    nohashI16,
    nohashI32,
    nohashI64,
    nohashIsize,
    fibBucketIndex,
    mixU64,
    unmixU64,
    IntMap64,
    IntSet64,
} from "../../packages/nohash-hasher/src/index.cp";

fn main() {
    test("1. @cap/nohash-hasher — IntHasher returns exact u64 and u32 identity values", () => {
        let h = newIntHasher();
        h.writeU64(0x123456789ABCDEF0);
        expect_eq(h.finish(), 0x123456789ABCDEF0 as u64);

        h.writeU32(42);
        expect_eq(h.finish(), 42 as u64);
    });

    test("2. @cap/nohash-hasher — IntHasher bit-casts negative i64 without loss", () => {
        let h = newIntHasher();
        h.writeI64(-1);
        const all_ones: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        expect_eq(h.finish(), all_ones);
    });

    test("3. @cap/nohash-hasher — All IsEnabled integer write methods, reset & clone", () => {
        let h: NoHashHasher = newNoHashHasher();
        h.writeU8(0xAB);
        expect_eq(h.finish(), 0xAB as u64);
        h.writeU16(0xBEEF);
        expect_eq(h.finish(), 0xBEEF as u64);
        h.writeUsize(123456);
        expect_eq(h.finish(), 123456 as u64);
        h.writeI8(-1);
        expect_eq(h.finish(), 0xFF as u64);
        h.writeI16(-1);
        expect_eq(h.finish(), 0xFFFF as u64);
        h.writeI32(-1);
        expect_eq(h.finish(), 0xFFFFFFFF as u64);
        const c: IntHasher = h.clone();
        expect_eq(c.finish(), 0xFFFFFFFF as u64);
        h.reset();
        expect_eq(h.finish(), 0 as u64);
        expect_eq(c.finish(), 0xFFFFFFFF as u64);
    });

    test("4. @cap/nohash-hasher — BuildNoHashHasher and identity helper functions", () => {
        const builder = BuildNoHashHasher.new();
        let h = builder.buildHasher();
        expect_eq(h.finish(), 0 as u64);
        h.writeU32(77);
        expect_eq(h.finish(), 77 as u64);

        expect_eq(nohashU8(255), 255 as u64);
        expect_eq(nohashU16(65535), 65535 as u64);
        expect_eq(nohashU32(1024), 1024 as u64);
        expect_eq(nohashU64(9876543210), 9876543210 as u64);
        expect_eq(nohashUsize(4096), 4096 as u64);
        expect_eq(nohashI8(-1), 0xFF as u64);
        expect_eq(nohashI16(-1), 0xFFFF as u64);
        expect_eq(nohashI32(-1), 0xFFFFFFFF as u64);
        const all_ones: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        expect_eq(nohashI64(-1), all_ones);
        expect_eq(nohashIsize(99), 99 as u64);
    });

    test("5. @cap/nohash-hasher — fibBucketIndex stays within 2^bits range", () => {
        const b1 = fibBucketIndex(1, 8);
        const b2 = fibBucketIndex(2, 8);
        const b3 = fibBucketIndex(3, 8);
        expect_eq(b1 < 256, true);
        expect_eq(b2 < 256, true);
        expect_eq(b3 < 256, true);
        expect_eq(b1 != b2, true);
        expect_eq(fibBucketIndex(12345, 0), 0 as usize);
    });

    test("6. @cap/nohash-hasher — mixU64 and unmixU64 are exact inverses", () => {
        const m0 = mixU64(0);
        const m1 = mixU64(1);
        const m2 = mixU64(2);
        expect_eq(m0, 0 as u64);
        expect_eq(m1 != m2, true);
        expect_eq(m1 != 1, true);
        expect_eq(mixU64(0xDEADBEEF), mixU64(0xDEADBEEF));

        expect_eq(unmixU64(mixU64(0)), 0 as u64);
        expect_eq(unmixU64(mixU64(1)), 1 as u64);
        expect_eq(unmixU64(mixU64(0x123456789ABCDEF0)), 0x123456789ABCDEF0 as u64);
    });

    test("7. @cap/nohash-hasher — IntMap64 insert, overwrite, collision, remove, clear", () => {
        let m = IntMap64.init();
        expect_eq(m.isEmpty(), true);
        expect_eq(m.insert(10, 100), true);
        expect_eq(m.insert(74, 740), true); // collides mod 64 with 10
        expect_eq(m.len(), 2 as usize);
        expect_eq(m.get(10) ?? 0, 100 as u64);
        expect_eq(m.get(74) ?? 0, 740 as u64);
        expect_eq(m.containsKey(99), false);

        // Overwrite existing key
        expect_eq(m.insert(10, 101), true);
        expect_eq(m.len(), 2 as usize);
        expect_eq(m.get(10) ?? 0, 101 as u64);

        // Remove first in probe chain, second must remain reachable
        expect_eq(m.remove(10) ?? 0, 101 as u64);
        expect_eq(m.get(10) == null, true);
        expect_eq(m.get(74) ?? 0, 740 as u64);
        expect_eq(m.len(), 1 as usize);

        m.clear();
        expect_eq(m.isEmpty(), true);
        expect_eq(m.get(74) == null, true);
    });

    test("8. @cap/nohash-hasher — IntSet64 insert, dedup, remove, clear", () => {
        let s = IntSet64.init();
        expect_eq(s.isEmpty(), true);
        expect_eq(s.insert(5), true);
        expect_eq(s.insert(69), true); // collides mod 64 with 5
        expect_eq(s.insert(5), true); // duplicate
        expect_eq(s.len(), 2 as usize);
        expect_eq(s.contains(5), true);
        expect_eq(s.contains(69), true);
        expect_eq(s.contains(133), false);

        expect_eq(s.remove(5), true);
        expect_eq(s.remove(5), false);
        expect_eq(s.contains(69), true);
        expect_eq(s.len(), 1 as usize);

        s.clear();
        expect_eq(s.isEmpty(), true);
    });
}
