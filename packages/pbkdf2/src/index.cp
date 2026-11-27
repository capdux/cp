// @cap/pbkdf2-sha256 — Pure CP zero-allocation RFC 2898 / RFC 8018 PBKDF2-HMAC-SHA256
// Parity with RustCrypto `pbkdf2` & npm `pbkdf2`.
// Reuses SHA-256 & constant-time `ctEqual` from `@cap/cookie-signature`.
// Accelerated with 16-lane @Vector(16, u8) SIMD pad XOR & 32-byte block XOR accumulation,
// plus bulk @memcpy/@memset and undefined stack buffers.

import {
    Sha256State,
    sha256Init,
    sha256Update,
    sha256Final,
    sha256,
    ctEqual
} from "../../cookie-signature/src/index.cp";

type PbkVec16 = @Vector(16, u8);

struct Pbkdf2Pads {
    inner_base: Sha256State,
    outer_base: Sha256State,
}

fn pbkdf2Xor32Simd(acc: *[32]u8, cur: [32]u8) {
    const a0: PbkVec16 = acc.*[0..16].*;
    const a1: PbkVec16 = acc.*[16..32].*;
    const b0: PbkVec16 = cur[0..16].*;
    const b1: PbkVec16 = cur[16..32].*;
    const r0: [16]u8 = a0 ^ b0;
    const r1: [16]u8 = a1 ^ b1;
    @memcpy(acc.*[0..16], r0[0..16]);
    @memcpy(acc.*[16..32], r1[0..16]);
}

fn pbkdf2InitPads(password: []const u8): Pbkdf2Pads {
    let key_blk: [64]u8 = undefined;
    @memset(key_blk[0..64], 0);

    if (password.len > 64) {
        sha256(password, key_blk[0..32]);
    } else if (password.len > 0) {
        @memcpy(key_blk[0..password.len], password[0..password.len]);
    }

    let ipad: [64]u8 = undefined;
    let opad: [64]u8 = undefined;
    const v36: PbkVec16 = @splat(0x36 as u8);
    const v5c: PbkVec16 = @splat(0x5C as u8);

    let off: usize = 0;
    while (off < 64) {
        const kv: PbkVec16 = key_blk[off .. off + 16][0..16].*;
        const iv: [16]u8 = kv ^ v36;
        const ov: [16]u8 = kv ^ v5c;
        @memcpy(ipad[off .. off + 16], iv[0..16]);
        @memcpy(opad[off .. off + 16], ov[0..16]);
        off += 16;
    }

    let inner_base = sha256Init();
    sha256Update(&inner_base, ipad[0..64]);

    let outer_base = sha256Init();
    sha256Update(&outer_base, opad[0..64]);

    return Pbkdf2Pads {
        inner_base: inner_base,
        outer_base: outer_base,
    };
}

fn pbkdf2HmacSaltBlock(pads: Pbkdf2Pads, salt: []const u8, block_idx: u32, out32: []u8) {
    const idx_be: [4]u8 = [
        ((block_idx >> 24) & 0xFF) as u8,
        ((block_idx >> 16) & 0xFF) as u8,
        ((block_idx >> 8) & 0xFF) as u8,
        (block_idx & 0xFF) as u8
    ];

    let inner = pads.inner_base;
    if (salt.len > 0) {
        sha256Update(&inner, salt);
    }
    sha256Update(&inner, idx_be[0..4]);
    let inner_hash: [32]u8 = undefined;
    sha256Final(&inner, inner_hash[0..32]);

    let outer = pads.outer_base;
    sha256Update(&outer, inner_hash[0..32]);
    sha256Final(&outer, out32);
}

fn pbkdf2Hmac32(pads: Pbkdf2Pads, msg32: []u8, out32: []u8) {
    let inner = pads.inner_base;
    sha256Update(&inner, msg32);
    let inner_hash: [32]u8 = undefined;
    sha256Final(&inner, inner_hash[0..32]);

    let outer = pads.outer_base;
    sha256Update(&outer, inner_hash[0..32]);
    sha256Final(&outer, out32);
}

// RFC 2898 Section 5.2: PBKDF2-HMAC-SHA256
pub fn pbkdf2HmacSha256(
    password: []const u8,
    salt: []const u8,
    iterations: u32,
    out_dk: []u8
): bool {
    if (iterations == 0) {
        return false;
    }
    if (out_dk.len == 0) {
        return true;
    }

    const pads = pbkdf2InitPads(password);
    let u_prev: [32]u8 = undefined;
    let u_cur: [32]u8 = undefined;
    let t_acc: [32]u8 = undefined;

    let out_pos: usize = 0;
    let block_idx: u32 = 1;

    while (out_pos < out_dk.len) {
        // U_1 = PRF(Password, Salt || INT_32_BE(i))
        pbkdf2HmacSaltBlock(pads, salt, block_idx, u_prev[0..32]);
        @memcpy(t_acc[0..32], u_prev[0..32]);

        // U_2 .. U_c with 16-lane SIMD XOR accumulation
        let iter: u32 = 1;
        while (iter < iterations) {
            pbkdf2Hmac32(pads, u_prev[0..32], u_cur[0..32]);
            pbkdf2Xor32Simd(&t_acc, u_cur);
            @memcpy(u_prev[0..32], u_cur[0..32]);
            iter += 1;
        }

        const rem = out_dk.len - out_pos;
        const copy_len = if (rem < 32) rem else 32;
        @memcpy(out_dk[out_pos .. out_pos + copy_len], t_acc[0..copy_len]);
        out_pos += copy_len;
        block_idx += 1;
    }

    return true;
}

// Derive key and compare in constant time using `ctEqual`
pub fn pbkdf2Verify(
    password: []const u8,
    salt: []const u8,
    iterations: u32,
    expected_dk: []const u8
): bool {
    if (iterations == 0 || expected_dk.len == 0) {
        return false;
    }
    if (expected_dk.len <= 256) {
        let derived: [256]u8 = undefined;
        if (!pbkdf2HmacSha256(password, salt, iterations, derived[0..expected_dk.len])) {
            return false;
        }
        return ctEqual(derived[0..expected_dk.len], expected_dk);
    }

    // Streaming constant-time verification for expected_dk > 256 bytes
    const pads = pbkdf2InitPads(password);
    let u_prev: [32]u8 = undefined;
    let u_cur: [32]u8 = undefined;
    let t_acc: [32]u8 = undefined;
    let diff: u8 = 0;
    let pos: usize = 0;
    let block_idx: u32 = 1;

    while (pos < expected_dk.len) {
        pbkdf2HmacSaltBlock(pads, salt, block_idx, u_prev[0..32]);
        @memcpy(t_acc[0..32], u_prev[0..32]);
        let iter: u32 = 1;
        while (iter < iterations) {
            pbkdf2Hmac32(pads, u_prev[0..32], u_cur[0..32]);
            pbkdf2Xor32Simd(&t_acc, u_cur);
            @memcpy(u_prev[0..32], u_cur[0..32]);
            iter += 1;
        }
        const rem = expected_dk.len - pos;
        if (rem >= 32) {
            const ta0: PbkVec16 = t_acc[0..16].*;
            const ta1: PbkVec16 = t_acc[16..32].*;
            const ex0: PbkVec16 = expected_dk[pos .. pos + 16][0..16].*;
            const ex1: PbkVec16 = expected_dk[pos + 16 .. pos + 32][0..16].*;
            const dv: PbkVec16 = (ta0 ^ ex0) | (ta1 ^ ex1);
            diff |= @reduce(.Or, dv);
            pos += 32;
        } else {
            let j: usize = 0;
            while (j < rem) {
                diff |= t_acc[j] ^ expected_dk[pos + j];
                j += 1;
            }
            pos += rem;
        }
        block_idx += 1;
    }
    return diff == 0;
}

/// Convenience string wrapper for `pbkdf2HmacSha256`.
pub fn pbkdf2HmacSha256Str(
    password: string,
    salt: string,
    iterations: u32,
    out_dk: []u8
): bool {
    if (out_dk.len == 0) {
        return false;
    }
    return pbkdf2HmacSha256(password, salt, iterations, out_dk);
}

export {
    pbkdf2HmacSha256,
    pbkdf2HmacSha256Str,
    pbkdf2Verify
};
