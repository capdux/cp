// @cap/salsa20 — Zero-heap Salsa20/20 stream cipher in pure CP
// Clean-room MIT implementation of crates.io `salsa20` (#917, 113.8M downloads).

fn s20Rotl(v: u32, shift: u5): u32 {
    const rshift: u5 = (32 - (shift as u32)) as u5;
    return (v << shift) | (v >> rshift);
}

fn s20LoadLe32(src: []const u8, off: usize): u32 {
    return (src[off] as u32)
        | ((src[off + 1] as u32) << 8)
        | ((src[off + 2] as u32) << 16)
        | ((src[off + 3] as u32) << 24);
}

fn s20StoreLe32(v: u32, dst: []u8, off: usize) {
    dst[off] = (v & 0xFF) as u8;
    dst[off + 1] = ((v >> 8) & 0xFF) as u8;
    dst[off + 2] = ((v >> 16) & 0xFF) as u8;
    dst[off + 3] = ((v >> 24) & 0xFF) as u8;
}

/// Generates one 64-byte Salsa20/20 keystream block for a 32-byte key, 8-byte nonce, and 64-bit block counter.
pub fn salsa20Block(key: []const u8, nonce: []const u8, counter: u64, out: []u8) {
    if (key.len < 32 || nonce.len < 8 || out.len < 64) {
        return;
    }
    // "expand 32-byte k" in little-endian u32
    let st: [16]u32 = [
        0x61707865,
        s20LoadLe32(key, 0),
        s20LoadLe32(key, 4),
        s20LoadLe32(key, 8),
        s20LoadLe32(key, 12),
        0x3320646E,
        s20LoadLe32(nonce, 0),
        s20LoadLe32(nonce, 4),
        (counter & 0xFFFFFFFF) as u32,
        ((counter >> 32) & 0xFFFFFFFF) as u32,
        0x79622D32,
        s20LoadLe32(key, 16),
        s20LoadLe32(key, 20),
        s20LoadLe32(key, 24),
        s20LoadLe32(key, 28),
        0x6B206574,
    ];

    let x: [16]u32 = st;
    let round: usize = 0;
    while (round < 10) {
        // Column rounds
        x[4] = x[4] ^ s20Rotl(x[0] +% x[12], 7);
        x[8] = x[8] ^ s20Rotl(x[4] +% x[0], 9);
        x[12] = x[12] ^ s20Rotl(x[8] +% x[4], 13);
        x[0] = x[0] ^ s20Rotl(x[12] +% x[8], 18);

        x[9] = x[9] ^ s20Rotl(x[5] +% x[1], 7);
        x[13] = x[13] ^ s20Rotl(x[9] +% x[5], 9);
        x[1] = x[1] ^ s20Rotl(x[13] +% x[9], 13);
        x[5] = x[5] ^ s20Rotl(x[1] +% x[13], 18);

        x[14] = x[14] ^ s20Rotl(x[10] +% x[6], 7);
        x[2] = x[2] ^ s20Rotl(x[14] +% x[10], 9);
        x[6] = x[6] ^ s20Rotl(x[2] +% x[14], 13);
        x[10] = x[10] ^ s20Rotl(x[6] +% x[2], 18);

        x[3] = x[3] ^ s20Rotl(x[15] +% x[11], 7);
        x[7] = x[7] ^ s20Rotl(x[3] +% x[15], 9);
        x[11] = x[11] ^ s20Rotl(x[7] +% x[3], 13);
        x[15] = x[15] ^ s20Rotl(x[11] +% x[7], 18);

        // Row rounds
        x[1] = x[1] ^ s20Rotl(x[0] +% x[3], 7);
        x[2] = x[2] ^ s20Rotl(x[1] +% x[0], 9);
        x[3] = x[3] ^ s20Rotl(x[2] +% x[1], 13);
        x[0] = x[0] ^ s20Rotl(x[3] +% x[2], 18);

        x[6] = x[6] ^ s20Rotl(x[5] +% x[4], 7);
        x[7] = x[7] ^ s20Rotl(x[6] +% x[5], 9);
        x[4] = x[4] ^ s20Rotl(x[7] +% x[6], 13);
        x[5] = x[5] ^ s20Rotl(x[4] +% x[7], 18);

        x[11] = x[11] ^ s20Rotl(x[10] +% x[9], 7);
        x[8] = x[8] ^ s20Rotl(x[11] +% x[10], 9);
        x[9] = x[9] ^ s20Rotl(x[8] +% x[11], 13);
        x[10] = x[10] ^ s20Rotl(x[9] +% x[8], 18);

        x[12] = x[12] ^ s20Rotl(x[15] +% x[14], 7);
        x[13] = x[13] ^ s20Rotl(x[12] +% x[15], 9);
        x[14] = x[14] ^ s20Rotl(x[13] +% x[12], 13);
        x[15] = x[15] ^ s20Rotl(x[14] +% x[13], 18);

        round += 1;
    }

    let i: usize = 0;
    while (i < 16) {
        s20StoreLe32(x[i] +% st[i], out, i * 4);
        i += 1;
    }
}

/// Encrypts or decrypts `input` into `out` using Salsa20/20 XOR keystream starting at `initial_counter`.
/// Returns bytes written.
pub fn salsa20Apply(
    key: []const u8,
    nonce: []const u8,
    initial_counter: u64,
    input: []const u8,
    out: []u8,
): usize {
    if (key.len < 32 || nonce.len < 8) {
        return 0;
    }
    let n = input.len;
    if (n > out.len) { n = out.len; }
    let block: [64]u8 = undefined;
    let ctr = initial_counter;
    let pos: usize = 0;
    while (pos < n) {
        salsa20Block(key, nonce, ctr, block[0..64]);
        let j: usize = 0;
        while (j < 64 && pos < n) {
            out[pos] = input[pos] ^ block[j];
            pos += 1;
            j += 1;
        }
        ctr += 1;
    }
    return n;
}

export {
    salsa20Block,
    salsa20Apply,
};
