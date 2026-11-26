import { test, expect_eq } from "cap:test";
import {
    Xoroshiro128Plus,
    uniformIntDistribution,
} from "../../packages/@cap/pure-rand/src/index.cp";

fn main() {
    test("1. @cap/pure-rand — pure immutability yields identical output from same state", () => {
        const rng = Xoroshiro128Plus.fromSeed(42);
        const r1 = rng.nextU64();
        const r2 = rng.nextU64();
        expect_eq(r1.value, r2.value);
        expect_eq(r1.next.s0, r2.next.s0);
        expect_eq(r1.next.s1, r2.next.s1);
    });

    test("2. @cap/pure-rand — advancing state produces distinct sequence", () => {
        const rng = Xoroshiro128Plus.fromSeed(123456);
        const r1 = rng.nextU64();
        const g1: Xoroshiro128Plus = r1.next;
        const r2 = g1.nextU64();
        const g2: Xoroshiro128Plus = r2.next;
        const r3 = g2.nextU64();
        expect_eq(r1.value != r2.value, true);
        expect_eq(r2.value != r3.value, true);
    });

    test("3. @cap/pure-rand — jump() produces deterministic independent substream", () => {
        const base = Xoroshiro128Plus.fromSeed(777);
        const jumped1 = base.jump();
        const jumped2 = base.jump();
        expect_eq(jumped1.s0, jumped2.s0);
        expect_eq(jumped1.s1, jumped2.s1);
        expect_eq(jumped1.nextU64().value != base.nextU64().value, true);
    });

    test("4. @cap/pure-rand — uniformIntDistribution stays strictly inside [min, max]", () => {
        let cur = Xoroshiro128Plus.fromSeed(999);
        let saw_neg = false;
        let saw_pos = false;
        let i: usize = 0;
        while (i < 200) {
            const r = uniformIntDistribution(-10, 10, cur);
            cur = r.next;
            expect_eq(r.value >= -10 && r.value <= 10, true);
            if (r.value < 0) { saw_neg = true; }
            if (r.value > 0) { saw_pos = true; }
            i += 1;
        }
        expect_eq(saw_neg, true);
        expect_eq(saw_pos, true);
    });

    test("5. @cap/pure-rand — uniformIntDistribution degenerate range returns min", () => {
        const rng = Xoroshiro128Plus.fromSeed(1);
        const r = uniformIntDistribution(55, 55, rng);
        expect_eq(r.value, 55 as i64);
    });

    test("6. @cap/pure-rand — nextI32 produces deterministic signed values", () => {
        const rng = Xoroshiro128Plus.fromSeed(2026);
        const a = rng.nextI32();
        const ga: Xoroshiro128Plus = a.next;
        const b = ga.nextI32();
        expect_eq(a.value != b.value, true);
        expect_eq(rng.nextI32().value, a.value);
    });
}
