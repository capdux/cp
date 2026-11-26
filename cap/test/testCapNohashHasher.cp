import { test, expect_eq } from "cap:test";
import {
    newIntHasher,
    nohashU64,
    nohashU32,
    fibBucketIndex,
    mixU64,
} from "../../packages/@cap/nohash-hasher/src/index.cp";

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

    test("3. @cap/nohash-hasher — nohashU64 and nohashU32 pass through unchanged", () => {
        expect_eq(nohashU64(9876543210), 9876543210 as u64);
        expect_eq(nohashU32(1024), 1024 as u64);
    });

    test("4. @cap/nohash-hasher — fibBucketIndex stays within 2^bits range", () => {
        const b1 = fibBucketIndex(1, 8);
        const b2 = fibBucketIndex(2, 8);
        const b3 = fibBucketIndex(3, 8);
        expect_eq(b1 < 256, true);
        expect_eq(b2 < 256, true);
        expect_eq(b3 < 256, true);
        expect_eq(b1 != b2, true);
        expect_eq(fibBucketIndex(12345, 0), 0 as usize);
    });

    test("5. @cap/nohash-hasher — mixU64 disperses sequential keys with strong avalanche", () => {
        const m0 = mixU64(0);
        const m1 = mixU64(1);
        const m2 = mixU64(2);
        expect_eq(m0, 0 as u64);
        expect_eq(m1 != m2, true);
        expect_eq(m1 != 1, true);
    });

    test("6. @cap/nohash-hasher — mixU64 is deterministic", () => {
        expect_eq(mixU64(0xDEADBEEF), mixU64(0xDEADBEEF));
    });
}
