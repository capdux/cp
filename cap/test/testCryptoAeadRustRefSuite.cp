// Source: cp/cap/test/testCryptoAeadRustRefSuite.cp
// Reference-hardened against RustCrypto/AEADs (MIT/Apache-2.0), RFC 8439 & NIST SP 800-38D
// Covers:
// 1. RFC 8439 Section 2.8.2 ChaCha20-Poly1305 official golden vector (114B PT + 12B AAD)
// 2. NIST SP 800-38D CAVP AES-256-GCM official vectors (empty PT/AAD & 16B PT)
// 3. Single-bit tamper rejection (CT/AAD/Tag/Nonce/Key) + fail-closed zeroization verification
// 4. 16-lane @Vector(16, u8) SIMD tag comparison & block XOR vs scalar 0-discrepancy parity
// 5. 1,500-case deterministic PRNG variable-length PT/AAD roundtrip & tamper fuzz suite

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    AeadError,
    aeadSecureZero,
    aeadTagEqlSimd,
    aeadXorSimd,
    Aes256Gcm,
    ChaCha20Poly1305,
} from "../../lib/crypto/cipher.cp";

// --- Scalar Reference Implementations for Differential Parity ---

fn scalarTagEql(a: *const [16]u8, b: *const [16]u8): bool {
    let diff: u8 = 0;
    for (i of 0..16) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

fn scalarXor(out: []u8, a: []const u8, b: []const u8): bool {
    if (out.len != a.len || out.len != b.len) return false;
    for (i of 0..out.len) {
        out[i] = a[i] ^ b[i];
    }
    return true;
}

fn isAllZero(buf: []const u8): bool {
    for (i of 0..buf.len) {
        if (buf[i] != 0) return false;
    }
    return true;
}

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

// --- Test 1: RFC 8439 Section 2.8.2 ChaCha20-Poly1305 Golden Vector ---

fn testRfc8439ChaCha20Poly1305Vector(): !void {
    const key: [32]u8 = [
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    ];
    const nonce: [12]u8 = [
        0x07, 0x00, 0x00, 0x00, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    ];
    const aad: [12]u8 = [
        0x50, 0x51, 0x52, 0x53, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    ];
    const pt: []const u8 = "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";
    expect_eq(pt.len, 114 as usize);

    const expected_ct: [114]u8 = [
        0xd3, 0x1a, 0x8d, 0x34, 0x64, 0x8e, 0x60, 0xdb, 0x7b, 0x86, 0xaf, 0xbc, 0x53, 0xef, 0x7e,
        0xc2, 0xa4, 0xad, 0xed, 0x51, 0x29, 0x6e, 0x08, 0xfe, 0xa9, 0xe2, 0xb5, 0xa7, 0x36, 0xee,
        0x62, 0xd6, 0x3d, 0xbe, 0xa4, 0x5e, 0x8c, 0xa9, 0x67, 0x12, 0x82, 0xfa, 0xfb, 0x69, 0xda,
        0x92, 0x72, 0x8b, 0x1a, 0x71, 0xde, 0x0a, 0x9e, 0x06, 0x0b, 0x29, 0x05, 0xd6, 0xa5, 0xb6,
        0x7e, 0xcd, 0x3b, 0x36, 0x92, 0xdd, 0xbd, 0x7f, 0x2d, 0x77, 0x8b, 0x8c, 0x98, 0x03, 0xae,
        0xe3, 0x28, 0x09, 0x1b, 0x58, 0xfa, 0xb3, 0x24, 0xe4, 0xfa, 0xd6, 0x75, 0x94, 0x55, 0x85,
        0x80, 0x8b, 0x48, 0x31, 0xd7, 0xbc, 0x3f, 0xf4, 0xde, 0xf0, 0x8e, 0x4b, 0x7a, 0x9d, 0xe5,
        0x76, 0xd2, 0x65, 0x86, 0xce, 0xc6, 0x4b, 0x61, 0x16,
    ];
    const expected_tag: [16]u8 = [
        0x1a, 0xe1, 0x0b, 0x59, 0x4f, 0x09, 0xe2, 0x6a, 0x7e, 0x90, 0x2e, 0xcb, 0xd0, 0x60, 0x06, 0x91,
    ];

    // 1. Detached encrypt/decrypt exact match
    let ct: [114]u8 = undefined;
    let tag: [16]u8 = undefined;
    try ChaCha20Poly1305.encryptDetached(&key, &nonce, pt, &aad, &ct, &tag);
    expect_eq(std.mem.eql(u8, &ct, &expected_ct), true);
    expect_eq(aeadTagEqlSimd(&tag, &expected_tag), true);

    let dec_out: [114]u8 = undefined;
    try ChaCha20Poly1305.decryptDetached(&key, &nonce, &ct, &tag, &aad, &dec_out);
    expect_eq(std.mem.eql(u8, &dec_out, pt), true);

    // 2. In-place encrypt/decrypt roundtrip
    let inplace_buf: [114]u8 = undefined;
    @memcpy(&inplace_buf, pt);
    let inplace_tag: [16]u8 = undefined;
    try ChaCha20Poly1305.encryptInPlace(&key, &nonce, &aad, &inplace_buf, &inplace_tag);
    expect_eq(std.mem.eql(u8, &inplace_buf, &expected_ct), true);
    expect_eq(aeadTagEqlSimd(&inplace_tag, &expected_tag), true);
    try ChaCha20Poly1305.decryptInPlace(&key, &nonce, &aad, &inplace_buf, &inplace_tag);
    expect_eq(std.mem.eql(u8, &inplace_buf, pt), true);

    // 3. Combined seal/open roundtrip
    let combined_buf: [130]u8 = undefined;
    const sealed = try ChaCha20Poly1305.sealCombined(&key, &nonce, pt, &aad, &combined_buf);
    expect_eq(sealed.len, 130 as usize);
    let opened_buf: [114]u8 = undefined;
    const opened = try ChaCha20Poly1305.openCombined(&key, &nonce, sealed, &aad, &opened_buf);
    expect_eq(std.mem.eql(u8, opened, pt), true);
}

// --- Test 2: NIST SP 800-38D CAVP AES-256-GCM Official Vectors ---

fn testNistAes256GcmVectors(): !void {
    // Vector 1: Empty PT & Empty AAD (NIST CAVP gcmEncryptExtIV256.rsp #0)
    const k1: [32]u8 = [
        0xb5, 0x2c, 0x50, 0x5a, 0x37, 0xd7, 0x8e, 0xda, 0x5d, 0xd3, 0x4f, 0x20, 0xc2, 0x25, 0x40, 0xea,
        0x1b, 0x58, 0x96, 0x3c, 0xf8, 0xe5, 0xbf, 0x8f, 0xfa, 0x85, 0xf9, 0xf2, 0x49, 0x25, 0x05, 0xb4,
    ];
    const n1: [12]u8 = [0x51, 0x6c, 0x33, 0x92, 0x9d, 0xf5, 0xa3, 0x28, 0x4f, 0xf4, 0x63, 0xd7];
    const exp_tag1: [16]u8 = [
        0xbd, 0xc1, 0xac, 0x88, 0x4d, 0x33, 0x24, 0x57, 0xa1, 0xd2, 0x66, 0x4f, 0x16, 0x8c, 0x76, 0xf0,
    ];
    let scratch_empty: [1]u8 = [0];
    const empty_buf: []u8 = scratch_empty[0..0];
    let tag1: [16]u8 = undefined;
    try Aes256Gcm.encryptDetached(&k1, &n1, empty_buf, empty_buf, empty_buf, &tag1);
    expect_eq(aeadTagEqlSimd(&tag1, &exp_tag1), true);
    try Aes256Gcm.decryptDetached(&k1, &n1, empty_buf, &tag1, empty_buf, empty_buf);

    // Vector 2: 16B PT (NIST CAVP gcmEncryptExtIV256.rsp)
    const k2: [32]u8 = [
        0x46, 0x0f, 0xc8, 0x64, 0x97, 0x22, 0x61, 0xc2, 0x56, 0x0e, 0x1e, 0xb8, 0x87, 0x61, 0xff, 0x1c,
        0x99, 0x2b, 0x98, 0x24, 0x97, 0xbd, 0x2a, 0xc3, 0x6c, 0x04, 0x07, 0x1c, 0xbb, 0x8e, 0x5d, 0x99,
    ];
    const n2: [12]u8 = [0x8a, 0x4a, 0x16, 0xb9, 0xe2, 0x10, 0xeb, 0x68, 0xbc, 0xb6, 0xf5, 0x8d];
    const pt2: [16]u8 = [
        0x99, 0xe4, 0xe9, 0x26, 0xff, 0xe9, 0x27, 0xf6, 0x91, 0x89, 0x3f, 0xb7, 0x9a, 0x96, 0xb0, 0x67,
    ];
    const exp_ct2: [16]u8 = [
        0x13, 0x3f, 0xc1, 0x57, 0x51, 0x62, 0x1b, 0x5f, 0x32, 0x5c, 0x7f, 0xf7, 0x1c, 0xe0, 0x83, 0x24,
    ];
    const exp_tag2: [16]u8 = [
        0xec, 0x4e, 0x87, 0xe0, 0xcf, 0x74, 0xa1, 0x36, 0x18, 0xd0, 0xb6, 0x86, 0x36, 0xba, 0x9f, 0xa7,
    ];

    let ct2: [16]u8 = undefined;
    let tag2: [16]u8 = undefined;
    try Aes256Gcm.encryptDetached(&k2, &n2, &pt2, empty_buf, &ct2, &tag2);
    expect_eq(std.mem.eql(u8, &ct2, &exp_ct2), true);
    expect_eq(aeadTagEqlSimd(&tag2, &exp_tag2), true);

    let out2: [16]u8 = undefined;
    try Aes256Gcm.decryptDetached(&k2, &n2, &ct2, &tag2, empty_buf, &out2);
    expect_eq(std.mem.eql(u8, &out2, &pt2), true);
}

// --- Test 3: Single-Bit Tamper Guards, Boundary Errors & Fail-Closed Zeroization ---

fn testTamperAndFailClosedZeroization(): !void {
    const key: [32]u8 = [0x42; 32];
    const nonce: [12]u8 = [0x24; 12];
    const aad: []const u8 = "header-auth-v1";
    const pt: []const u8 = "top-secret-agent-payload-non-aligned-len!";

    let ct: [41]u8 = undefined;
    let tag: [16]u8 = undefined;
    try Aes256Gcm.encryptDetached(&key, &nonce, pt, aad, &ct, &tag);

    // 1. Tamper single bit in CT -> must fail AND zeroize output buffer
    let bad_ct = ct;
    bad_ct[17] ^= 0x01;
    let out_buf: [41]u8 = [0xAA; 41];
    let failed = false;
    Aes256Gcm.decryptDetached(&key, &nonce, &bad_ct, &tag, aad, &out_buf) catch { failed = true; };
    expect_eq(failed, true);
    expect_eq(isAllZero(&out_buf), true);

    // 2. Tamper single bit in Tag -> must fail AND zeroize in-place buffer
    let bad_tag = tag;
    bad_tag[15] ^= 0x80;
    let inplace_buf = ct;
    let in_failed = false;
    Aes256Gcm.decryptInPlace(&key, &nonce, aad, &inplace_buf, &bad_tag) catch { in_failed = true; };
    expect_eq(in_failed, true);
    expect_eq(isAllZero(&inplace_buf), true);

    // 3. Tamper AAD / Nonce / Key on ChaCha20-Poly1305 -> all rejected & zeroized
    let cc_ct: [41]u8 = undefined;
    let cc_tag: [16]u8 = undefined;
    try ChaCha20Poly1305.encryptDetached(&key, &nonce, pt, aad, &cc_ct, &cc_tag);

    let out_cc: [41]u8 = [0xBB; 41];
    let aad_fail = false;
    ChaCha20Poly1305.decryptDetached(&key, &nonce, &cc_ct, &cc_tag, "header-auth-v2", &out_cc) catch { aad_fail = true; };
    expect_eq(aad_fail, true);
    expect_eq(isAllZero(&out_cc), true);

    let bad_nonce = nonce;
    bad_nonce[0] ^= 0x01;
    let n_fail = false;
    ChaCha20Poly1305.decryptDetached(&key, &bad_nonce, &cc_ct, &cc_tag, aad, &out_cc) catch { n_fail = true; };
    expect_eq(n_fail, true);

    let bad_key = key;
    bad_key[31] ^= 0x01;
    let k_fail = false;
    ChaCha20Poly1305.decryptDetached(&bad_key, &nonce, &cc_ct, &cc_tag, aad, &out_cc) catch { k_fail = true; };
    expect_eq(k_fail, true);

    // 4. Short key, short nonce, and combined ciphertext shorter than 16B tag
    let short_key: [16]u8 = [0; 16];
    let short_nonce: [8]u8 = [0; 8];
    let short_ct: [15]u8 = [0; 15];
    let err_k = false;
    Aes256Gcm.encryptDetached(&short_key, &nonce, pt, aad, &ct, &tag) catch { err_k = true; };
    expect_eq(err_k, true);

    let err_n = false;
    ChaCha20Poly1305.encryptDetached(&key, &short_nonce, pt, aad, &cc_ct, &cc_tag) catch { err_n = true; };
    expect_eq(err_n, true);

    let err_short = false;
    * = Aes256Gcm.openCombined(&key, &nonce, &short_ct, aad, &out_buf) catch { err_short = true; };
    expect_eq(err_short, true);

    let err_short_cc = false;
    * = ChaCha20Poly1305.openCombined(&key, &nonce, &short_ct, aad, &out_buf) catch { err_short_cc = true; };
    expect_eq(err_short_cc, true);
}

// --- Test 4: 16-Lane SIMD Tag Comparison & Block XOR vs Scalar Parity ---

fn testSimdVsScalarParity(): !void {
    let rng = Rng { state: 0x3A5C_7E91_2468_ACEF };

    // 1. Tag equality across all 16 lanes & single-bit flips
    for (iter of 0..128) {
        let ta: [16]u8 = undefined;
        let tb: [16]u8 = undefined;
        for (i of 0..16) {
            ta[i] = rng.nextU8();
            tb[i] = ta[i];
        }
        expect_eq(aeadTagEqlSimd(&ta, &tb), scalarTagEql(&ta, &tb));
        expect_eq(aeadTagEqlSimd(&ta, &tb), true);

        const lane = (iter as usize) % 16;
        tb[lane] ^= ((1 as u8) << @as(u3, @intCast((iter as usize) % 8)));
        expect_eq(aeadTagEqlSimd(&ta, &tb), scalarTagEql(&ta, &tb));
        expect_eq(aeadTagEqlSimd(&ta, &tb), false);
    }

    // 2. Block XOR across all lengths 0..96 (unaligned & multi-lane)
    let a_buf: [96]u8 = undefined;
    let b_buf: [96]u8 = undefined;
    let simd_out: [96]u8 = undefined;
    let scal_out: [96]u8 = undefined;
    for (len of 0..97) {
        const len_u = len as usize;
        for (i of 0..len) {
            a_buf[i] = rng.nextU8();
            b_buf[i] = rng.nextU8();
        }
        expect_eq(aeadXorSimd(simd_out[0..len_u], a_buf[0..len_u], b_buf[0..len_u]), true);
        expect_eq(scalarXor(scal_out[0..len_u], a_buf[0..len_u], b_buf[0..len_u]), true);
        expect_eq(std.mem.eql(u8, simd_out[0..len_u], scal_out[0..len_u]), true);
    }
}

// --- Test 5: 1,500-Case Deterministic PRNG Variable-Length Roundtrip & Tamper Fuzz ---

fn testDeterministicAeadFuzz(): !void {
    let rng = Rng { state: 0x5E7B_19D3_4C8A_2F61 };
    let key: [32]u8 = undefined;
    let nonce: [12]u8 = undefined;
    let pt_buf: [160]u8 = undefined;
    let aad_buf: [64]u8 = undefined;
    let sealed_buf: [176]u8 = undefined;
    let open_buf: [160]u8 = undefined;

    let total: usize = 0;
    for (case_idx of 0..1500) {
        for (i of 0..32) key[i] = rng.nextU8();
        for (i of 0..12) nonce[i] = rng.nextU8();

        const pt_len = rng.nextRange(161);
        const aad_len = rng.nextRange(65);
        for (i of 0..pt_len) pt_buf[i] = rng.nextU8();
        for (i of 0..aad_len) aad_buf[i] = rng.nextU8();

        const pt = pt_buf[0..pt_len];
        const aad = aad_buf[0..aad_len];
        const use_aes = ((case_idx as usize) % 2) == 0;

        const sealed = if (use_aes)
            try Aes256Gcm.sealCombined(&key, &nonce, pt, aad, &sealed_buf)
        else
            try ChaCha20Poly1305.sealCombined(&key, &nonce, pt, aad, &sealed_buf);

        expect_eq(sealed.len, pt_len + 16);

        // Valid roundtrip must recover exact plaintext
        const opened = if (use_aes)
            try Aes256Gcm.openCombined(&key, &nonce, sealed, aad, &open_buf)
        else
            try ChaCha20Poly1305.openCombined(&key, &nonce, sealed, aad, &open_buf);
        expect_eq(std.mem.eql(u8, opened, pt), true);

        // Flip 1 random bit in combined (ciphertext or tag) -> MUST fail & zeroize open_buf[0..pt_len]
        const flip_pos = rng.nextRange(sealed.len);
        sealed_buf[flip_pos] ^= ((1 as u8) << @as(u3, @intCast(rng.nextRange(8))));
        if (pt_len > 0) open_buf[0] = 0xFF;

        let tampered_err = false;
        if (use_aes) {
            * = Aes256Gcm.openCombined(&key, &nonce, sealed_buf[0..sealed.len], aad, &open_buf) catch {
                tampered_err = true;
            };
        } else {
            * = ChaCha20Poly1305.openCombined(&key, &nonce, sealed_buf[0..sealed.len], aad, &open_buf) catch {
                tampered_err = true;
            };
        }
        expect_eq(tampered_err, true);
        expect_eq(isAllZero(open_buf[0..pt_len]), true);

        total += 1;
    }
    expect_eq(total, 1500 as usize);
}

fn main() {
    test("1. RFC 8439 Section 2.8.2 ChaCha20-Poly1305 Official Golden Vector", () => {
        try testRfc8439ChaCha20Poly1305Vector();
    });
    test("2. NIST SP 800-38D CAVP AES-256-GCM Official Vectors", () => {
        try testNistAes256GcmVectors();
    });
    test("3. Single-Bit Tamper Rejection & Fail-Closed Zeroization", () => {
        try testTamperAndFailClosedZeroization();
    });
    test("4. 16-Lane SIMD Tag Compare & Block XOR vs Scalar Parity", () => {
        try testSimdVsScalarParity();
    });
    test("5. 1,500-Case Deterministic PRNG AEAD Roundtrip & Tamper Fuzz Suite", () => {
        try testDeterministicAeadFuzz();
    });
}
