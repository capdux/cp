// @cap/pbkdf2-sha256 — Pure CP zero-allocation RFC 2898 / RFC 8018 PBKDF2-HMAC-SHA256
// Parity with RustCrypto `pbkdf2` & npm `pbkdf2`.
// Reuses SHA-256 & constant-time `ctEqual` from `@cap/cookie-signature`.

import {
    Sha256State,
    sha256Init,
    sha256Update,
    sha256Final,
    sha256,
    ctEqual
} from "../../cookie-signature/src/index.cp";

struct Pbkdf2Pads {
    inner_base: Sha256State,
    outer_base: Sha256State,
}

fn pbkdf2InitPads(password: []const u8): Pbkdf2Pads {
    let key_blk: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 64) {
        key_blk[i] = 0;
        i = i + 1;
    }

    if (password.len > 64) {
        let kh: [32]u8 = undefined;
        sha256(password, kh[0..32]);
        i = 0;
        while (i < 32) {
            key_blk[i] = kh[i];
            i = i + 1;
        }
    } else {
        i = 0;
        while (i < password.len) {
            key_blk[i] = password[i];
            i = i + 1;
        }
    }

    let ipad: [64]u8 = undefined;
    let opad: [64]u8 = undefined;
    i = 0;
    while (i < 64) {
        ipad[i] = key_blk[i] ^ 0x36;
        opad[i] = key_blk[i] ^ 0x5C;
        i = i + 1;
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
    let idx_be: [4]u8 = [
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

    let pads = pbkdf2InitPads(password);
    let u_prev: [32]u8 = undefined;
    let u_cur: [32]u8 = undefined;
    let t_acc: [32]u8 = undefined;

    let out_pos: usize = 0;
    let block_idx: u32 = 1;

    while (out_pos < out_dk.len) {
        // U_1 = PRF(Password, Salt || INT_32_BE(i))
        pbkdf2HmacSaltBlock(pads, salt, block_idx, u_prev[0..32]);
        let j: usize = 0;
        while (j < 32) {
            t_acc[j] = u_prev[j];
            j = j + 1;
        }

        // U_2 .. U_c
        let iter: u32 = 1;
        while (iter < iterations) {
            pbkdf2Hmac32(pads, u_prev[0..32], u_cur[0..32]);
            j = 0;
            while (j < 32) {
                t_acc[j] = t_acc[j] ^ u_cur[j];
                u_prev[j] = u_cur[j];
                j = j + 1;
            }
            iter = iter + 1;
        }

        let rem = out_dk.len - out_pos;
        let copy_len: usize = 32;
        if (rem < copy_len) {
            copy_len = rem;
        }
        j = 0;
        while (j < copy_len) {
            out_dk[out_pos + j] = t_acc[j];
            j = j + 1;
        }
        out_pos = out_pos + copy_len;
        block_idx = block_idx + 1;
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
    let pads = pbkdf2InitPads(password);
    let u_prev: [32]u8 = undefined;
    let u_cur: [32]u8 = undefined;
    let t_acc: [32]u8 = undefined;
    let diff: u8 = 0;
    let pos: usize = 0;
    let block_idx: u32 = 1;

    while (pos < expected_dk.len) {
        pbkdf2HmacSaltBlock(pads, salt, block_idx, u_prev[0..32]);
        let j: usize = 0;
        while (j < 32) {
            t_acc[j] = u_prev[j];
            j = j + 1;
        }
        let iter: u32 = 1;
        while (iter < iterations) {
            pbkdf2Hmac32(pads, u_prev[0..32], u_cur[0..32]);
            j = 0;
            while (j < 32) {
                t_acc[j] = t_acc[j] ^ u_cur[j];
                u_prev[j] = u_cur[j];
                j = j + 1;
            }
            iter = iter + 1;
        }
        let rem = expected_dk.len - pos;
        let cmp_len: usize = 32;
        if (rem < cmp_len) {
            cmp_len = rem;
        }
        j = 0;
        while (j < cmp_len) {
            diff = diff | (t_acc[j] ^ expected_dk[pos + j]);
            j = j + 1;
        }
        pos = pos + cmp_len;
        block_idx = block_idx + 1;
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
    return pbkdf2HmacSha256(password, salt, iterations, out_dk);
}

export {
    pbkdf2HmacSha256,
    pbkdf2HmacSha256Str,
    pbkdf2Verify
};
