import {
    Sha256State, sha256Init, sha256Update, sha256Final, sha256, ctEqual
} from "../../cookie-signature/src/index.cp";
import { hexCharLower } from "cap:ascii";

pub const HMAC_SHA256_LEN: usize = 32;
pub const HMAC_BLOCK_SIZE: usize = 64;

pub fn encodeHex32(digest: []u8, out64: []u8): usize {
    if (digest.len < 32 || out64.len < 64) { return 0; }
    let i: usize = 0;
    while (i < 32) {
        let b = digest[i];
        out64[i * 2] = hexCharLower((b >> 4) & 15);
        out64[i * 2 + 1] = hexCharLower(b & 15);
        i = i + 1;
    }
    return 64;
}

pub struct HmacSha256 {
    pub inner: Sha256State,
    pub o_key_pad: [64]u8,

    pub static fn newFromSlice(key: []u8): HmacSha256 {
        let kblock: [64]u8 = [0; 64];
        if (key.len > HMAC_BLOCK_SIZE) {
            let hashed: [32]u8 = undefined;
            sha256(key, hashed[0..32]);
            let i: usize = 0;
            while (i < 32) {
                kblock[i] = hashed[i];
                i = i + 1;
            }
        } else {
            let i: usize = 0;
            while (i < key.len) {
                kblock[i] = key[i];
                i = i + 1;
            }
        }

        let ipad: [64]u8 = undefined;
        let opad: [64]u8 = undefined;
        let j: usize = 0;
        while (j < HMAC_BLOCK_SIZE) {
            ipad[j] = kblock[j] ^ 54; // 0x36
            opad[j] = kblock[j] ^ 92; // 0x5c
            j = j + 1;
        }

        let st = sha256Init();
        sha256Update(*st, ipad[0..64]);
        return HmacSha256 {
            inner: st,
            o_key_pad: opad,
        };
    }

    pub static fn newFromKey(key: string): HmacSha256 {
        let buf: [256]u8 = undefined;
        let n = if (key.len > 256) { 256 } else { key.len };
        let i: usize = 0;
        while (i < n) {
            buf[i] = key[i];
            i = i + 1;
        }
        return HmacSha256.newFromSlice(buf[0..n]);
    }

    pub fn update(*_, data: []u8) {
        sha256Update(*_.inner, data);
    }

    pub fn updateStr(*_, msg: string) {
        let chunk: [64]u8 = undefined;
        let pos: usize = 0;
        while (pos < msg.len) {
            let rem = msg.len - pos;
            let take = if (rem > 64) { 64 } else { rem };
            let i: usize = 0;
            while (i < take) {
                chunk[i] = msg[pos + i];
                i = i + 1;
            }
            sha256Update(*_.inner, chunk[0..take]);
            pos = pos + take;
        }
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
}

pub fn hmacSha256Hex(key: string, msg: string, out64: []u8): usize {
    let mac = HmacSha256.newFromKey(key);
    mac.updateStr(msg);
    let digest: [32]u8 = undefined;
    mac.finalize(digest[0..32]);
    return encodeHex32(digest[0..32], out64);
}
