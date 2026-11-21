// Source: cp/cap/test/test_mysql_auth_production_deep.cp (Production deep tests for cap:mysql auth)

import { test, expect_eq } from "cap:test";
import {
    AuthMethod,
    AuthFastStatus,
    SSLRequest,
    NATIVE_SCRAMBLE_LEN,
    CACHING_SHA2_SCRAMBLE_LEN,
    SSL_REQUEST_FRAME_LEN,
    CAP_CLIENT_SSL,
    scrambleNativePassword,
    scrambleCachingSha2Password,
    encodeCleartextPassword,
    xorPasswordWithNonce,
} from "cap:mysql";

test("AuthMethod plugin name recognition and AuthFastStatus mapping") {
    const names: [4]string = [
        "mysql_native_password",
        "caching_sha2_password",
        "mysql_clear_password",
        "sha256_password",
    ];
    let i: usize = 0;
    while (i < 4) {
        if (m := AuthMethod.fromPluginName(names[i])) {
            expect_eq(m.pluginName(), names[i]);
            expect_eq(m.requiresTls(), i == 2);
        } else {
            expect_eq(true, false);
        }
        i += 1;
    }

    expect_eq(AuthMethod.fromPluginName("unknown_plugin") == null, true);
    expect_eq(AuthMethod.fromPluginName("") == null, true);

    expect_eq(AuthFastStatus.fromByte(0x03).? == .success, true);
    expect_eq(AuthFastStatus.fromByte(0x04).? == .continue_auth, true);
    expect_eq(AuthFastStatus.fromByte(0x05) == null, true);
}

test("scrambleNativePassword SHA-1 vector and XOR invertibility") {
    const pw = "my_secret_password";
    const salt = "0123456789abcdefghij";

    let out: [20]u8 = [0; 20];
    const scrambled = scrambleNativePassword(pw, salt, out[0..20]) catch unreachable;
    expect_eq(scrambled.len, NATIVE_SCRAMBLE_LEN);

    // 独立计算 SHA1(pw) XOR SHA1(salt || SHA1(SHA1(pw))) 逐字节比对
    let h1: [20]u8 = [0; 20];
    let h2: [20]u8 = [0; 20];
    let h3: [20]u8 = [0; 20];
    std.crypto.hash.Sha1.hash(pw, &h1, {});
    std.crypto.hash.Sha1.hash(h1[0..20], &h2, {});

    let combined: [40]u8 = [0; 40];
    @memcpy(combined[0..20], salt[0..20]);
    @memcpy(combined[20..40], h2[0..20]);
    std.crypto.hash.Sha1.hash(combined[0..40], &h3, {});

    let idx: usize = 0;
    while (idx < 20) {
        expect_eq(scrambled[idx], h1[idx] ^ h3[idx]);
        // 服务端验证性质：scrambled XOR h3 == h1
        expect_eq(scrambled[idx] ^ h3[idx], h1[idx]);
        idx += 1;
    }
}

test("scrambleCachingSha2Password SHA-256 vector and salt sensitivity") {
    const pw = "mysql8_strong_pass!";
    const salt_a = "ABCDEFGHIJ0123456789";
    const salt_b = "ABCDEFGHIJ0123456780";

    let out_a: [32]u8 = [0; 32];
    let out_b: [32]u8 = [0; 32];
    const res_a = scrambleCachingSha2Password(pw, salt_a, out_a[0..32]) catch unreachable;
    const res_b = scrambleCachingSha2Password(pw, salt_b, out_b[0..32]) catch unreachable;
    expect_eq(res_a.len, CACHING_SHA2_SCRAMBLE_LEN);
    expect_eq(std.mem.eql(u8, res_a, res_b), false);

    // 独立计算 SHA256(pw) XOR SHA256(SHA256(SHA256(pw)) || salt_a)
    let d1: [32]u8 = [0; 32];
    let d2: [32]u8 = [0; 32];
    let d3: [32]u8 = [0; 32];
    std.crypto.hash.sha2.Sha256.hash(pw, &d1, {});
    std.crypto.hash.sha2.Sha256.hash(d1[0..32], &d2, {});

    let buf52: [52]u8 = [0; 52];
    @memcpy(buf52[0..32], d2[0..32]);
    @memcpy(buf52[32..52], salt_a[0..20]);
    std.crypto.hash.sha2.Sha256.hash(buf52[0..52], &d3, {});

    let i: usize = 0;
    while (i < 32) {
        expect_eq(res_a[i], d1[i] ^ d3[i]);
        i += 1;
    }
}

test("Empty password returns empty slice and cleartext/XOR helpers") {
    const salt = "0123456789abcdefghij";
    let buf32: [32]u8 = [0xFF; 32];

    const empty_native = scrambleNativePassword("", salt, buf32[0..20]) catch unreachable;
    expect_eq(empty_native.len, @as(usize, 0));

    const empty_sha2 = scrambleCachingSha2Password("", salt, buf32[0..32]) catch unreachable;
    expect_eq(empty_sha2.len, @as(usize, 0));

    let clear_buf: [16]u8 = [0xAA; 16];
    const clear = encodeCleartextPassword("root", clear_buf[0..16]) catch unreachable;
    expect_eq(clear.len, @as(usize, 5));
    expect_eq(std.mem.eql(u8, clear[0..4], "root"), true);
    expect_eq(clear[4], @as(u8, 0));

    // xorPasswordWithNonce 双次异或还原 NUL 结尾明文
    let x1: [16]u8 = [0; 16];
    const masked = xorPasswordWithNonce("hello", salt, x1[0..16]) catch unreachable;
    expect_eq(masked.len, @as(usize, 6));
    let i: usize = 0;
    while (i < 5) {
        expect_eq(masked[i] ^ salt[i], "hello"[i]);
        i += 1;
    }
    expect_eq(masked[5] ^ salt[5], @as(u8, 0));
}

test("SSLRequest 36-byte frame encode/decode roundtrip") {
    const req = SSLRequest{
        capability_flags: CAP_CLIENT_SSL | 0x01080200,
        max_packet_size: 0x01000000,
        character_set: 45,
        sequence_id: 1,
    };

    let frame: [36]u8 = [0xFF; 36];
    const encoded = req.encode(frame[0..36]) catch unreachable;
    expect_eq(encoded.len, SSL_REQUEST_FRAME_LEN);
    // 校验 4 字节包头与 23 字节零填充区
    expect_eq(encoded[0], @as(u8, 32));
    expect_eq(encoded[1], @as(u8, 0));
    expect_eq(encoded[2], @as(u8, 0));
    expect_eq(encoded[3], @as(u8, 1));
    let pad_i: usize = 13;
    while (pad_i < 36) {
        expect_eq(encoded[pad_i], @as(u8, 0));
        pad_i += 1;
    }

    const decoded = SSLRequest.decode(encoded) catch unreachable;
    expect_eq(decoded.eql(req), true);
    expect_eq(decoded.hasSslCapability(), true);
}

test("Auth scramble and SSLRequest malformed boundary guards") {
    let small: [10]u8 = [0; 10];
    let ok32: [32]u8 = [0; 32];

    // 盐值不足 20 字节 -> MissingSalt
    expect_eq(switch (scrambleNativePassword("pw", "short_salt", ok32[0..20])) { err.MissingSalt => true, else => false }, true);
    expect_eq(switch (scrambleCachingSha2Password("pw", "short_salt", ok32[0..32])) { err.MissingSalt => true, else => false }, true);
    expect_eq(switch (xorPasswordWithNonce("pw", "", ok32[0..10])) { err.MissingSalt => true, else => false }, true);

    // 输出缓冲不足 -> BufferTooSmall
    const salt = "0123456789abcdefghij";
    expect_eq(switch (scrambleNativePassword("pw", salt, small[0..10])) { err.BufferTooSmall => true, else => false }, true);
    expect_eq(switch (scrambleCachingSha2Password("pw", salt, small[0..10])) { err.BufferTooSmall => true, else => false }, true);
    expect_eq(switch (encodeCleartextPassword("0123456789", small[0..10])) { err.BufferTooSmall => true, else => false }, true);

    // SSLRequest 缓冲不足、截断帧与非法包头长度
    const req = SSLRequest{ capability_flags: CAP_CLIENT_SSL };
    expect_eq(switch (req.encode(small[0..10])) { err.BufferTooSmall => true, else => false }, true);
    expect_eq(switch (SSLRequest.decode(small[0..10])) { err.TruncatedFrame => true, else => false }, true);

    let bad_frame: [36]u8 = [0; 36];
    bad_frame[0] = 16; // != 32
    expect_eq(switch (SSLRequest.decode(bad_frame[0..36])) { err.InvalidFrameLength => true, else => false }, true);
}
