// @cap/chacha20-crate — Pure CP zero-allocation RFC 8439 ChaCha20 stream cipher
// Clean-room MIT parity with Rust crates.io `chacha20` v0.9.

pub const CHACHA20_KEY_LEN: usize = 32;
pub const CHACHA20_NONCE_LEN: usize = 12;
pub const CHACHA20_BLOCK_LEN: usize = 64;

fn cc20Add32(a: u32, b: u32): u32 {
    return (((a as u64) + (b as u64)) & (0xFFFFFFFF as u64)) as u32;
}

fn cc20Rotl16(x: u32): u32 {
    return (x << 16) | (x >> 16);
}

fn cc20Rotl12(x: u32): u32 {
    return (x << 12) | (x >> 20);
}

fn cc20Rotl8(x: u32): u32 {
    return (x << 8) | (x >> 24);
}

fn cc20Rotl7(x: u32): u32 {
    return (x << 7) | (x >> 25);
}

fn cc20ReadLe32(b: []u8, off: usize): u32 {
    let b0: u32 = b[off] as u32;
    let b1: u32 = (b[off + (1 as usize)] as u32) << 8;
    let b2: u32 = (b[off + (2 as usize)] as u32) << 16;
    let b3: u32 = (b[off + (3 as usize)] as u32) << 24;
    return b0 | b1 | b2 | b3;
}

fn cc20WriteLe32(out: []u8, off: usize, v: u32) {
    out[off] = (v & (0xFF as u32)) as u8;
    out[off + (1 as usize)] = ((v >> 8) & (0xFF as u32)) as u8;
    out[off + (2 as usize)] = ((v >> 16) & (0xFF as u32)) as u8;
    out[off + (3 as usize)] = ((v >> 24) & (0xFF as u32)) as u8;
}

fn cc20QuarterRound(st: []u32, ai: usize, bi: usize, ci: usize, di: usize) {
    let a: u32 = st[ai];
    let b: u32 = st[bi];
    let c: u32 = st[ci];
    let d: u32 = st[di];

    a = cc20Add32(a, b);
    d = cc20Rotl16(d ^ a);
    c = cc20Add32(c, d);
    b = cc20Rotl12(b ^ c);
    a = cc20Add32(a, b);
    d = cc20Rotl8(d ^ a);
    c = cc20Add32(c, d);
    b = cc20Rotl7(b ^ c);

    st[ai] = a;
    st[bi] = b;
    st[ci] = c;
    st[di] = d;
}

pub fn chacha20Block(key: []u8, counter: u32, nonce: []u8, out64: []u8): bool {
    if (key.len != CHACHA20_KEY_LEN || nonce.len != CHACHA20_NONCE_LEN || out64.len < CHACHA20_BLOCK_LEN) {
        return false;
    }

    let initial: [16]u32 = undefined;
    initial[0] = 0x61707865 as u32;
    initial[1] = 0x3320646e as u32;
    initial[2] = 0x79622d32 as u32;
    initial[3] = 0x6b206574 as u32;

    let i: usize = 0;
    while (i < (8 as usize)) {
        initial[(4 as usize) + i] = cc20ReadLe32(key, i * (4 as usize));
        i += 1;
    }

    initial[12] = counter;
    initial[13] = cc20ReadLe32(nonce, 0 as usize);
    initial[14] = cc20ReadLe32(nonce, 4 as usize);
    initial[15] = cc20ReadLe32(nonce, 8 as usize);

    let working: [16]u32 = undefined;
    i = 0;
    while (i < (16 as usize)) {
        working[i] = initial[i];
        i += 1;
    }

    let round: usize = 0;
    while (round < (10 as usize)) {
        // Column rounds
        cc20QuarterRound(working[0..16], 0 as usize, 4 as usize, 8 as usize, 12 as usize);
        cc20QuarterRound(working[0..16], 1 as usize, 5 as usize, 9 as usize, 13 as usize);
        cc20QuarterRound(working[0..16], 2 as usize, 6 as usize, 10 as usize, 14 as usize);
        cc20QuarterRound(working[0..16], 3 as usize, 7 as usize, 11 as usize, 15 as usize);
        // Diagonal rounds
        cc20QuarterRound(working[0..16], 0 as usize, 5 as usize, 10 as usize, 15 as usize);
        cc20QuarterRound(working[0..16], 1 as usize, 6 as usize, 11 as usize, 12 as usize);
        cc20QuarterRound(working[0..16], 2 as usize, 7 as usize, 8 as usize, 13 as usize);
        cc20QuarterRound(working[0..16], 3 as usize, 4 as usize, 9 as usize, 14 as usize);
        round += 1;
    }

    i = 0;
    while (i < (16 as usize)) {
        let word: u32 = cc20Add32(working[i], initial[i]);
        cc20WriteLe32(out64, i * (4 as usize), word);
        i += 1;
    }
    return true;
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
        let i: usize = 0;
        while (i < CHACHA20_KEY_LEN) {
            k_arr[i] = key[i];
            i += 1;
        }
        let n_arr: [12]u8 = undefined;
        i = 0;
        while (i < CHACHA20_NONCE_LEN) {
            n_arr[i] = nonce[i];
            i += 1;
        }
        let b_arr: [64]u8 = undefined;
        i = 0;
        while (i < CHACHA20_BLOCK_LEN) {
            b_arr[i] = 0;
            i += 1;
        }
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
        while (i < data.len) {
            if (_.block_pos == CHACHA20_BLOCK_LEN) {
                * = chacha20Block(_.key[0..32], _.counter, _.nonce[0..12], _.block_buf[0..64]);
                _.counter = cc20Add32(_.counter, 1 as u32);
                _.block_pos = 0;
            }
            data[i] = data[i] ^ _.block_buf[_.block_pos];
            _.block_pos += 1;
            i += 1;
        }
    }
}
