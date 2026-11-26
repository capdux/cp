import { HmacSha256, HMAC_SHA256_LEN } from "../../hmac-crate/src/index.cp";

pub const PBKDF2_HASH_LEN: usize = 32;

pub fn pbkdf2HmacSha256(password: []u8, salt: []u8, iterations: u32, dk_out: []u8): bool {
    if (iterations == 0 || dk_out.len == 0) {
        return false;
    }
    let blocks = (dk_out.len + HMAC_SHA256_LEN - 1) / HMAC_SHA256_LEN;
    let out_pos: usize = 0;
    let blk: usize = 1;
    while (blk <= blocks) {
        let be: [4]u8 = [
            ((blk >> 24) & 255) as u8,
            ((blk >> 16) & 255) as u8,
            ((blk >> 8) & 255) as u8,
            (blk & 255) as u8,
        ];

        let u_cur: [32]u8 = [0; 32];
        let acc: [32]u8 = [0; 32];

        // U_1 = PRF(Password, Salt || INT_32_BE(i))
        let m1 = HmacSha256.newFromSlice(password);
        if (salt.len > 0) {
            m1.update(salt);
        }
        m1.update(be[0..4]);
        m1.finalize(u_cur[0..32]);

        let j: usize = 0;
        while (j < 32) {
            acc[j] = u_cur[j];
            j = j + 1;
        }

        // U_2 .. U_c
        let iter: u32 = 1;
        while (iter < iterations) {
            let m = HmacSha256.newFromSlice(password);
            m.update(u_cur[0..32]);
            m.finalize(u_cur[0..32]);
            let k: usize = 0;
            while (k < 32) {
                acc[k] = acc[k] ^ u_cur[k];
                k = k + 1;
            }
            iter = iter + 1;
        }

        let rem = dk_out.len - out_pos;
        let take = if (rem > 32) { 32 } else { rem };
        let t: usize = 0;
        while (t < take) {
            dk_out[out_pos + t] = acc[t];
            t = t + 1;
        }
        out_pos = out_pos + take;
        blk = blk + 1;
    }
    return true;
}

pub fn pbkdf2HmacSha256Str(password: string, salt: string, iterations: u32, dk_out: []u8): bool {
    let pw_buf: [256]u8 = [0; 256];
    let st_buf: [256]u8 = [0; 256];
    let pw_len = if (password.len > 256) { 256 } else { password.len };
    let st_len = if (salt.len > 256) { 256 } else { salt.len };
    let i: usize = 0;
    while (i < pw_len) { pw_buf[i] = password[i]; i = i + 1; }
    i = 0;
    while (i < st_len) { st_buf[i] = salt[i]; i = i + 1; }
    return pbkdf2HmacSha256(pw_buf[0..pw_len], st_buf[0..st_len], iterations, dk_out);
}
