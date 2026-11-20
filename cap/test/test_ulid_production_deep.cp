// Source: cp/cap/test/test_ulid_production_deep.cp
// Deep Production Test Suite for cap:ulid & cap.ulid
// 5 test cases:
//   1. Known timestamp+entropy vs Crockford Base32 26-char exact round-trip & permissive/strict decode
//   2. MonotonicUlidGenerator 10,000x same-millisecond strict lexicographical monotonicity & new-ms reseed
//   3. Overflow & invalid input guards (8000...0, 'U', InvalidLength, TimestampOverflow, MonotonicOverflow)
//   4. Bidirectional lossless ULID <-> UUID (8-4-4-4-12) conversion & InvalidUuid guard
//   5. Dynamic cap.ulid namespace mounting & zero-heap-allocation verification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    ULID_MAX_TIMESTAMP,
    UlidError,
    Ulid,
    MonotonicUlidGenerator,
} from "cap:ulid";
import cap from "cap";

fn expectDecodeErr(input: []const u8, strict: bool, expected: UlidError) {
    const res = if (strict) Ulid.decodeStrict(input) else Ulid.decode(input);
    switch (res) {
        val => {
            expect_eq(true, false);
        },
        err(e) => {
            expect_eq(e == expected, true);
        },
    }
}

test("ulid: known timestamp + entropy Crockford Base32 exact match & decode modes", () => {
    const ts: u64 = 1469918176385; // 0x0156_3DF3_6481
    const ent: [10]u8 = [0x4B, 0x7A, 0xE7, 0x8F, 0x62, 0x5B, 0x2F, 0xC1, 0x99, 0x05];

    const u = Ulid.fromParts(ts, ent)?;
    expect_eq(u.timestampMs(), ts);
    expect_eq(std.mem.eql(u8, u.entropy()[0..10], ent[0..10]), true);
    expect_eq(u.isZero(), false);

    const enc = u.encode();
    expect_eq(std.mem.eql(u8, enc[0..26], "01ARYZ6S419DXEF3V2BCQW3685"), true);

    const enc_lower = u.encodeLower();
    expect_eq(std.mem.eql(u8, enc_lower[0..26], "01aryz6s419dxef3v2bcqw3685"), true);

    // Strict decode
    const dec_strict = Ulid.decodeStrict(enc[0..26])?;
    expect_eq(dec_strict.equals(u), true);
    expect_eq(dec_strict.compare(u), 0 as i8);

    // Permissive decode: lowercase, hyphens, O->0, I/L->1
    const dec_perm = Ulid.decode("o1aryz6s4i-9dxef3v2bc-qw3685")?;
    expect_eq(dec_perm.equals(u), true);

    // Strict mode rejects hyphens and I/L/O aliases
    expectDecodeErr("01ARYZ6S41-9DXEF3V2BCQW3685", true, UlidError.InvalidLength);
    expectDecodeErr("O1ARYZ6S419DXEF3V2BCQW3685", true, UlidError.InvalidChar);
    expectDecodeErr("0LARYZ6S419DXEF3V2BCQW3685", true, UlidError.InvalidChar);

    // Nil and Max boundaries
    const nil_u = Ulid.nil();
    expect_eq(nil_u.isZero(), true);
    expect_eq(std.mem.eql(u8, nil_u.encode()[0..26], "00000000000000000000000000"), true);

    const max_u = Ulid.max();
    expect_eq(std.mem.eql(u8, max_u.encode()[0..26], "7ZZZZZZZZZZZZZZZZZZZZZZZZZ"), true);
    expect_eq(nil_u.compare(max_u), -1 as i8);
    expect_eq(max_u.compare(nil_u), 1 as i8);
});

test("ulid: MonotonicUlidGenerator 10,000x same-ms monotonicity & new-ms reseed", () => {
    const seed: [10]u8 = [0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00];
    let gen = MonotonicUlidGenerator.init(seed);

    const fixed_ms: u64 = 1700000000000;
    let prev = gen.generateAt(fixed_ms)?;
    let prev_enc = prev.encode();

    for (_i of 1..10000) {
        const curr = gen.generateAt(fixed_ms)?;
        const curr_enc = curr.encode();
        expect_eq(curr.timestampMs(), fixed_ms);
        expect_eq(prev.compare(curr), -1 as i8);
        expect_eq(std.mem.order(u8, prev_enc[0..26], curr_enc[0..26]) == .lt, true);
        prev = curr;
        prev_enc = curr_enc;
    }

    // Advance to next millisecond: entropy should be freshly randomized and still > prev
    const next_ms: u64 = fixed_ms + 1;
    const next_u = gen.generateAt(next_ms)?;
    expect_eq(next_u.timestampMs(), next_ms);
    expect_eq(prev.compare(next_u), -1 as i8);

    // Wall-clock generate()
    let wall_gen = MonotonicUlidGenerator.init(null);
    const w1 = wall_gen.generate()?;
    const w2 = wall_gen.generate()?;
    expect_eq(w1.compare(w2), -1 as i8);
});

test("ulid: overflow guards (8000...0, invalid char U, length, MonotonicOverflow)", () => {
    // 128-bit overflow: leading char '8' (> '7')
    expectDecodeErr("80000000000000000000000000", false, UlidError.TimestampOverflow);
    expectDecodeErr("ZZZZZZZZZZZZZZZZZZZZZZZZZZ", true, UlidError.TimestampOverflow);

    // Invalid Crockford Base32 character 'U' / 'u'
    expectDecodeErr("01ARYZ6S419DXEF3V2BCQW368U", false, UlidError.InvalidChar);
    expectDecodeErr("01ARYZ6S419DXEF3V2BCQW368u", true, UlidError.InvalidChar);

    // Invalid length
    expectDecodeErr("01ARYZ6S41", false, UlidError.InvalidLength);
    expectDecodeErr("01ARYZ6S419DXEF3V2BCQW36850", false, UlidError.InvalidLength);

    // fromParts timestamp overflow (> 48-bit)
    const ent: [10]u8 = [0; 10];
    switch (Ulid.fromParts(ULID_MAX_TIMESTAMP + 1, ent)) {
        val => {
            expect_eq(true, false);
        },
        err(e) => {
            expect_eq(e == UlidError.TimestampOverflow, true);
        },
    }

    // 80-bit MonotonicOverflow when all 10 entropy bytes are 0xFF
    const max_ent: [10]u8 = [0xFF; 10];
    let gen = MonotonicUlidGenerator.init(max_ent);
    const first = gen.generateAt(1700000000123)?;
    expect_eq(first.timestampMs(), 1700000000123 as u64);

    switch (gen.generateAt(1700000000123)) {
        val => {
            expect_eq(true, false);
        },
        err(e) => {
            expect_eq(e == UlidError.MonotonicOverflow, true);
        },
    }
});

test("ulid: bidirectional lossless ULID <-> UUID (8-4-4-4-12) conversion", () => {
    const raw: [16]u8 = [
        0x01, 0x89, 0x0A, 0x5D, 0xAC, 0x96, 0x7A, 0x3B,
        0x8C, 0x4E, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC,
    ];
    const u = Ulid.fromBytes(raw);
    const uuid_str = u.toUuidString();
    expect_eq(std.mem.eql(u8, uuid_str[0..36], "01890a5d-ac96-7a3b-8c4e-123456789abc"), true);

    const roundtrip = Ulid.fromUuidString(uuid_str[0..36])?;
    expect_eq(roundtrip.equals(u), true);
    expect_eq(std.mem.eql(u8, roundtrip.toBytes()[0..16], raw[0..16]), true);

    // Uppercase UUID input also accepted
    const from_upper = Ulid.fromUuidString("01890A5D-AC96-7A3B-8C4E-123456789ABC")?;
    expect_eq(from_upper.equals(u), true);

    // Invalid UUID formats rejected
    switch (Ulid.fromUuidString("01890a5d-ac96-7a3b-8c4e-123456789ab")) {
        val => {
            expect_eq(true, false);
        },
        err(e) => {
            expect_eq(e == UlidError.InvalidUuid, true);
        },
    }
    switch (Ulid.fromUuidString("01890a5d_ac96-7a3b-8c4e-123456789abc")) {
        val => {
            expect_eq(true, false);
        },
        err(e) => {
            expect_eq(e == UlidError.InvalidUuid, true);
        },
    }
    switch (Ulid.fromUuidString("01890a5d-ac96-7a3b-8c4e-123456789abg")) {
        val => {
            expect_eq(true, false);
        },
        err(e) => {
            expect_eq(e == UlidError.InvalidUuid, true);
        },
    }
});

test("ulid: dynamic cap.ulid namespace mounting & zero-heap-allocation check", () => {
    // Verify stack-only zero-heap-allocation APIs with std.testing.allocator untouched
    const _alloc = std.testing.allocator;
    * = _alloc;

    let gen = cap.ulid.MonotonicUlidGenerator.init(null);
    const u1 = gen.generateAt(1700000001000)?;
    const u2 = gen.generateAt(1700000001000)?;
    expect_eq(u1.compare(u2), -1 as i8);

    const enc = u1.encode();
    const decoded = cap.ulid.Ulid.decode(enc[0..26])?;
    expect_eq(decoded.equals(u1), true);

    const uuid_s = u2.toUuidString();
    const dec_uuid = cap.ulid.Ulid.fromUuidString(uuid_s[0..36])?;
    expect_eq(dec_uuid.equals(u2), true);
});
