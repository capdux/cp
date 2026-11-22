// Source: cp/cap/test/test_nanoid_production_deep.cp
// Deep Production Verification Suite for cap:nanoid & cap.nanoid
// Covers:
//   1. NanoId default 21-char, custom hex/numeric unbiased generation & invalid alphabet/length guards
//   2. Ulid 26-char Crockford Base32 encode/decode round-trip, case-insensitivity, I/L/O tolerance & '>'7' overflow guard
//   3. MonotonicUlidGenerator 1000 same-ms strictly increasing IDs & 80-bit 0xFF MonotonicOverflow guard
//   4. UuidV7 RFC 9562 version=7 / variant=2 bit layout, 36-byte parse round-trip & timestamp ordering
//   5. `import cap from "cap"; cap.nanoid` dynamic mounting & std.testing.allocator zero-leak verification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    IdError,
    IdRng,
    NanoId,
    Ulid,
    MonotonicUlidGenerator,
    UuidV7,
} from "cap:codec";

fn containsByte(haystack: []const u8, needle: u8): bool {
    for (i of 0..haystack.len) {
        if (haystack[i] == needle) return true;
    }
    return false;
}

test("cap:nanoid - NanoId default 21-char, custom alphabets & invalid alphabet/length guards") {
    const alloc = std.heap.page_allocator;
    let rng1 = IdRng.init(123456789);
    let rng2 = IdRng.init(123456789);
    let rng3 = IdRng.init(987654321);

    const id1 = NanoId.generate(alloc, &rng1, NanoId.DEFAULT_SIZE) catch unreachable;
    defer alloc.free(id1);
    const id2 = NanoId.generate(alloc, &rng2, NanoId.DEFAULT_SIZE) catch unreachable;
    defer alloc.free(id2);
    const id3 = NanoId.generate(alloc, &rng3, NanoId.DEFAULT_SIZE) catch unreachable;
    defer alloc.free(id3);

    expect_eq(id1.len, 21);
    expect_eq(std.mem.eql(u8, id1, id2), true);
    expect_eq(std.mem.eql(u8, id1, id3), false);

    for (i of 0..id1.len) {
        expect_eq(containsByte(NanoId.DEFAULT_ALPHABET, id1[i]), true);
    }

    // Custom hex alphabet (16 chars)
    const hex_alpha = "0123456789abcdef";
    const hex_id = NanoId.generateCustom(alloc, &rng1, hex_alpha, 64) catch unreachable;
    defer alloc.free(hex_id);
    expect_eq(hex_id.len, 64);
    for (i of 0..hex_id.len) {
        expect_eq(containsByte(hex_alpha, hex_id[i]), true);
    }

    // Custom numeric alphabet (10 chars - non-power-of-two rejection sampling)
    const num_alpha = "0123456789";
    const num_id = NanoId.generateCustom(alloc, &rng1, num_alpha, 128) catch unreachable;
    defer alloc.free(num_id);
    expect_eq(num_id.len, 128);
    for (i of 0..num_id.len) {
        expect_eq(containsByte(num_alpha, num_id[i]), true);
    }

    // Invalid empty alphabet
    let empty_alpha_err = false;
    * = NanoId.generateCustom(alloc, &rng1, "", 10) catch |e| {
        expect_eq(e == IdError.InvalidAlphabet, true);
        empty_alpha_err = true;
    };
    expect_eq(empty_alpha_err, true);

    // Invalid >256-byte alphabet
    let big_alpha: [257]u8 = undefined;
    for (i of 0..257) {
        big_alpha[i] = 'a';
    }
    let big_alpha_err = false;
    * = NanoId.generateCustom(alloc, &rng1, &big_alpha, 10) catch |e| {
        expect_eq(e == IdError.InvalidAlphabet, true);
        big_alpha_err = true;
    };
    expect_eq(big_alpha_err, true);

    // Invalid zero length
    let zero_len_err = false;
    * = NanoId.generate(alloc, &rng1, 0) catch |e| {
        expect_eq(e == IdError.InvalidLength, true);
        zero_len_err = true;
    };
    expect_eq(zero_len_err, true);
}

test("cap:nanoid - Ulid Crockford Base32 round-trip, I/L/O tolerance & overflow guard") {
    let rng = IdRng.init(20261120);
    const ts: u64 = 1700000000123;
    const u = Ulid.generate(&rng, ts) catch unreachable;
    expect_eq(u.timestampMs(), ts);

    const enc = u.encode();
    const decoded = Ulid.decode(&enc) catch unreachable;
    expect_eq(decoded.timestampMs(), ts);
    expect_eq(decoded.rand80(), u.rand80());

    // Exact 26-char Crockford Base32 tolerance test: lowercase + i/l -> 1, o -> 0
    const canonical = "01ARZ3NDEKTSV4RRFFQ69G5FAV";
    const tolerant  = "oiarz3ndektsv4rrffq69g5fav";
    const u_canon = Ulid.decode(canonical) catch unreachable;
    const u_tol = Ulid.decode(tolerant) catch unreachable;
    expect_eq(u_canon.timestampMs(), u_tol.timestampMs());
    expect_eq(u_canon.rand80(), u_tol.rand80());
    expect_eq(std.mem.eql(u8, &u_canon.encode(), canonical), true);

    // First char '7' is max valid (128-bit boundary); '8' overflows 128 bits
    const max_valid = Ulid.decode("7ZZZZZZZZZZZZZZZZZZZZZZZZZ") catch unreachable;
    expect_eq(max_valid.timestampMs(), 0xFFFF_FFFF_FFFF);

    let overflow_err = false;
    * = Ulid.decode("80000000000000000000000000") catch |e| {
        expect_eq(e == IdError.InvalidUlid, true);
        overflow_err = true;
    };
    expect_eq(overflow_err, true);

    // Invalid char 'U' and wrong length
    let invalid_u_err = false;
    * = Ulid.decode("01ARZ3NDEKTSV4RRFFQ69G5FAU") catch |e| {
        expect_eq(e == IdError.InvalidUlid, true);
        invalid_u_err = true;
    };
    expect_eq(invalid_u_err, true);

    let short_err = false;
    * = Ulid.decode("01ARZ3NDEK") catch |e| {
        expect_eq(e == IdError.InvalidUlid, true);
        short_err = true;
    };
    expect_eq(short_err, true);
}

test("cap:nanoid - MonotonicUlidGenerator 1000 same-ms ordering & 80-bit 0xFF overflow") {
    let rng = IdRng.init(777777);
    let gen = MonotonicUlidGenerator.init();
    const ts: u64 = 1732100000000;

    let prev = gen.next(&rng, ts) catch unreachable;
    let prev_enc = prev.encode();

    for (i of 0..999) {
        * = i;
        const cur = gen.next(&rng, ts) catch unreachable;
        const cur_enc = cur.encode();
        expect_eq(cur.timestampMs(), ts);
        expect_eq(std.mem.order(u8, &prev_enc, &cur_enc) == .lt, true);
        prev_enc = cur_enc;
    }

    // Advancing timestamp generates a strictly larger ULID
    const next_ms = gen.next(&rng, ts + 1) catch unreachable;
    const next_enc = next_ms.encode();
    expect_eq(next_ms.timestampMs(), ts + 1);
    expect_eq(std.mem.order(u8, &prev_enc, &next_enc) == .lt, true);

    // 80-bit all-0xFF overflow returns MonotonicOverflow
    const max_rand80: u128 = ((1 as u128) << 80) - 1;
    let sat_gen = MonotonicUlidGenerator.fromState(ts, max_rand80) catch unreachable;
    let mono_overflow = false;
    * = sat_gen.next(&rng, ts) catch |e| {
        expect_eq(e == IdError.MonotonicOverflow, true);
        mono_overflow = true;
    };
    expect_eq(mono_overflow, true);
}

test("cap:nanoid - UuidV7 RFC 9562 version=7 / variant=2 layout, round-trip & ordering") {
    let rng = IdRng.init(31415926);
    const t1: u64 = 1700000001000;
    const t2: u64 = 1700000002000;

    const u1 = UuidV7.generate(&rng, t1) catch unreachable;
    const u2 = UuidV7.generate(&rng, t2) catch unreachable;

    expect_eq(u1.version(), 7);
    expect_eq(u1.variant(), 2);
    expect_eq(u1.timestampMs(), t1);

    expect_eq(u2.version(), 7);
    expect_eq(u2.variant(), 2);
    expect_eq(u2.timestampMs(), t2);

    const s1 = u1.format();
    const s2 = u2.format();
    expect_eq(s1[8], '-');
    expect_eq(s1[13], '-');
    expect_eq(s1[14], '7');
    expect_eq(s1[18], '-');
    expect_eq(s1[23], '-');
    expect_eq(std.mem.order(u8, &s1, &s2) == .lt, true);

    const p1 = UuidV7.parse(&s1) catch unreachable;
    expect_eq(p1.timestampMs(), t1);
    expect_eq(p1.version(), 7);
    expect_eq(p1.variant(), 2);
    expect_eq(std.mem.eql(u8, &p1.format(), &s1), true);

    // Reject UUID v4 (version nibble = 4)
    let bad_ver = false;
    * = UuidV7.parse("018c2e65-4b1a-4000-8000-000000000000") catch |e| {
        expect_eq(e == IdError.InvalidUuid, true);
        bad_ver = true;
    };
    expect_eq(bad_ver, true);

    // Reject bad variant (variant nibble = 0)
    let bad_var = false;
    * = UuidV7.parse("018c2e65-4b1a-7000-0000-000000000000") catch |e| {
        expect_eq(e == IdError.InvalidUuid, true);
        bad_var = true;
    };
    expect_eq(bad_var, true);
}

test("cap.nanoid - dynamic mounting & std.testing.allocator zero-leak check") {
    const alloc = std.testing.allocator;
    let rng = cap.codec.IdRng.init(55555);

    const nid = cap.codec.NanoId.generate(alloc, &rng, 21) catch unreachable;
    defer alloc.free(nid);
    expect_eq(nid.len, 21);

    let mono = cap.codec.MonotonicUlidGenerator.init();
    const u = mono.next(&rng, 1700000005000) catch unreachable;
    const enc = u.encode();
    const decoded = cap.codec.Ulid.decode(&enc) catch unreachable;
    expect_eq(decoded.timestampMs(), 1700000005000);

    const v7 = cap.codec.UuidV7.generate(&rng, 1700000005000) catch unreachable;
    const fmt = v7.format();
    const pv7 = cap.codec.UuidV7.parse(&fmt) catch unreachable;
    expect_eq(pv7.version(), 7);
    expect_eq(pv7.variant(), 2);
}
