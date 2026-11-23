// Source: cp/cap/test/testUuidRustRefSuite.cp
// Reference-hardened against uuid-rs/uuid (MIT/Apache-2.0) & RFC 9562
// Comprehensive RFC 9562 test vectors, uuid-rs/uuid parser/v5/v7 fixtures,
// 16-lane SIMD vs scalar differential parity, and 1,500-case deterministic fuzz suite.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    Uuid,
    NIL,
    MAX,
    NAMESPACE_DNS,
    NAMESPACE_URL,
    NAMESPACE_OID,
    NAMESPACE_X500,
    VARIANT_NCS,
    VARIANT_RFC4122,
    VARIANT_RFC9562,
    VARIANT_MICROSOFT,
    VARIANT_FUTURE,
    VERSION_NIL,
    VERSION_RANDOM,
    VERSION_SHA1,
    VERSION_SORT_RAND,
    VERSION_MAX,
    simdEql16,
    simdIsHex16,
    simdIsHex32,
    simdAsciiToLower16,
    simdNormalizeHexLower32,
    simdDecodeHex32,
    simdFormatHex32,
} from "cap:uuid";

// --- Scalar Reference Implementations for Differential Parity ---

fn scalarHexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return (c - 'a') + 10;
    if (c >= 'A' && c <= 'F') return (c - 'A') + 10;
    return null;
}

fn scalarIsHex32(s: []const u8): bool {
    if (s.len != 32) return false;
    for (i of 0..32) {
        if (scalarHexNibble(s[i]) == null) return false;
    }
    return true;
}

fn scalarEql16(a: *const [16]u8, b: *const [16]u8): bool {
    for (i of 0..16) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn scalarNormalizeHexLower32(input: []const u8, out: *[32]u8): bool {
    if (input.len != 32) return false;
    for (i of 0..32) {
        let c = input[i];
        if (scalarHexNibble(c) == null) return false;
        if (c >= 'A' && c <= 'Z') c += 32;
        out[i] = c;
    }
    return true;
}

fn scalarFormatHex32(bytes: *const [16]u8, out: *[32]u8, upper: bool) {
    const table: []const u8 = if (upper) "0123456789ABCDEF" else "0123456789abcdef";
    for (i of 0..16) {
        const b = bytes[i];
        out[i * 2] = table[(b >> 4) & 0x0F];
        out[i * 2 + 1] = table[b & 0x0F];
    }
}

fn scalarParseUuid(input: []const u8): Uuid? {
    let body = input;
    if (input.len == 32) {
        let out: [16]u8 = undefined;
        for (i of 0..16) {
            const hi = scalarHexNibble(input[i * 2]) ?? return null;
            const lo = scalarHexNibble(input[i * 2 + 1]) ?? return null;
            out[i] = (hi << 4) | lo;
        }
        return Uuid.fromBytes(out);
    }
    if (input.len == 38) {
        if (input[0] != '{' || input[37] != '}') return null;
        body = input[1..37];
    } else if (input.len == 45) {
        const prefix: []const u8 = "urn:uuid:";
        for (i of 0..9) {
            let c = input[i];
            if (c >= 'A' && c <= 'Z') c += 32;
            if (c != prefix[i]) return null;
        }
        body = input[9..45];
    } else if (input.len != 36) {
        return null;
    }

    if (body[8] != '-' || body[13] != '-' || body[18] != '-' || body[23] != '-') {
        return null;
    }
    let out: [16]u8 = undefined;
    let pos: usize = 0;
    for (i of 0..16) {
        if (pos == 8 || pos == 13 || pos == 18 || pos == 23) {
            pos += 1;
        }
        const hi = scalarHexNibble(body[pos]) ?? return null;
        const lo = scalarHexNibble(body[pos + 1]) ?? return null;
        out[i] = (hi << 4) | lo;
        pos += 2;
    }
    return Uuid.fromBytes(out);
}

// Deterministic xorshift64* PRNG for reproducible fuzzing
struct Rng {
    state: u64;

    fn nextU64(*_): u64 {
        let x = _.state;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        _.state = x;
        return x *% 0x2545F4914F6CDD1D;
    }

    fn nextU8(*_): u8 {
        return ((_.nextU64() >> 32) & 0xFF) as u8;
    }

    fn nextRange(*_, bound: usize): usize {
        return ((_.nextU64() as usize) % bound);
    }
}

// --- Test 1: RFC 9562 Appendix A & uuid-rs/uuid v5/v7/NIL/MAX Vectors ---

fn testRfc9562AndUuidRsVectors(): !void {
    // 1. NIL & MAX constants, version & variant semantics
    expect_eq(NIL.isNil(), true);
    expect_eq(NIL.isMax(), false);
    expect_eq(NIL.eql(Uuid.nil()), true);
    expect_eq(NIL.getVersion().?, VERSION_NIL);
    expect_eq(NIL.getVariant(), VARIANT_NCS);
    expect_eq(NIL.asU128(), 0);

    expect_eq(MAX.isMax(), true);
    expect_eq(MAX.isNil(), false);
    expect_eq(MAX.eql(Uuid.max()), true);
    expect_eq(MAX.getVersion().?, VERSION_MAX);
    expect_eq(MAX.getVariant(), VARIANT_FUTURE);
    expect_eq(MAX.asU128(), std.math.maxInt(u128));
    expect_eq(Uuid.fromU128(MAX.asU128()).eql(MAX), true);

    // 2. RFC 9562 Appendix A.4 Official v5 Vector
    let buf36: [36]u8 = undefined;
    const rfc_v5 = Uuid.v5(NAMESPACE_DNS, "www.example.com");
    expect_eq(rfc_v5.version(), 5);
    expect_eq(rfc_v5.getVersion().?, VERSION_SHA1);
    expect_eq(rfc_v5.getVariant(), VARIANT_RFC9562);
    expect_eq(rfc_v5.variant(), VARIANT_RFC4122);
    expect_eq(rfc_v5.format(&buf36), "2ed6657d-e927-568b-95e1-2665a8aea6a2");

    // 3. Full 16-vector fixture inherited from uuid-rs/uuid/src/v5.rs
    expect_eq(Uuid.v5(NAMESPACE_DNS, "example.org").format(&buf36), "aad03681-8b63-5304-89e0-8ca8f49461b5");
    expect_eq(Uuid.v5(NAMESPACE_DNS, "rust-lang.org").format(&buf36), "c66bbb60-d62e-5f17-a399-3a0bd237c503");
    expect_eq(Uuid.v5(NAMESPACE_DNS, "42").format(&buf36), "7c411b5e-9d3f-50b5-9c28-62096e41c4ed");
    expect_eq(Uuid.v5(NAMESPACE_DNS, "lorem ipsum").format(&buf36), "97886a05-8a68-5743-ad55-56ab2d61cf7b");

    expect_eq(Uuid.v5(NAMESPACE_URL, "example.org").format(&buf36), "54a35416-963c-5dd6-a1e2-5ab7bb5bafc7");
    expect_eq(Uuid.v5(NAMESPACE_URL, "rust-lang.org").format(&buf36), "c48d927f-4122-5413-968c-598b1780e749");
    expect_eq(Uuid.v5(NAMESPACE_URL, "42").format(&buf36), "5c2b23de-4bad-58ee-a4b3-f22f3b9cfd7d");
    expect_eq(Uuid.v5(NAMESPACE_URL, "lorem ipsum").format(&buf36), "15c67689-4b85-5253-86b4-49fbb138569f");

    expect_eq(Uuid.v5(NAMESPACE_OID, "example.org").format(&buf36), "34784df9-b065-5094-92c7-00bb3da97a30");
    expect_eq(Uuid.v5(NAMESPACE_OID, "rust-lang.org").format(&buf36), "8ef61ecb-977a-5844-ab0f-c25ef9b8d5d6");
    expect_eq(Uuid.v5(NAMESPACE_OID, "42").format(&buf36), "ba293c61-ad33-57b9-9671-f3319f57d789");
    expect_eq(Uuid.v5(NAMESPACE_OID, "lorem ipsum").format(&buf36), "6485290d-f79e-5380-9e64-cb4312c7b4a6");

    expect_eq(Uuid.v5(NAMESPACE_X500, "example.org").format(&buf36), "e3635e86-f82b-5bbc-a54a-da97923e5c76");
    expect_eq(Uuid.v5(NAMESPACE_X500, "rust-lang.org").format(&buf36), "26c9c3e9-49b7-56da-8b9f-a0fb916a71a3");
    expect_eq(Uuid.v5(NAMESPACE_X500, "42").format(&buf36), "e4b88014-47c6-5fe0-a195-13710e5f6e27");
    expect_eq(Uuid.v5(NAMESPACE_X500, "lorem ipsum").format(&buf36), "b11f79a5-1e6d-57ce-a4b5-ba8531ea03d0");

    // 4. RFC 9562 Appendix A.6 UUID v7 vector: 017f22e2-79b0-7cc3-98c4-dc0c0c07398f (ts = 1645557742000)
    const v7_rfc = Uuid.parse("017f22e2-79b0-7cc3-98c4-dc0c0c07398f").?;
    expect_eq(v7_rfc.version(), 7);
    expect_eq(v7_rfc.getVersion().?, VERSION_SORT_RAND);
    expect_eq(v7_rfc.getVariant(), VARIANT_RFC9562);
    expect_eq(v7_rfc.extractV7TimestampMs().?, 1645557742000);

    const v7_gen = Uuid.v7At(1645557742000);
    const v7_str = v7_gen.format(&buf36);
    expect_eq(std.mem.startsWith(u8, v7_str, "017f22e2-79b0-7"), true);
    expect_eq(v7_gen.extractV7TimestampMs().?, 1645557742000);

    // Non-RFC9562 variant with version nibble 7 must return null for getVersion() and extractV7TimestampMs()
    let ms_bytes = v7_rfc.asBytes();
    ms_bytes[8] = (ms_bytes[8] & 0x1F) | 0xC0; // Microsoft variant (110x)
    const ms_uuid = Uuid.fromBytes(ms_bytes);
    expect_eq(ms_uuid.getVariant(), VARIANT_MICROSOFT);
    expect_eq(ms_uuid.getVersion() == null, true);
    expect_eq(ms_uuid.extractV7TimestampMs() == null, true);

    // 5. v4 version/variant check & v7 strict monotonic ordering
    const v4_id = Uuid.v4();
    expect_eq(v4_id.getVersion().?, VERSION_RANDOM);
    expect_eq(v4_id.getVariant(), VARIANT_RFC9562);

    let prev = Uuid.v7();
    let prev_s: [36]u8 = undefined;
    * = prev.format(&prev_s);
    for (idx of 0..128) {
        * = idx;
        const cur = Uuid.v7();
        expect_eq(cur.getVersion().?, VERSION_SORT_RAND);
        expect_eq(cur.getVariant(), VARIANT_RFC9562);
        expect_eq(prev.cmp(cur) < 0, true);
        expect_eq(prev.asU128() < cur.asU128(), true);
        let cur_s: [36]u8 = undefined;
        const cs = cur.format(&cur_s);
        expect_eq(std.mem.order(u8, prev_s[0..36], cs) == .lt, true);
        prev = cur;
        @memcpy(prev_s[0..36], cs);
    }
}

// --- Test 2: All 4 Formats Round-Trip & uuid-rs/uuid Invalid Rejection Matrix ---

fn testAllFourFormatsAndStrictRejection(): !void {
    const expected = Uuid.parse("67e55044-10b1-426f-9247-bb680e5fe0c8").?;

    // Valid parse across all 4 formats & case variations
    const p_hyph = Uuid.parseHyphenated("67e55044-10b1-426f-9247-bb680e5fe0c8").?;
    const p_simp = Uuid.parseSimple("67e5504410b1426f9247bb680e5fe0c8").?;
    const p_brac = Uuid.parseBraced("{67e55044-10b1-426f-9247-bb680e5fe0c8}").?;
    const p_urn1 = Uuid.parseUrn("urn:uuid:67e55044-10b1-426f-9247-bb680e5fe0c8").?;
    const p_urn2 = Uuid.parseUrn("URN:UUID:67E55044-10B1-426F-9247-BB680E5FE0C8").?;
    const p_urn3 = Uuid.parse("Urn:Uuid:67e55044-10B1-426f-9247-bb680e5fe0c8").?;

    expect_eq(p_hyph.eql(expected), true);
    expect_eq(p_simp.eql(expected), true);
    expect_eq(p_brac.eql(expected), true);
    expect_eq(p_urn1.eql(expected), true);
    expect_eq(p_urn2.eql(expected), true);
    expect_eq(p_urn3.eql(expected), true);

    // Format round-trip for all 4 formats (lower + upper)
    let b32: [32]u8 = undefined;
    let b36: [36]u8 = undefined;
    let b38: [38]u8 = undefined;
    let b45: [45]u8 = undefined;

    expect_eq(expected.formatSimple(&b32), "67e5504410b1426f9247bb680e5fe0c8");
    expect_eq(expected.formatSimpleUpper(&b32), "67E5504410B1426F9247BB680E5FE0C8");
    expect_eq(expected.format(&b36), "67e55044-10b1-426f-9247-bb680e5fe0c8");
    expect_eq(expected.formatUpper(&b36), "67E55044-10B1-426F-9247-BB680E5FE0C8");
    expect_eq(expected.formatBraced(&b38), "{67e55044-10b1-426f-9247-bb680e5fe0c8}");
    expect_eq(expected.formatBracedUpper(&b38), "{67E55044-10B1-426F-9247-BB680E5FE0C8}");
    expect_eq(expected.formatUrn(&b45), "urn:uuid:67e55044-10b1-426f-9247-bb680e5fe0c8");
    expect_eq(expected.formatUrnUpper(&b45), "urn:uuid:67E55044-10B1-426F-9247-BB680E5FE0C8");

    // Cross-format strict parser rejection (matching uuid-rs/uuid/src/parser.rs)
    expect_eq(Uuid.parseSimple("67e55044-10b1-426f-9247-bb680e5fe0c8") == null, true);
    expect_eq(Uuid.parseHyphenated("67e5504410b1426f9247bb680e5fe0c8") == null, true);
    expect_eq(Uuid.parseBraced("67e55044-10b1-426f-9247-bb680e5fe0c8") == null, true);
    expect_eq(Uuid.parseBraced("{{67e55044-10b1-426f-9247-bb680e5fe0c8}}") == null, true);
    expect_eq(Uuid.parseUrn("67e55044-10b1-426f-9247-bb680e5fe0c8") == null, true);
    expect_eq(Uuid.parseUrn(":67e55044-10b1-426f-9247-bb680e5fe0c8") == null, true);

    // Malformed matrix directly inherited from uuid-rs/uuid/src/parser.rs::test_parse_invalid
    const invalid_cases: []const []const u8 = [
        "",
        "{}",
        "!",
        "F9168C5E-CEB2-4faa-B6BF-329BF39FA1E45",
        "F9168C5E-CEB2-4faa-BBF-329BF39FA1E4",
        "F9168C5E-CEB2-4faa-BGBF-329BF39FA1E4",
        "F9168C5E-CEB2F4faaFB6BFF329BF39FA1E4",
        "F9168C5E-CEB2-4faaFB6BFF329BF39FA1E4",
        "F9168C5E-CEB2-4faa-B6BFF329BF39FA1E4",
        "F9168C5E-CEB2-4faa",
        "F9168C5E-CEB2-4faaXB6BFF329BF39FA1E4",
        "{F9168C5E-CEB2-4faa9B6BFF329BF39FA1E41",
        "{F9168C5E-CEB2-4faa9B6BFF329BF39FA1E41}",
        "F9168C5E-CEB-24fa-eB6BFF32-BF39FA1E4",
        "01020304-1112-2122-3132-41424344",
        "67e5504410b1426f9247bb680e5fe0c",
        "67e5504410b1426f9247bb680e5fe0c88",
        "67e5504410b1426f9247bb680e5fe0cg",
        "67e5504410b1426%9247bb680e5fe0c8",
        "231231212212423424324323477343246663",
        // Critical RFC 9562 / uuid-rs strictness: braced & URN MUST wrap hyphenated, NEVER simple 32B
        "{00000000000000000000000000000000}",
        "urn:uuid:00000000000000000000000000000000",
        "67e550X410b1426f9247bb680e5fe0cd",
        "67e550-4105b1426f9247bb680e5fe0c",
        "F9168C5E-CEB2-4faa-B6BF1-02BF39FA1E4",
        "urn:uuid:urae0c8",
        "67e55044-10b1-426f-9247-bb680e5\x00e0c8",
    ];

    for (bad of invalid_cases) {
        expect_eq(Uuid.parse(bad) == null, true);
    }
}

// --- Test 3: 16-lane SIMD vs Scalar Differential Parity (0 Discrepancy) ---

fn testSimdVsScalarParity(): !void {
    // 1. Exhaustive 0..255 byte sweep for simdIsHex16 / simdIsHex32 / simdAsciiToLower16 / simdNormalizeHexLower32
    let buf32: [32]u8 = ['0'; 32];
    for (byte_val of 0..256) {
        const b = byte_val as u8;
        const idx_u = byte_val as usize;
        const pos = idx_u % 32;
        buf32[pos] = b;

        const simd_hex = simdIsHex32(&buf32);
        const scalar_hex = scalarIsHex32(&buf32);
        expect_eq(simd_hex, scalar_hex);

        let simd_norm: [32]u8 = [0; 32];
        let scalar_norm: [32]u8 = [0; 32];
        const simd_ok = simdNormalizeHexLower32(&buf32, &simd_norm);
        const scalar_ok = scalarNormalizeHexLower32(&buf32, &scalar_norm);
        expect_eq(simd_ok, scalar_ok);
        if (simd_ok) {
            expect_eq(std.mem.eql(u8, &simd_norm, &scalar_norm), true);
        }

        // Check simdAsciiToLower16 on raw chunk
        let chunk16: [16]u8 = ['a'; 16];
        const pos16 = idx_u % 16;
        chunk16[pos16] = b;
        const lower16 = simdAsciiToLower16(&chunk16);
        const expected_ch: u8 = if (b >= 'A' && b <= 'Z') (b + 32) else b;
        expect_eq(lower16[pos16], expected_ch);

        buf32[pos] = '0';
    }

    // 2. 256 deterministic random UUIDs: simdFormatHex32 (lower+upper), simdDecodeHex32, simdEql16
    let rng = Rng { state: 0x1E37_79B9_7F4A_7C15 };
    for (iter of 0..256) {
        let raw_a: [16]u8 = undefined;
        let raw_b: [16]u8 = undefined;
        for (i of 0..16) {
            raw_a[i] = rng.nextU8();
            raw_b[i] = raw_a[i];
        }
        // Equal pair
        expect_eq(simdEql16(&raw_a, &raw_b), scalarEql16(&raw_a, &raw_b));
        expect_eq(simdEql16(&raw_a, &raw_b), true);

        // Single-byte flip at each lane 0..15
        const flip_idx = (iter as usize) % 16;
        raw_b[flip_idx] ^= 0x01;
        expect_eq(simdEql16(&raw_a, &raw_b), scalarEql16(&raw_a, &raw_b));
        expect_eq(simdEql16(&raw_a, &raw_b), false);

        // Format lower + upper parity
        let simd_lo: [32]u8 = undefined;
        let scal_lo: [32]u8 = undefined;
        simdFormatHex32(&raw_a, &simd_lo, false);
        scalarFormatHex32(&raw_a, &scal_lo, false);
        expect_eq(std.mem.eql(u8, &simd_lo, &scal_lo), true);

        let simd_hi: [32]u8 = undefined;
        let scal_hi: [32]u8 = undefined;
        simdFormatHex32(&raw_a, &simd_hi, true);
        scalarFormatHex32(&raw_a, &scal_hi, true);
        expect_eq(std.mem.eql(u8, &simd_hi, &scal_hi), true);

        // Decode round-trip parity
        let dec_lo: [16]u8 = undefined;
        let dec_hi: [16]u8 = undefined;
        expect_eq(simdDecodeHex32(&simd_lo, &dec_lo), true);
        expect_eq(simdDecodeHex32(&simd_hi, &dec_hi), true);
        expect_eq(simdEql16(&dec_lo, &raw_a), true);
        expect_eq(simdEql16(&dec_hi, &raw_a), true);

        // Normalize upper -> lower matches simd_lo
        let norm_out: [32]u8 = undefined;
        expect_eq(simdNormalizeHexLower32(&simd_hi, &norm_out), true);
        expect_eq(std.mem.eql(u8, &norm_out, &simd_lo), true);
    }

    // 3. Direct simdIsHex16 sanity
    let valid16: [16]u8 = undefined;
    let valid16_up: [16]u8 = undefined;
    let bad16: [16]u8 = undefined;
    @memcpy(&valid16, "0123456789abcdef");
    @memcpy(&valid16_up, "0123456789ABCDEF");
    @memcpy(&bad16, "0123456789abcdeg");
    expect_eq(simdIsHex16(&valid16), true);
    expect_eq(simdIsHex16(&valid16_up), true);
    expect_eq(simdIsHex16(&bad16), false);
}

// --- Test 4: 1,500-Case Deterministic PRNG Malformed UUID Fuzz Suite ---

fn testDeterministicMalformedFuzz(): !void {
    let rng = Rng { state: 0x51B5_4A32_D192_ED03 };
    let buf: [64]u8 = undefined;
    const base_hyph: []const u8 = "67e55044-10b1-426f-9247-bb680e5fe0c8";
    const base_simp: []const u8 = "67e5504410b1426f9247bb680e5fe0c8";
    const base_brac: []const u8 = "{67e55044-10b1-426f-9247-bb680e5fe0c8}";
    const base_urn: []const u8 = "urn:uuid:67e55044-10b1-426f-9247-bb680e5fe0c8";

    let total_cases: usize = 0;
    let valid_hits: usize = 0;
    let rejected_hits: usize = 0;

    // 1,500 deterministic fuzz inputs across 5 mutation strategies
    for (case_idx of 0..1500) {
        let len: usize = 0;
        const strategy = (case_idx as usize) % 5;

        if (strategy == 0) {
            // Pure random length [0..64] and arbitrary bytes [0..255]
            len = rng.nextRange(65);
            for (i of 0..len) {
                buf[i] = rng.nextU8();
            }
        } else if (strategy == 1) {
            // Mutate valid 32B Simple UUID (1..3 byte corruptions or truncation)
            len = 32;
            @memcpy(buf[0..32], base_simp);
            const muts = 1 + rng.nextRange(3);
            for (m of 0..muts) {
                * = m;
                const pos = rng.nextRange(32);
                buf[pos] = rng.nextU8();
            }
        } else if (strategy == 2) {
            // Mutate valid 36B Hyphenated UUID (hyphen removal/relocation/char corruption)
            len = 36;
            @memcpy(buf[0..36], base_hyph);
            const muts = 1 + rng.nextRange(3);
            for (m of 0..muts) {
                * = m;
                const pos = rng.nextRange(36);
                buf[pos] = rng.nextU8();
            }
        } else if (strategy == 3) {
            // Mutate valid 38B Braced UUID
            len = 38;
            @memcpy(buf[0..38], base_brac);
            const muts = 1 + rng.nextRange(3);
            for (m of 0..muts) {
                * = m;
                const pos = rng.nextRange(38);
                buf[pos] = rng.nextU8();
            }
        } else {
            // Mutate valid 45B URN UUID (prefix & payload mutations)
            len = 45;
            @memcpy(buf[0..45], base_urn);
            const muts = 1 + rng.nextRange(3);
            for (m of 0..muts) {
                * = m;
                const pos = rng.nextRange(45);
                buf[pos] = rng.nextU8();
            }
        }

        const slice = buf[0..len];
        const got = Uuid.parse(slice);
        const expected = scalarParseUuid(slice);

        // Differential oracle: production SIMD parser must match scalar oracle on every single input
        if (expected == null) {
            expect_eq(got == null, true);
            rejected_hits += 1;
        } else {
            expect_eq(got != null, true);
            const exp_uuid = expected.?;
            expect_eq(simdEql16(&got.?.bytes, &exp_uuid.bytes), true);
            valid_hits += 1;
        }

        // Also exercise strict format parsers on every fuzz input (must never panic / OOB)
        * = Uuid.parseSimple(slice);
        * = Uuid.parseHyphenated(slice);
        * = Uuid.parseBraced(slice);
        * = Uuid.parseUrn(slice);
        * = simdIsHex32(slice);

        total_cases += 1;
    }

    expect_eq(total_cases, 1500);
    expect_eq(rejected_hits > 1300, true);
    expect_eq(valid_hits + rejected_hits, 1500);
}

fn main() {
    test("1. RFC 9562 Appendix A & uuid-rs/uuid v5/v7/NIL/MAX Vectors", () => {
        testRfc9562AndUuidRsVectors()?;
    });
    test("2. All 4 Formats Round-Trip & Strict Malformed Rejection Matrix", () => {
        testAllFourFormatsAndStrictRejection()?;
    });
    test("3. 16-Lane SIMD vs Scalar Differential Parity (0 Discrepancy)", () => {
        testSimdVsScalarParity()?;
    });
    test("4. 1,500-Case Deterministic PRNG Malformed UUID Fuzz Suite", () => {
        testDeterministicMalformedFuzz()?;
    });
}
