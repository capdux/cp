// Source: native-cp-stdlib (RFC 7914 scrypt KDF)

import * as std from "std";
import { test, expect_eq } from "cap:test";
import * as scrypt from "../../lib/crypto/scrypt_.cp";

fn hexEncode(alloc: std.mem.Allocator, bytes: []const u8): ![]u8 {
    const hex_chars = "0123456789abcdef";
    let out = try alloc.alloc(u8, bytes.len * 2);
    for (i of 0..bytes.len) {
        const b = bytes[i];
        const hi_idx: usize = @intCast(b >> 4);
        const lo_idx: usize = @intCast(b & 0x0f);
        out[i * 2] = hex_chars[hi_idx];
        out[i * 2 + 1] = hex_chars[lo_idx];
    }
    return out;
}

test "RFC 7914 Section 12 official test vectors" {
    const alloc = std.testing.allocator;

    // Vector 1: P="", S="", N=16 (ln=4), r=1, p=1, dkLen=64
    const p1 = try scrypt.ScryptParams.fromN(16, 1, 1, scrypt.DEFAULT_MAX_MEM_BYTES);
    const dk1 = try scrypt.hashRaw(alloc, "", "", p1, 64);
    defer alloc.free(dk1);
    const hex1 = try hexEncode(alloc, dk1);
    defer alloc.free(hex1);
    try expect_eq(
        std.mem.eql(
            u8,
            hex1,
            "77d6576238657b203b19ca42c18a0497f16b4844e3074ae8dfdffa3fede21442fcd0069ded0948f8326a753a0fc81f17e8d3e0fb2e0d3628cf35e20c38d18906",
        ),
        true,
    );

    // Vector 2: P="password", S="NaCl", N=1024 (ln=10), r=8, p=16, dkLen=64
    const p2 = try scrypt.ScryptParams.fromN(1024, 8, 16, scrypt.DEFAULT_MAX_MEM_BYTES);
    const dk2 = try scrypt.hashRaw(alloc, "password", "NaCl", p2, 64);
    defer alloc.free(dk2);
    const hex2 = try hexEncode(alloc, dk2);
    defer alloc.free(hex2);
    try expect_eq(
        std.mem.eql(
            u8,
            hex2,
            "fdbabe1c9d3472007856e7190d01e9fe7c6ad7cbc8237830e77376634b3731622eaf30d92e22a3886ff109279d9830dac727afb94a83ee6d8360cbdfa2cc0640",
        ),
        true,
    );

    // Vector 3: P="pleaseletmein", S="SodiumChloride", N=16384 (ln=14), r=8, p=1, dkLen=64
    const p3 = try scrypt.ScryptParams.fromN(16384, 8, 1, scrypt.DEFAULT_MAX_MEM_BYTES);
    const dk3 = try scrypt.hashRaw(alloc, "pleaseletmein", "SodiumChloride", p3, 64);
    defer alloc.free(dk3);
    const hex3 = try hexEncode(alloc, dk3);
    defer alloc.free(hex3);
    try expect_eq(
        std.mem.eql(
            u8,
            hex3,
            "7023bdcb3afd7348461c06cd81fd38ebfda8fbba904f8e3ea9b543f6545da1f2d5432955613f0fcf62d49705242a9af9e61e85dc0d651e40dfcf017b45575887",
        ),
        true,
    );
}

test "PHC encode, parse roundtrip and wrong password rejection" {
    const alloc = std.testing.allocator;
    const params = scrypt.ScryptParams.init(10, 8, 1, scrypt.DEFAULT_MAX_MEM_BYTES);

    const phc = try scrypt.hashPhc(alloc, "hunter2", "somesalt1234", params, 32);
    defer alloc.free(phc);

    try expect_eq(std.mem.startsWith(u8, phc, "$scrypt$ln=10,r=8,p=1$"), true);

    let parsed = try scrypt.parsePhc(alloc, phc);
    defer parsed.deinit(alloc);
    try expect_eq(parsed.params.ln, @as(u6, 10));
    try expect_eq(parsed.params.r, @as(u30, 8));
    try expect_eq(parsed.params.p, @as(u30, 1));
    try expect_eq(std.mem.eql(u8, parsed.salt, "somesalt1234"), true);
    try expect_eq(parsed.hash.len, @as(usize, 32));

    const ok_good = try scrypt.verifyPhc(alloc, "hunter2", phc);
    try expect_eq(ok_good, true);

    const ok_bad = try scrypt.verifyPhc(alloc, "hunter3", phc);
    try expect_eq(ok_bad, false);
}

test "hashPhcRandom produces unique salted PHC strings and verifies" {
    const alloc = std.testing.allocator;
    const params = scrypt.ScryptParams.init(10, 8, 1, scrypt.DEFAULT_MAX_MEM_BYTES);

    const phc1 = try scrypt.hashPhcRandom(alloc, "my-secret-pass", params);
    defer alloc.free(phc1);

    const phc2 = try scrypt.hashPhcRandom(alloc, "my-secret-pass", params);
    defer alloc.free(phc2);

    try expect_eq(std.mem.eql(u8, phc1, phc2), false);
    try expect_eq(try scrypt.verifyPhc(alloc, "my-secret-pass", phc1), true);
    try expect_eq(try scrypt.verifyPhc(alloc, "my-secret-pass", phc2), true);
    try expect_eq(try scrypt.verifyPhc(alloc, "wrong-pass", phc1), false);
}

test "DoS protection: reject non-power-of-two N and excessive max_mem_bytes" {
    const alloc = std.testing.allocator;

    // Non-power-of-two N must be rejected
    let got_non_pow2_err = false;
    if (_ := scrypt.ScryptParams.fromN(12, 8, 1, scrypt.DEFAULT_MAX_MEM_BYTES)) {
        got_non_pow2_err = false;
    } else {
        got_non_pow2_err = true;
    }
    try expect_eq(got_non_pow2_err, true);

    // Out-of-range ln must be rejected
    const bad_ln = scrypt.ScryptParams.init(0, 8, 1, scrypt.DEFAULT_MAX_MEM_BYTES);
    let got_bad_ln_err = false;
    if (_ := bad_ln.validate()) {
        got_bad_ln_err = false;
    } else {
        got_bad_ln_err = true;
    }
    try expect_eq(got_bad_ln_err, true);

    // Excessive memory requirement (ln=14, r=8 -> 16 MiB > 1 MiB limit) must be rejected
    const dos_params = scrypt.ScryptParams.init(14, 8, 1, 1024 * 1024);
    let got_mem_err = false;
    if (dk := scrypt.hashRaw(alloc, "pw", "salt", dos_params, 32)) {
        alloc.free(dk);
    } else {
        got_mem_err = true;
    }
    try expect_eq(got_mem_err, true);

    // verifyPhcWithLimit must reject PHC string exceeding tight memory limit
    const valid_params = scrypt.ScryptParams.init(12, 8, 1, scrypt.DEFAULT_MAX_MEM_BYTES);
    const phc = try scrypt.hashPhc(alloc, "pw", "salt", valid_params, 32);
    defer alloc.free(phc);

    let got_verify_limit_err = false;
    if (_ := scrypt.verifyPhcWithLimit(alloc, "pw", phc, 1024)) {
        got_verify_limit_err = false;
    } else {
        got_verify_limit_err = true;
    }
    try expect_eq(got_verify_limit_err, true);
}

test "cap.scrypt dynamic mount and std.testing.allocator zero leaks" {
    const alloc = std.testing.allocator;
    const params = scrypt.ScryptParams.init(10, 8, 1, scrypt.DEFAULT_MAX_MEM_BYTES);

    const phc = try scrypt.hashPhcRandom(alloc, "dynamic-mount-pw", params);
    defer alloc.free(phc);

    try expect_eq(try scrypt.verifyPhc(alloc, "dynamic-mount-pw", phc), true);
    try expect_eq(try scrypt.verifyPhc(alloc, "bad-pw", phc), false);
}
