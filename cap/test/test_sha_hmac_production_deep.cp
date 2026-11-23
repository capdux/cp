// Source: cp/cap/test/test_sha_hmac_production_deep.cp (Production deep test suite for cap:sha_hmac & cap.sha_hmac)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ShaError,
    HashAlg,
    digestLen,
    blockLen,
    StreamingHasher,
    StreamingHmac,
    digestHex,
    hashOneshot,
    hashHex,
    hmacOneshot,
    hmacHex,
    verifyHmac,
    shaHkdfExtract as hkdfExtract,
    shaHkdfExpand as hkdfExpand,
    hkdf,
    pbkdf2HmacSha256,
    pbkdf2Hmac,
} from "cap:crypto";

test("RFC 4231 HMAC-SHA256 and HMAC-SHA512 official test vectors") {
    const alloc = std.testing.allocator;

    // Test Case 1: key = 0x0b * 20, data = "Hi There"
    let key1: [20]u8 = undefined;
    @memset(&key1, 0x0b);
    const mac1 = try hmacHex(alloc, .Sha256, &key1, "Hi There");
    defer alloc.free(mac1);
    try expect_eq(
        mac1,
        "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7",
    );

    // Test Case 2: key = "Jefe", data = "what do ya want for nothing?"
    const mac2_256 = try hmacHex(alloc, .Sha256, "Jefe", "what do ya want for nothing?");
    defer alloc.free(mac2_256);
    try expect_eq(
        mac2_256,
        "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843",
    );

    const mac2_512 = try hmacHex(alloc, .Sha512, "Jefe", "what do ya want for nothing?");
    defer alloc.free(mac2_512);
    try expect_eq(
        mac2_512,
        "164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea2505549758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737",
    );
}

test("RFC 5869 HKDF-SHA256 official test vector and out_len limit enforcement") {
    const alloc = std.testing.allocator;

    // RFC 5869 Appendix A.1 (Test Case 1)
    let ikm: [22]u8 = undefined;
    @memset(&ikm, 0x0b);
    const salt = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c";
    const info = "\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9";

    const prk = try hkdfExtract(alloc, .Sha256, salt, &ikm);
    defer alloc.free(prk);
    const prk_hex = try digestHex(alloc, prk);
    defer alloc.free(prk_hex);
    try expect_eq(prk_hex, "077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5");

    const okm = try hkdf(alloc, .Sha256, salt, &ikm, info, 42);
    defer alloc.free(okm);
    const okm_hex = try digestHex(alloc, okm);
    defer alloc.free(okm_hex);
    try expect_eq(
        okm_hex,
        "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865",
    );

    // Enforce RFC 5869 max length: 255 * HashLen (255 * 32 = 8160)
    const err_res = hkdf(alloc, .Sha256, salt, &ikm, info, 8161);
    try std.testing.expectError(ShaError.OutputTooLong, err_res);

    const err_expand = hkdfExpand(alloc, .Sha256, prk, info, 8161);
    try std.testing.expectError(ShaError.OutputTooLong, err_expand);
}

test("RFC 6070 / RFC 7914 PBKDF2-HMAC-SHA256 vectors and zero iterations guard") {
    const alloc = std.testing.allocator;

    const dk1 = try pbkdf2HmacSha256(alloc, "password", "salt", 1, 32);
    defer alloc.free(dk1);
    const hex1 = try digestHex(alloc, dk1);
    defer alloc.free(hex1);
    try expect_eq(hex1, "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b");

    const dk2 = try pbkdf2HmacSha256(alloc, "password", "salt", 2, 32);
    defer alloc.free(dk2);
    const hex2 = try digestHex(alloc, dk2);
    defer alloc.free(hex2);
    try expect_eq(hex2, "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43");

    const dk3 = try pbkdf2Hmac(alloc, .Sha256, "password", "salt", 4096, 32);
    defer alloc.free(dk3);
    const hex3 = try digestHex(alloc, dk3);
    defer alloc.free(hex3);
    try expect_eq(hex3, "c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a");

    // Enforce iterations >= 1
    const zero_err = pbkdf2HmacSha256(alloc, "password", "salt", 0, 32);
    try std.testing.expectError(ShaError.InvalidIterations, zero_err);
}

test("StreamingHasher & StreamingHmac 1..7 byte chunking matches oneshot and verifyHmac") {
    const alloc = std.testing.allocator;
    const msg = "The quick brown fox jumps over the lazy dog while computing streaming SHA and HMAC digests across multiple block boundaries 0123456789abcdef!";
    const key = "super-secret-streaming-key-for-hmac-verification";

    const algs: [7]HashAlg = [.Sha1, .Sha224, .Sha256, .Sha384, .Sha512, .Blake2b256, .Blake2s256];
    const chunk_sizes: [7]usize = [1, 2, 3, 4, 5, 6, 7];

    for (alg of algs) {
        const expected_hash = try hashOneshot(alloc, alg, msg);
        defer alloc.free(expected_hash);
        const expected_hmac = try hmacOneshot(alloc, alg, key, msg);
        defer alloc.free(expected_hmac);

        try std.testing.expectEqual(digestLen(alg), expected_hash.len);
        try std.testing.expect(blockLen(alg) >= 64);
        try std.testing.expect(verifyHmac(alg, key, msg, expected_hmac));

        // Tampered tag & wrong length must return false
        let tampered = try alloc.dupe(u8, expected_hmac);
        defer alloc.free(tampered);
        tampered[0] ^= 0x01;
        try std.testing.expect(!verifyHmac(alg, key, msg, tampered));
        try std.testing.expect(!verifyHmac(alg, key, msg, expected_hmac[0 .. expected_hmac.len - 1]));

        // Arbitrary 1..7 byte chunking
        let hasher = StreamingHasher.init(alg);
        let hmac = StreamingHmac.init(alg, key);
        let pos: usize = 0;
        let step_idx: usize = 0;
        while (pos < msg.len) {
            const step = chunk_sizes[step_idx % chunk_sizes.len];
            const end = @min(pos + step, msg.len);
            hasher.update(msg[pos..end]);
            hmac.update(msg[pos..end]);
            pos = end;
            step_idx += 1;
        }

        let h_buf: [64]u8 = undefined;
        let m_buf: [64]u8 = undefined;
        const stream_hash = try hasher.final(&h_buf);
        const stream_hmac = try hmac.final(&m_buf);
        try std.testing.expectEqualSlices(u8, expected_hash, stream_hash);
        try std.testing.expectEqualSlices(u8, expected_hmac, stream_hmac);
    }
}

test("cap.sha_hmac dynamic mount and zero memory leaks under std.testing.allocator") {
    const alloc = std.testing.allocator;

    const sha_hex = try cap.crypto.hashHex(alloc, .Sha256, "abc");
    defer alloc.free(sha_hex);
    try expect_eq(sha_hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    const mac_raw = try cap.crypto.hmacOneshot(alloc, .Sha256, "k", "v");
    defer alloc.free(mac_raw);
    try std.testing.expect(cap.crypto.verifyHmac(.Sha256, "k", "v", mac_raw));

    const derived = try cap.crypto.hkdf(alloc, .Sha256, "s", "ikm", "ctx", 16);
    defer alloc.free(derived);
    try std.testing.expectEqual(@as(usize, 16), derived.len);

    const pbk = try cap.crypto.pbkdf2HmacSha256(alloc, "pw", "salt", 10, 16);
    defer alloc.free(pbk);
    try std.testing.expectEqual(@as(usize, 16), pbk.len);
}
