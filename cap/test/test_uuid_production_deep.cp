// Source: cp/cap/test/test_uuid_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:uuid & cap.uuid

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    Uuid,
    Ulid,
    NanoId,
    NAMESPACE_DNS,
    NAMESPACE_URL,
    NAMESPACE_OID,
    NAMESPACE_X500,
    VARIANT_RFC9562,
    NANOID_DEFAULT_SIZE,
    NANOID_URL_ALPHABET,
} from "cap:uuid";

fn testUuidV4ParseFormatNilMax(): !void {
    // 1. Nil & Max UUID invariants
    const u_nil = Uuid.nil();
    const u_max = Uuid.max();
    expect_eq(u_nil.isNil(), true);
    expect_eq(u_nil.isMax(), false);
    expect_eq(u_max.isNil(), false);
    expect_eq(u_max.isMax(), true);
    expect_eq(u_nil.cmp(u_max) < 0, true);
    expect_eq(u_max.cmp(u_nil) > 0, true);
    expect_eq(u_nil.eql(Uuid.nil()), true);

    let buf36: [36]u8 = undefined;
    expect_eq(u_nil.format(&buf36), "00000000-0000-0000-0000-000000000000");
    expect_eq(u_max.format(&buf36), "ffffffff-ffff-ffff-ffff-ffffffffffff");
    expect_eq(u_max.formatUpper(&buf36), "FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF");

    // 2. UUID v4 generation, version/variant bits & uniqueness
    const v4a = Uuid.v4();
    const v4b = Uuid.v4();
    expect_eq(v4a.version(), 4);
    expect_eq(v4a.variant(), VARIANT_RFC9562);
    expect_eq(v4b.version(), 4);
    expect_eq(v4b.variant(), VARIANT_RFC9562);
    expect_eq(v4a.eql(v4b), false);

    // 3. Round-trip across all 4 accepted parse formats
    const s_lower = v4a.format(&buf36);
    let buf_upper: [36]u8 = undefined;
    const s_upper = v4a.formatUpper(&buf_upper);
    let buf_compact: [32]u8 = undefined;
    const s_compact = v4a.formatCompact(&buf_compact);
    let buf_urn: [45]u8 = undefined;
    const s_urn = v4a.formatUrn(&buf_urn);

    expect_eq(Uuid.parse(s_lower).?.eql(v4a), true);
    expect_eq(Uuid.parse(s_upper).?.eql(v4a), true);
    expect_eq(Uuid.parse(s_compact).?.eql(v4a), true);
    expect_eq(Uuid.parse(s_urn).?.eql(v4a), true);

    const braced = `{${s_upper}}`;
    expect_eq(Uuid.parse(braced).?.eql(v4a), true);
    expect_eq(Uuid.parse("URN:UUID:6BA7B810-9DAD-11D1-80B4-00C04FD430C8").?.eql(NAMESPACE_DNS), true);

    // 4. Reject malformed inputs
    expect_eq(Uuid.parse("") == null, true);
    expect_eq(Uuid.parse("6ba7b810-9dad-11d1-80b4-00c04fd430c") == null, true);
    expect_eq(Uuid.parse("6ba7b810-9dad-11d1-80b4-00c04fd430cg") == null, true);
    expect_eq(Uuid.parse("6ba7b8109dad-11d1-80b4-00c04fd430c8") == null, true);
    expect_eq(Uuid.parse("{6ba7b810-9dad-11d1-80b4-00c04fd430c8") == null, true);

    // 5. Allocator-backed string formatting
    const alloc_s = v4a.toStringAlloc(std.heap.page_allocator)?;
    defer std.heap.page_allocator.free(alloc_s);
    expect_eq(alloc_s, s_lower);
}

fn testUuidV5Rfc9562Vectors(): !void {
    // Official RFC 9562 Appendix A.4 / RFC 4122 SHA-1 vector:
    // NAMESPACE_DNS + "www.example.com" -> 2ed6657d-e927-568b-95e1-2665a8aea6a2
    const u_dns = Uuid.v5(NAMESPACE_DNS, "www.example.com");
    let buf: [36]u8 = undefined;
    expect_eq(u_dns.version(), 5);
    expect_eq(u_dns.variant(), VARIANT_RFC9562);
    expect_eq(u_dns.format(&buf), "2ed6657d-e927-568b-95e1-2665a8aea6a2");

    // Determinism & namespace isolation
    const u_dns_2 = Uuid.v5(NAMESPACE_DNS, "www.example.com");
    expect_eq(u_dns.eql(u_dns_2), true);

    const u_url = Uuid.v5(NAMESPACE_URL, "www.example.com");
    const u_oid = Uuid.v5(NAMESPACE_OID, "www.example.com");
    const u_x500 = Uuid.v5(NAMESPACE_X500, "www.example.com");
    expect_eq(u_url.version(), 5);
    expect_eq(u_oid.version(), 5);
    expect_eq(u_x500.version(), 5);
    expect_eq(u_dns.eql(u_url), false);
    expect_eq(u_dns.eql(u_oid), false);
    expect_eq(u_dns.eql(u_x500), false);
    expect_eq(u_url.eql(u_oid), false);
}

fn testUuidV7TimeOrdering(): !void {
    // 1. Explicit timestamp embedding & extraction
    const t1: u64 = 1700000000123;
    const t2: u64 = 1700000000124;
    const uuid_1 = Uuid.v7At(t1);
    const uuid_2 = Uuid.v7At(t2);

    expect_eq(uuid_1.version(), 7);
    expect_eq(uuid_1.variant(), VARIANT_RFC9562);
    expect_eq(uuid_2.version(), 7);
    expect_eq(uuid_2.variant(), VARIANT_RFC9562);
    expect_eq(uuid_1.extractV7TimestampMs().?, t1);
    expect_eq(uuid_2.extractV7TimestampMs().?, t2);

    // Non-v7 UUIDs must return null for extractV7TimestampMs
    expect_eq(Uuid.v4().extractV7TimestampMs() == null, true);
    expect_eq(NAMESPACE_DNS.extractV7TimestampMs() == null, true);

    // Timestamp ordering guarantees both binary cmp and canonical string lexicographic order
    expect_eq(uuid_1.cmp(uuid_2) < 0, true);
    let b1: [36]u8 = undefined;
    let b2: [36]u8 = undefined;
    const s1 = uuid_1.format(&b1);
    const s2 = uuid_2.format(&b2);
    expect_eq(std.mem.order(u8, s1, s2) == .lt, true);

    // 2. Rapid consecutive Uuid.v7() calls must remain strictly monotonically increasing
    let prev = Uuid.v7();
    let prev_buf: [36]u8 = undefined;
    * = prev.format(&prev_buf);

    for (iter_idx of 0..256) {
        * = iter_idx;
        const cur = Uuid.v7();
        expect_eq(cur.version(), 7);
        expect_eq(cur.variant(), VARIANT_RFC9562);
        expect_eq(cur.extractV7TimestampMs() != null, true);
        expect_eq(prev.cmp(cur) < 0, true);

        let cur_buf: [36]u8 = undefined;
        const cur_s = cur.format(&cur_buf);
        expect_eq(std.mem.order(u8, prev_buf[0..36], cur_s) == .lt, true);

        prev = cur;
        @memcpy(prev_buf[0..36], cur_s);
    }
}

fn testUlidCrockfordAndUuidRoundTrip(): !void {
    // 1. Nil & Max canonical Crockford strings
    let buf: [26]u8 = undefined;
    expect_eq(Ulid.nil().format(&buf), "00000000000000000000000000");
    expect_eq(Ulid.max().format(&buf), "7ZZZZZZZZZZZZZZZZZZZZZZZZZ");
    expect_eq(Ulid.parse("7zzzzzzzzzzzzzzzzzzzzzzzzz").?.eql(Ulid.max()), true);

    // 2. Ambiguous character normalization (I/L -> 1, O -> 0, case-insensitive)
    const canonical = Ulid.parse("01ARZ3NDEKTSV4RRFFQ69G5FAV").?;
    const ambiguous = Ulid.parse("oiARZ3NDEKTSV4RRFFQ69G5FAV").?;
    const ambiguous_lower = Ulid.parse("0lArz3ndektsv4rrffq69g5fav").?;
    expect_eq(canonical.eql(ambiguous), true);
    expect_eq(canonical.eql(ambiguous_lower), true);
    expect_eq(canonical.format(&buf), "01ARZ3NDEKTSV4RRFFQ69G5FAV");
    expect_eq(canonical.timestampMs(), 1469922850259);

    // 3. Reject invalid length, 'U' character, and 128-bit overflow prefix ('8'..'Z')
    expect_eq(Ulid.parse("01ARZ3NDEKTSV4RRFFQ69G5FA") == null, true);
    expect_eq(Ulid.parse("01ARZ3NDEKTSV4RRFFQ69G5FAU") == null, true);
    expect_eq(Ulid.parse("80000000000000000000000000") == null, true);

    // 4. Timestamped & monotonic generation
    const u_early = Ulid.generateAt(1700000001000);
    const u_late = Ulid.generateAt(1700000002000);
    expect_eq(u_early.timestampMs(), 1700000001000);
    expect_eq(u_late.timestampMs(), 1700000002000);
    expect_eq(u_early.cmp(u_late) < 0, true);

    let prev = Ulid.generate();
    let prev_str: [26]u8 = undefined;
    * = prev.format(&prev_str);
    for (iter_idx of 0..128) {
        * = iter_idx;
        const next = Ulid.generate();
        expect_eq(prev.cmp(next) < 0, true);
        let next_str: [26]u8 = undefined;
        const ns = next.format(&next_str);
        expect_eq(std.mem.order(u8, prev_str[0..26], ns) == .lt, true);
        prev = next;
        @memcpy(prev_str[0..26], ns);
    }

    // 5. Lossless bidirectional 128-bit ULID <-> UUID conversion
    const as_uuid = canonical.toUuid();
    const back_to_ulid = Ulid.fromUuid(as_uuid);
    expect_eq(back_to_ulid.eql(canonical), true);
    expect_eq(std.mem.eql(u8, &as_uuid.bytes, &canonical.bytes), true);
}

fn testNanoIdUnbiasedAndCapLazyMount(): !void {
    // 1. Default 21-char NanoID
    expect_eq(NANOID_DEFAULT_SIZE, 21);
    expect_eq(NANOID_URL_ALPHABET.len, 64);

    let nbuf: [21]u8 = undefined;
    const id1 = NanoId.generate(&nbuf);
    expect_eq(id1.len, 21);
    for (ch of id1) {
        expect_eq(std.mem.indexOfScalar(u8, NANOID_URL_ALPHABET, ch) != null, true);
    }

    const alloc_id = NanoId.generateAlloc(std.heap.page_allocator, 32)?;
    defer std.heap.page_allocator.free(alloc_id);
    expect_eq(alloc_id.len, 32);

    // 2. Unbiased rejection-sampling check on non-power-of-two alphabet ("0123456789", len=10, mask=15)
    // Generate 10,000 digits across batches and verify each digit appears within reasonable statistical bounds
    // (expected 1,000 per digit; stddev ~ 30; checking [800, 1200] verifies zero 60% modulo-bias skew).
    let counts: [10]usize = [0; 10];
    let sample_buf: [200]u8 = undefined;
    for (batch_idx of 0..50) {
        * = batch_idx;
        const s = NanoId.generateInto(&sample_buf, "0123456789");
        expect_eq(s.len, 200);
        for (ch of s) {
            expect_eq(ch >= '0' && ch <= '9', true);
            counts[(ch - '0') as usize] += 1;
        }
    }
    for (d of 0..10) {
        expect_eq(counts[d] >= 800 && counts[d] <= 1200, true);
    }

    // 3. Custom alloc API & invalid alphabet guard
    const custom_id = NanoId.generateCustomAlloc(std.heap.page_allocator, 16, "ABCDEF")?;
    defer std.heap.page_allocator.free(custom_id);
    expect_eq(custom_id.len, 16);
    for (ch of custom_id) {
        expect_eq(std.mem.indexOfScalar(u8, "ABCDEF", ch) != null, true);
    }
    expect_eq(NanoId.generateInto(&sample_buf, "").len, 0);

    // 4. `import cap from "cap"; cap.uuid` automatic dynamic lazy mounting verification
    const hub_v4 = cap.uuid.Uuid.v4();
    expect_eq(hub_v4.version(), 4);
    expect_eq(hub_v4.variant(), cap.uuid.VARIANT_RFC9562);

    const hub_v5 = cap.uuid.Uuid.v5(cap.uuid.NAMESPACE_DNS, "www.example.com");
    let hub_buf: [36]u8 = undefined;
    expect_eq(hub_v5.format(&hub_buf), "2ed6657d-e927-568b-95e1-2665a8aea6a2");

    const hub_ulid = cap.uuid.Ulid.generate();
    const hub_uuid_rt = cap.uuid.Ulid.fromUuid(hub_ulid.toUuid());
    expect_eq(hub_uuid_rt.eql(hub_ulid), true);

    let hub_nano: [21]u8 = undefined;
    expect_eq(cap.uuid.NanoId.generate(&hub_nano).len, 21);
}

fn main() {
    test("1. UUID v4, Parsing (36/32/URN/Braces), Formatting, Nil & Max", () => {
        testUuidV4ParseFormatNilMax()?;
    });
    test("2. UUID v5 Official RFC 9562 DNS Vector & Built-in Namespaces", () => {
        testUuidV5Rfc9562Vectors()?;
    });
    test("3. UUID v7 Time-Ordered Monotonic Generation & Timestamp Extraction", () => {
        testUuidV7TimeOrdering()?;
    });
    test("4. ULID Crockford Base32 Codec, Ambiguous Chars & UUID Lossless Round-Trip", () => {
        testUlidCrockfordAndUuidRoundTrip()?;
    });
    test("5. NanoID Unbiased Distribution & cap.uuid Automatic Lazy Mounting", () => {
        testNanoIdUnbiasedAndCapLazyMount()?;
    });
}
