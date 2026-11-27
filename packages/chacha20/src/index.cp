// @cap/chacha20-crate — Pure CP zero-allocation RFC 8439 ChaCha20 stream cipher
// Clean-room MIT parity with Rust crates.io `chacha20` v0.9.
// Accelerated with 4-lane @Vector(4, u32) ChaCha20 quarter-round SIMD, 16-lane @Vector(16, u8) keystream XOR, @memcpy, and undefined stack buffers.

pub const CHACHA20_KEY_LEN: usize = 32;
pub const CHACHA20_NONCE_LEN: usize = 12;
pub const CHACHA20_BLOCK_LEN: usize = 64;

type Cc20Vec4 = @Vector(4, u32);
type Cc20Vec4Shift = @Vector(4, u5);
type Cc20Vec16 = @Vector(16, u8);

fn cc20Add32(a: u32, b: u32): u32 {
    return a +% b;
}

fn cc20ReadLe32(b: []u8, off: usize): u32 {
    const b0: u32 = b[off] as u32;
    const b1: u32 = (b[off + 1] as u32) << 8;
    const b2: u32 = (b[off + 2] as u32) << 16;
    const b3: u32 = (b[off + 3] as u32) << 24;
    return b0 | b1 | b2 | b3;
}

fn cc20WriteLe32(out: []u8, off: usize, v: u32) {
    out[off] = (v & 0xFF) as u8;
    out[off + 1] = ((v >> 8) & 0xFF) as u8;
    out[off + 2] = ((v >> 16) & 0xFF) as u8;
    out[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn cc20Rotl4(v: Cc20Vec4, comptime n: u5): Cc20Vec4 {
    const s_l: Cc20Vec4Shift = @splat(n);
    const s_r: Cc20Vec4Shift = @splat((32 - (n as u32)) as u5);
    return (v << s_l) | (v >> s_r);
}

fn cc20RotRow1(v: Cc20Vec4): Cc20Vec4 {
    const a: [4]u32 = v;
    const r: [4]u32 = [a[1], a[2], a[3], a[0]];
    return r;
}

fn cc20RotRow2(v: Cc20Vec4): Cc20Vec4 {
    const a: [4]u32 = v;
    const r: [4]u32 = [a[2], a[3], a[0], a[1]];
    return r;
}

fn cc20RotRow3(v: Cc20Vec4): Cc20Vec4 {
    const a: [4]u32 = v;
    const r: [4]u32 = [a[3], a[0], a[1], a[2]];
    return r;
}

fn cc20WriteRowLe(out64: []u8, off: usize, row: Cc20Vec4) {
    const a: [4]u32 = row;
    cc20WriteLe32(out64, off, a[0]);
    cc20WriteLe32(out64, off + 4, a[1]);
    cc20WriteLe32(out64, off + 8, a[2]);
    cc20WriteLe32(out64, off + 12, a[3]);
}

pub fn chacha20Block(key: []u8, counter: u32, nonce: []u8, out64: []u8): bool {
    if (key.len != CHACHA20_KEY_LEN || nonce.len != CHACHA20_NONCE_LEN || out64.len < CHACHA20_BLOCK_LEN) {
        return false;
    }

    const init0_arr: [4]u32 = [0x61707865 as u32, 0x3320646e as u32, 0x79622d32 as u32, 0x6b206574 as u32];
    const init1_arr: [4]u32 = [
        cc20ReadLe32(key, 0),
        cc20ReadLe32(key, 4),
        cc20ReadLe32(key, 8),
        cc20ReadLe32(key, 12),
    ];
    const init2_arr: [4]u32 = [
        cc20ReadLe32(key, 16),
        cc20ReadLe32(key, 20),
        cc20ReadLe32(key, 24),
        cc20ReadLe32(key, 28),
    ];
    const init3_arr: [4]u32 = [
        counter,
        cc20ReadLe32(nonce, 0),
        cc20ReadLe32(nonce, 4),
        cc20ReadLe32(nonce, 8),
    ];

    const init0: Cc20Vec4 = init0_arr;
    const init1: Cc20Vec4 = init1_arr;
    const init2: Cc20Vec4 = init2_arr;
    const init3: Cc20Vec4 = init3_arr;

    let r0: Cc20Vec4 = init0;
    let r1: Cc20Vec4 = init1;
    let r2: Cc20Vec4 = init2;
    let r3: Cc20Vec4 = init3;

    const s16: Cc20Vec4Shift = @splat(16 as u5);
    const s12: Cc20Vec4Shift = @splat(12 as u5);
    const s20: Cc20Vec4Shift = @splat(20 as u5);
    const s8: Cc20Vec4Shift = @splat(8 as u5);
    const s24: Cc20Vec4Shift = @splat(24 as u5);
    const s7: Cc20Vec4Shift = @splat(7 as u5);
    const s25: Cc20Vec4Shift = @splat(25 as u5);

    let round: usize = 0;
    while (round < 10) {
        // 4-lane Column round
        r0 = r0 +% r1;
        r3 = r3 ^ r0;
        r3 = (r3 << s16) | (r3 >> s16);
        r2 = r2 +% r3;
        r1 = r1 ^ r2;
        r1 = (r1 << s12) | (r1 >> s20);
        r0 = r0 +% r1;
        r3 = r3 ^ r0;
        r3 = (r3 << s8) | (r3 >> s24);
        r2 = r2 +% r3;
        r1 = r1 ^ r2;
        r1 = (r1 << s7) | (r1 >> s25);

        // Rotate rows for 4-lane Diagonal round
        r1 = cc20RotRow1(r1);
        r2 = cc20RotRow2(r2);
        r3 = cc20RotRow3(r3);

        // 4-lane Diagonal round
        r0 = r0 +% r1;
        r3 = r3 ^ r0;
        r3 = (r3 << s16) | (r3 >> s16);
        r2 = r2 +% r3;
        r1 = r1 ^ r2;
        r1 = (r1 << s12) | (r1 >> s20);
        r0 = r0 +% r1;
        r3 = r3 ^ r0;
        r3 = (r3 << s8) | (r3 >> s24);
        r2 = r2 +% r3;
        r1 = r1 ^ r2;
        r1 = (r1 << s7) | (r1 >> s25);

        // Un-rotate rows back to column alignment
        r1 = cc20RotRow3(r1);
        r2 = cc20RotRow2(r2);
        r3 = cc20RotRow1(r3);

        round += 1;
    }

    cc20WriteRowLe(out64, 0, r0 +% init0);
    cc20WriteRowLe(out64, 16, r1 +% init1);
    cc20WriteRowLe(out64, 32, r2 +% init2);
    cc20WriteRowLe(out64, 48, r3 +% init3);
    return true;
}

fn cc20Xor16(dst: []u8, ks: []const u8) {
    const vd: Cc20Vec16 = dst[0..16].*;
    const vk: Cc20Vec16 = ks[0..16].*;
    const out_arr: [16]u8 = vd ^ vk;
    @memcpy(dst[0..16], out_arr[0..16]);
}

pub struct ChaCha20Cipher {
    key: [32]u8,
    nonce: [12]u8,
    counter: u32,
    block_buf: [64]u8,
    block_pos: usize,

    pub static fn new(key: []u8, nonce: []u8): ChaCha20Cipher? {
        return ChaCha20Cipher.newWithCounter(key, nonce, 0 as u32);
    }

    pub static fn newWithCounter(key: []u8, nonce: []u8, counter: u32): ChaCha20Cipher? {
        if (key.len != CHACHA20_KEY_LEN || nonce.len != CHACHA20_NONCE_LEN) {
            return null;
        }
        let k_arr: [32]u8 = undefined;
        @memcpy(k_arr[0..32], key[0..32]);
        let n_arr: [12]u8 = undefined;
        @memcpy(n_arr[0..12], nonce[0..12]);
        let b_arr: [64]u8 = undefined;
        return ChaCha20Cipher {
            key: k_arr,
            nonce: n_arr,
            counter: counter,
            block_buf: b_arr,
            block_pos: CHACHA20_BLOCK_LEN,
        };
    }

    pub fn seek(*_, block_counter: u32) {
        _.counter = block_counter;
        _.block_pos = CHACHA20_BLOCK_LEN;
    }

    pub fn applyKeystream(*_, data: []u8) {
        let i: usize = 0;
        const n = data.len;

        // Drain any remaining buffered keystream bytes first
        if (_.block_pos < CHACHA20_BLOCK_LEN && i < n) {
            const avail = CHACHA20_BLOCK_LEN - _.block_pos;
            const take = if (n - i < avail) n - i else avail;
            let j: usize = 0;
            while (j + 16 <= take) {
                cc20Xor16(data[i + j .. i + j + 16], _.block_buf[_.block_pos + j .. _.block_pos + j + 16]);
                j += 16;
            }
            while (j < take) {
                data[i + j] = data[i + j] ^ _.block_buf[_.block_pos + j];
                j += 1;
            }
            _.block_pos += take;
            i += take;
        }

        // Full 64-byte blocks via 4x 16-lane @Vector(16, u8) SIMD XOR
        while (i + CHACHA20_BLOCK_LEN <= n) {
            * = chacha20Block(_.key[0..32], _.counter, _.nonce[0..12], _.block_buf[0..64]);
            _.counter = cc20Add32(_.counter, 1 as u32);
            cc20Xor16(data[i .. i + 16], _.block_buf[0..16]);
            cc20Xor16(data[i + 16 .. i + 32], _.block_buf[16..32]);
            cc20Xor16(data[i + 32 .. i + 48], _.block_buf[32..48]);
            cc20Xor16(data[i + 48 .. i + 64], _.block_buf[48..64]);
            i += CHACHA20_BLOCK_LEN;
        }

        // Trailing partial block (< 64 bytes)
        if (i < n) {
            * = chacha20Block(_.key[0..32], _.counter, _.nonce[0..12], _.block_buf[0..64]);
            _.counter = cc20Add32(_.counter, 1 as u32);
            const rem = n - i;
            let j: usize = 0;
            while (j + 16 <= rem) {
                cc20Xor16(data[i + j .. i + j + 16], _.block_buf[j .. j + 16]);
                j += 16;
            }
            while (j < rem) {
                data[i + j] = data[i + j] ^ _.block_buf[j];
                j += 1;
            }
            _.block_pos = rem;
        }
    }
}
