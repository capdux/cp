// hmac — Zero-heap RFC 2104 streaming HMAC-SHA256 & HMAC-SHA512 with constant-time verify
// Clean-room MIT/Apache-2.0 parity with Rust `hmac` crate.
// Accelerated with 16-lane @Vector(16, u8) SIMD pad XOR & constant-time compare,
// comptime 256-entry hex encoding LUTs, and 256B streaming chunks.

const std = @import("std");
import {
    Sha256State, sha256Init, sha256Update, sha256Final, sha256, ctEqual
} from "../../cookie-signature/src/index.cp";
import { hexCharLower } from "cap:ascii";

type HmacVec16 = @Vector(16, u8);

fn hmacBuildHexHiLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    const digits = "0123456789abcdef";
    let lut: [256]u8 = [0; 256];
    let i: usize = 0;
    while (i < 256) {
        lut[i] = digits[(i >> 4) & 15];
        i += 1;
    }
    return lut;
}

fn hmacBuildHexLoLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    const digits = "0123456789abcdef";
    let lut: [256]u8 = [0; 256];
    let i: usize = 0;
    while (i < 256) {
        lut[i] = digits[i & 15];
        i += 1;
    }
    return lut;
}

const HMAC_HEX_HI_LUT: [256]u8 = hmacBuildHexHiLut();
const HMAC_HEX_LO_LUT: [256]u8 = hmacBuildHexLoLut();

fn hmacBuildPads64(kblock: [64]u8, ipad: []u8, opad: []u8) {
    const v_ipad: HmacVec16 = @splat(0x36 as u8);
    const v_opad: HmacVec16 = @splat(0x5c as u8);
    let off: usize = 0;
    while (off < 64) {
        const vk: HmacVec16 = kblock[off .. off + 16][0..16].*;
        const ip: [16]u8 = vk ^ v_ipad;
        const op: [16]u8 = vk ^ v_opad;
        @memcpy(ipad[off .. off + 16], ip[0..16]);
        @memcpy(opad[off .. off + 16], op[0..16]);
        off += 16;
    }
}

pub const HMAC_SHA256_LEN: usize = 32;
pub const HMAC_BLOCK_SIZE: usize = 64;
pub const HMAC_SHA512_LEN: usize = 64;
pub const HMAC_SHA512_BLOCK_SIZE: usize = 128;

/// Constant-time byte slice equality check resistant to timing side channels.
pub fn ctEqualBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let acc: HmacVec16 = @splat(0 as u8);
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va: HmacVec16 = a[i .. i + 16][0..16].*;
        const vb: HmacVec16 = b[i .. i + 16][0..16].*;
        acc = acc | (va ^ vb);
        i += 16;
    }
    const acc_arr: [16]u8 = acc;
    let diff: u8 = 0;
    let k: usize = 0;
    while (k < 16) {
        diff = diff | acc_arr[k];
        k += 1;
    }
    while (i < a.len) {
        diff = diff | (a[i] ^ b[i]);
        i += 1;
    }
    return diff == 0;
}

/// Lowercase hex encoding for a 32-byte digest into a 64-byte output buffer.
pub fn encodeHex32(digest: []u8, out64: []u8): usize {
    if (digest.len < 32 || out64.len < 64) { return 0; }
    let i: usize = 0;
    while (i < 32) {
        const b = digest[i] as usize;
        out64[i * 2] = HMAC_HEX_HI_LUT[b];
        out64[i * 2 + 1] = HMAC_HEX_LO_LUT[b];
        i += 1;
    }
    return 64;
}

/// Lowercase hex encoding for a 64-byte digest into a 128-byte output buffer.
pub fn encodeHex64(digest: []const u8, out128: []u8): usize {
    if (digest.len < 64 || out128.len < 128) { return 0; }
    let i: usize = 0;
    while (i < 64) {
        const b = digest[i] as usize;
        out128[i * 2] = HMAC_HEX_HI_LUT[b];
        out128[i * 2 + 1] = HMAC_HEX_LO_LUT[b];
        i += 1;
    }
    return 128;
}

/// Streaming RFC 2104 HMAC-SHA256 MAC state.
pub struct HmacSha256 {
    pub inner: Sha256State,
    pub o_key_pad: [64]u8,

    pub static fn newFromSlice(key: []u8): HmacSha256 {
        let kblock: [64]u8 = [0; 64];
        if (key.len > HMAC_BLOCK_SIZE) {
            let hashed: [32]u8 = undefined;
            sha256(key, hashed[0..32]);
            @memcpy(kblock[0..32], hashed[0..32]);
        } else if (key.len > 0) {
            @memcpy(kblock[0..key.len], key[0..key.len]);
        }

        let ipad: [64]u8 = undefined;
        let opad: [64]u8 = undefined;
        hmacBuildPads64(kblock, ipad[0..64], opad[0..64]);

        let st = sha256Init();
        sha256Update(*st, ipad[0..64]);
        return HmacSha256 {
            inner: st,
            o_key_pad: opad,
        };
    }

    pub static fn newFromConstSlice(key: []const u8): HmacSha256 {
        let kblock: [64]u8 = [0; 64];
        if (key.len > HMAC_BLOCK_SIZE) {
            let st_k = sha256Init();
            let chunk: [256]u8 = undefined;
            let pos: usize = 0;
            while (pos < key.len) {
                const rem = key.len - pos;
                const take = if (rem > 256) 256 else rem;
                @memcpy(chunk[0..take], key[pos..pos + take]);
                sha256Update(*st_k, chunk[0..take]);
                pos += take;
            }
            sha256Final(*st_k, kblock[0..32]);
        } else if (key.len > 0) {
            @memcpy(kblock[0..key.len], key[0..key.len]);
        }

        let ipad: [64]u8 = undefined;
        let opad: [64]u8 = undefined;
        hmacBuildPads64(kblock, ipad[0..64], opad[0..64]);

        let st = sha256Init();
        sha256Update(*st, ipad[0..64]);
        return HmacSha256 {
            inner: st,
            o_key_pad: opad,
        };
    }

    pub static fn newFromKey(key: string): HmacSha256 {
        return HmacSha256.newFromConstSlice(key);
    }

    pub fn update(*_, data: []u8) {
        sha256Update(*_.inner, data);
    }

    pub fn updateConst(*_, data: []const u8) {
        let chunk: [256]u8 = undefined;
        let pos: usize = 0;
        while (pos < data.len) {
            const rem = data.len - pos;
            const take = if (rem > 256) 256 else rem;
            @memcpy(chunk[0..take], data[pos..pos + take]);
            sha256Update(*_.inner, chunk[0..take]);
            pos += take;
        }
    }

    pub fn updateStr(*_, msg: string) {
        _.updateConst(msg);
    }

    pub fn finalize(*_, out32: []u8) {
        let inner_hash: [32]u8 = undefined;
        sha256Final(*_.inner, inner_hash[0..32]);
        let outer = sha256Init();
        sha256Update(*outer, _.o_key_pad[0..64]);
        sha256Update(*outer, inner_hash[0..32]);
        sha256Final(*outer, out32);
    }

    pub fn verifySlice(*_, expected: []u8): bool {
        if (expected.len != HMAC_SHA256_LEN) { return false; }
        let actual: [32]u8 = undefined;
        _.finalize(actual[0..32]);
        return ctEqual(actual[0..32], expected);
    }

    pub fn verifyConstSlice(*_, expected: []const u8): bool {
        if (expected.len != HMAC_SHA256_LEN) { return false; }
        let actual: [32]u8 = undefined;
        _.finalize(actual[0..32]);
        return ctEqualBytes(actual[0..32], expected);
    }
}

/// Streaming RFC 2104 HMAC-SHA512 MAC state (zero-heap, constant-time verification).
pub struct HmacSha512 {
    state: std.crypto.auth.hmac.sha2.HmacSha512,

    pub static fn newFromSlice(key: []const u8): HmacSha512 {
        return HmacSha512 {
            state: std.crypto.auth.hmac.sha2.HmacSha512.init(key),
        };
    }

    pub static fn newFromKey(key: string): HmacSha512 {
        return HmacSha512.newFromSlice(key);
    }

    pub fn update(*_, data: []const u8) {
        _.state.update(data);
    }

    pub fn updateStr(*_, msg: string) {
        _.state.update(msg);
    }

    pub fn finalize(*_, out64: []u8) {
        if (out64.len < HMAC_SHA512_LEN) { return; }
        let mac: [64]u8 = undefined;
        _.state.final(&mac);
        @memcpy(out64[0..64], mac[0..64]);
    }

    pub fn verifySlice(*_, expected: []const u8): bool {
        if (expected.len != HMAC_SHA512_LEN) { return false; }
        let actual: [64]u8 = undefined;
        _.state.final(&actual);
        return ctEqualBytes(actual[0..64], expected);
    }
}

/// One-shot HMAC-SHA256 raw 32-byte digest into `out32`.
pub fn hmacSha256Bytes(key: []const u8, msg: []const u8, out32: []u8) {
    let mac = HmacSha256.newFromConstSlice(key);
    mac.updateConst(msg);
    mac.finalize(out32);
}

/// One-shot HMAC-SHA256 lowercase hex digest into `out64`.
pub fn hmacSha256Hex(key: string, msg: string, out64: []u8): usize {
    let mac = HmacSha256.newFromKey(key);
    mac.updateStr(msg);
    let digest: [32]u8 = undefined;
    mac.finalize(digest[0..32]);
    return encodeHex32(digest[0..32], out64);
}

/// One-shot HMAC-SHA512 raw 64-byte digest into `out64`.
pub fn hmacSha512(key: []const u8, msg: []const u8, out64: []u8) {
    let mac = HmacSha512.newFromSlice(key);
    mac.update(msg);
    mac.finalize(out64);
}

/// One-shot HMAC-SHA512 lowercase hex digest into `out128`.
pub fn hmacSha512Hex(key: string, msg: string, out128: []u8): usize {
    let mac = HmacSha512.newFromKey(key);
    mac.updateStr(msg);
    let digest: [64]u8 = undefined;
    mac.finalize(digest[0..64]);
    return encodeHex64(digest[0..64], out128);
}

export {
    HMAC_SHA256_LEN,
    HMAC_BLOCK_SIZE,
    HMAC_SHA512_LEN,
    HMAC_SHA512_BLOCK_SIZE,
    ctEqualBytes,
    encodeHex32,
    encodeHex64,
    HmacSha256,
    HmacSha512,
    hmacSha256Bytes,
    hmacSha256Hex,
    hmacSha512,
    hmacSha512Hex,
};
