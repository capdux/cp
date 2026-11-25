// cap/test/testUlidDeep.cp
// Deep unit tests for cap:ulid (Rust `ulid` crate parity: u128 roundtrip, increment() overflow guard,
// Crockford Base32 lenient/strict decode, UUID roundtrip, and MonotonicUlidGenerator).

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { Ulid, MonotonicUlidGenerator } from "cap:ulid";

fn main() {
    test("1. cap:ulid — fromU128 / toU128 & increment() Boundary", () => {
        const ts: u64 = 1700000000000;
        const ent: [10]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFE];
        const u0 = Ulid.fromParts(ts, ent) catch unreachable;
        const v128 = u0.toU128();
        const round = Ulid.fromU128(v128);
        expect_eq(round.equals(u0), true);
        expect_eq(round.timestampMs(), ts);

        const u1 = u0.increment().?;
        expect_eq(u1.timestampMs(), ts);
        expect_eq(u1.toU128(), v128 + 1);
        expect_eq(u0.compare(u1), -1 as i8);

        // Overflow guard: 80-bit entropy all 0xFF must return null and never touch timestamp
        const max_ent: [10]u8 = [0xFF; 10];
        const umax = Ulid.fromParts(ts, max_ent) catch unreachable;
        expect_eq(umax.increment() == null, true);
        expect_eq(Ulid.max().increment() == null, true);
    });

    test("2. cap:ulid — Crockford Base32 & UUID Lossless Roundtrip", () => {
        const ent2: [10]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
        const u = Ulid.fromParts(1469918176385, ent2) catch unreachable;
        const enc = u.encode();
        const dec_strict = Ulid.decodeStrict(enc[0..]) catch unreachable;
        expect_eq(dec_strict.equals(u), true);

        const uuid = u.toUuidString();
        const from_uuid = Ulid.fromUuidString(uuid[0..]) catch unreachable;
        expect_eq(from_uuid.equals(u), true);
    });

    test("3. cap:ulid — MonotonicUlidGenerator Strict Same-ms Ordering", () => {
        const seed: [10]u8 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
        let gen = MonotonicUlidGenerator.init(seed);
        const a = gen.generateAt(1700000000123) catch unreachable;
        const b = gen.generateAt(1700000000123) catch unreachable;
        const c = gen.generateAt(1700000000124) catch unreachable;
        expect_eq(a.compare(b), -1 as i8);
        expect_eq(b.compare(c), -1 as i8);
    });
}
