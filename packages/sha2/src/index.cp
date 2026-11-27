import {
    Sha256State, sha256Init, sha256Update, sha256Final
} from "../../cookie-signature/src/index.cp";

pub const SHA256_DIGEST_LEN: usize = 32;
pub const SHA224_DIGEST_LEN: usize = 28;

fn sh2HexDigit(n: u8): u8 {
    if (n < 10) { return 48 + n; }
    return 87 + n;
}

fn sh2EncodeHex(data: []u8, out: []u8): usize {
    let n = data.len;
    if (out.len < n * 2) { return 0; }
    let i: usize = 0;
    while (i < n) {
        out[i * 2] = sh2HexDigit((data[i] >> 4) & 15);
        out[i * 2 + 1] = sh2HexDigit(data[i] & 15);
        i = i + 1;
    }
    return n * 2;
}

pub struct Sha256 {
    pub st: Sha256State,

    pub static fn new(): Sha256 {
        return Sha256 { st: sha256Init() };
    }

    pub fn update(*_, data: []u8) {
        sha256Update(*_.st, data);
    }

    pub fn updateStr(*_, s: string) {
        let chunk: [64]u8 = [0; 64];
        let pos: usize = 0;
        while (pos < s.len) {
            let rem = s.len - pos;
            let take = if (rem > 64) { 64 } else { rem };
            let i: usize = 0;
            while (i < take) {
                chunk[i] = s[pos + i];
                i = i + 1;
            }
            sha256Update(*_.st, chunk[0..take]);
            pos = pos + take;
        }
    }

    pub fn finalize(*_, out32: []u8) {
        sha256Final(*_.st, out32);
    }

    pub fn finalizeHex(*_, out64: []u8): usize {
        let dig: [32]u8 = [0; 32];
        sha256Final(*_.st, dig[0..32]);
        return sh2EncodeHex(dig[0..32], out64);
    }

    pub fn reset(*_) {
        _.st = sha256Init();
    }
}

pub struct Sha224 {
    pub st: Sha256State,

    pub static fn new(): Sha224 {
        let s = sha256Init();
        // FIPS 180-4 SHA-224 initial hash values
        s.h[0] = 3238371032; // 0xc1059ed8
        s.h[1] = 914150663;  // 0x367cd507
        s.h[2] = 812702999;  // 0x3070dd17
        s.h[3] = 4144912697; // 0xf70e5939
        s.h[4] = 4290775857; // 0xffc00b31
        s.h[5] = 1750603025; // 0x68581511
        s.h[6] = 1694076839; // 0x64f98fa7
        s.h[7] = 3204075428; // 0xbefa4fa4
        return Sha224 { st: s };
    }

    pub fn update(*_, data: []u8) {
        sha256Update(*_.st, data);
    }

    pub fn updateStr(*_, s: string) {
        let chunk: [64]u8 = [0; 64];
        let pos: usize = 0;
        while (pos < s.len) {
            let rem = s.len - pos;
            let take = if (rem > 64) { 64 } else { rem };
            let i: usize = 0;
            while (i < take) {
                chunk[i] = s[pos + i];
                i = i + 1;
            }
            sha256Update(*_.st, chunk[0..take]);
            pos = pos + take;
        }
    }

    pub fn finalize(*_, out28: []u8) {
        let dig32: [32]u8 = [0; 32];
        sha256Final(*_.st, dig32[0..32]);
        let i: usize = 0;
        while (i < 28 && i < out28.len) {
            out28[i] = dig32[i];
            i = i + 1;
        }
    }

    pub fn finalizeHex(*_, out56: []u8): usize {
        let dig28: [28]u8 = [0; 28];
        _.finalize(dig28[0..28]);
        return sh2EncodeHex(dig28[0..28], out56);
    }
}

pub fn sha256Hex(s: string, out64: []u8): usize {
    let h = Sha256.new();
    h.updateStr(s);
    return h.finalizeHex(out64);
}

pub fn sha224Hex(s: string, out56: []u8): usize {
    let h = Sha224.new();
    h.updateStr(s);
    return h.finalizeHex(out56);
}
