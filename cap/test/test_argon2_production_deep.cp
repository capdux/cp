// Source: cp/cap/test/test_argon2_production_deep.cp (Production deep test suite for cap:argon2 & cap.argon2)

const std = @import("std");
import cap from "cap";
import { test, expect_eq } from "cap:test";
import {
    Argon2Error,
    Argon2Variant,
    Argon2Params,
    hashRaw,
    hashPhc,
    hashPhcRandom,
    parsePhc,
    verifyPhc,
    verifyPhcWithLimit,
} from "cap:argon2";

test("RFC 9106 official test vectors (Argon2id / Argon2i / Argon2d)") {
    const alloc = std.testing.allocator;
    const password: [32]u8 = @splat(0x01);
    const salt: [16]u8 = @splat(0x02);
    const secret: [8]u8 = @splat(0x03);
    const ad: [12]u8 = @splat(0x04);

    // 1. Argon2id (m=32, t=3, p=4)
    const dk_id = try hashRaw(alloc, &password, &salt, Argon2Params{
        .variant = .Argon2id,
        .t_cost = 3,
        .m_cost_kib = 32,
        .lanes = 4,
        .out_len = 32,
        .secret = &secret,
        .ad = &ad,
    });
    defer alloc.free(dk_id);

    let want_id: [32]u8 = undefined;
    * = try std.fmt.hexToBytes(&want_id, "0d640df58d78766c08c037a34a8b53c9d01ef0452d75b65eb52520e96b01e659");
    try std.testing.expectEqualSlices(u8, &want_id, dk_id);

    // 2. Argon2i (m=32, t=3, p=4)
    const dk_i = try hashRaw(alloc, &password, &salt, Argon2Params{
        .variant = .Argon2i,
        .t_cost = 3,
        .m_cost_kib = 32,
        .lanes = 4,
        .out_len = 32,
        .secret = &secret,
        .ad = &ad,
    });
    defer alloc.free(dk_i);

    let want_i: [32]u8 = undefined;
    * = try std.fmt.hexToBytes(&want_i, "c814d9d1dc7f37aa13f0d77f2494bda1c8de6b016dd388d29952a4c4672b6ce8");
    try std.testing.expectEqualSlices(u8, &want_i, dk_i);

    // 3. Argon2d ("password", "somesalt", m=64, t=1, p=1, out_len=24)
    const dk_d = try hashRaw(alloc, "password", "somesalt", Argon2Params{
        .variant = .Argon2d,
        .t_cost = 1,
        .m_cost_kib = 64,
        .lanes = 1,
        .out_len = 24,
    });
    defer alloc.free(dk_d);

    let want_d: [24]u8 = undefined;
    * = try std.fmt.hexToBytes(&want_d, "8727405fd07c32c78d64f547f24150d3f2e703a89f981a19");
    try std.testing.expectEqualSlices(u8, &want_d, dk_d);
}

test("hashPhc and verifyPhc roundtrip with wrong password rejection") {
    const alloc = std.testing.allocator;
    const params = Argon2Params{
        .variant = .Argon2id,
        .t_cost = 2,
        .m_cost_kib = 32,
        .lanes = 4,
        .out_len = 32,
    };

    const phc = try hashPhc(alloc, "super-secret-pass", "somesalt1234", params);
    defer alloc.free(phc);

    try expect_eq(std.mem.startsWith(u8, phc, "$argon2id$v=19$m=32,t=2,p=4$"), true);
    try expect_eq(try verifyPhc(alloc, "super-secret-pass", phc), true);
    try expect_eq(try verifyPhc(alloc, "wrong-password", phc), false);
}

test("hashPhcRandom generates unique CSPRNG salts and verifies") {
    const alloc = std.testing.allocator;
    const params = Argon2Params{
        .variant = .Argon2id,
        .t_cost = 1,
        .m_cost_kib = 32,
        .lanes = 2,
        .out_len = 32,
    };

    const phc1 = try hashPhcRandom(alloc, "same-password", params);
    defer alloc.free(phc1);

    const phc2 = try hashPhcRandom(alloc, "same-password", params);
    defer alloc.free(phc2);

    try expect_eq(std.mem.eql(u8, phc1, phc2), false);
    try expect_eq(try verifyPhc(alloc, "same-password", phc1), true);
    try expect_eq(try verifyPhc(alloc, "same-password", phc2), true);
    try expect_eq(try verifyPhc(alloc, "other-password", phc1), false);
}

test("parameter validation, malformed PHC, and DoS memory limit guards") {
    const alloc = std.testing.allocator;

    // t_cost = 0
    try std.testing.expectError(Argon2Error.InvalidParams, hashRaw(alloc, "pw", "somesalt", Argon2Params{
        .t_cost = 0,
        .m_cost_kib = 32,
        .lanes = 1,
    }));

    // m_cost_kib < 8 * lanes
    try std.testing.expectError(Argon2Error.InvalidParams, hashRaw(alloc, "pw", "somesalt", Argon2Params{
        .t_cost = 1,
        .m_cost_kib = 16,
        .lanes = 4,
    }));

    // short salt (< 8 bytes)
    try std.testing.expectError(Argon2Error.InvalidSalt, hashRaw(alloc, "pw", "short", Argon2Params{
        .t_cost = 1,
        .m_cost_kib = 32,
        .lanes = 1,
    }));

    // malformed PHC strings
    try std.testing.expectError(Argon2Error.InvalidPhcFormat, verifyPhc(alloc, "pw", "not-a-phc-string"));
    try std.testing.expectError(Argon2Error.UnsupportedVariant, verifyPhc(alloc, "pw", "$bcrypt$v=19$m=32,t=1,p=1$c29tZXNhbHQ$c29tZWhhc2g"));
    try std.testing.expectError(Argon2Error.UnsupportedVersion, verifyPhc(alloc, "pw", "$argon2id$v=16$m=32,t=1,p=1$c29tZXNhbHQ$c29tZWhhc2g"));

    // DoS memory limit guard
    try std.testing.expectError(
        Argon2Error.MemoryLimitExceeded,
        verifyPhcWithLimit(alloc, "pw", "$argon2id$v=19$m=65536,t=2,p=1$c29tZXNhbHQ$c29tZWhhc2g", 1024),
    );
}

test("cap.argon2 dynamic mount and zero-leak verification") {
    const alloc = std.testing.allocator;
    const params = cap.argon2.Argon2Params{
        .variant = .Argon2i,
        .t_cost = 1,
        .m_cost_kib = 32,
        .lanes = 1,
        .out_len = 32,
    };

    const phc = try cap.argon2.hashPhc(alloc, "cap-mount-test", "12345678", params);
    defer alloc.free(phc);

    let parsed = try cap.argon2.parsePhc(alloc, phc, 1024);
    defer parsed.deinit(alloc);

    try expect_eq(parsed.variant == .Argon2i, true);
    try expect_eq(parsed.m_cost_kib, 32);
    try expect_eq(try cap.argon2.verifyPhc(alloc, "cap-mount-test", phc), true);
}
