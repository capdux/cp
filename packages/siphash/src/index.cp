// packages/@cap/siphash-pure/src/index.cp
// Zero-allocation SipHash-2-4 & SipHash-1-3 64-bit keyed PRF (Aumasson & Bernstein).
// MIT clean-room pure CP implementation.

fn shU64(hi: u32, lo: u32): u64 {
    return ((hi as u64) << (32 as u6)) | (lo as u64);
}

/// Wrapping 64-bit addition using 32-bit split (Debug-mode overflow safe).
fn shAdd64(a: u64, b: u64): u64 {
    const mask32: u64 = 0xFFFFFFFF as u64;
    const loSum = (a & mask32) + (b & mask32);
    const carry = (loSum >> (32 as u6)) & (1 as u64);
    const hiSum = ((a >> (32 as u6)) & mask32) + ((b >> (32 as u6)) & mask32) + carry;
    return ((hiSum & mask32) << (32 as u6)) | (loSum & mask32);
}

fn shRotl64(x: u64, bits: u32): u64 {
    const s1 = bits as u6;
    const s2 = ((64 as u32) - bits) as u6;
    return (x << s1) | (x >> s2);
}

fn shReadLe64(data: []const u8, offset: usize): u64 {
    let v: u64 = 0 as u64;
    v = v | (data[offset + 0] as u64);
    v = v | ((data[offset + 1] as u64) << (8 as u6));
    v = v | ((data[offset + 2] as u64) << (16 as u6));
    v = v | ((data[offset + 3] as u64) << (24 as u6));
    v = v | ((data[offset + 4] as u64) << (32 as u6));
    v = v | ((data[offset + 5] as u64) << (40 as u6));
    v = v | ((data[offset + 6] as u64) << (48 as u6));
    v = v | ((data[offset + 7] as u64) << (56 as u6));
    return v;
}

struct SipState {
    v0: u64,
    v1: u64,
    v2: u64,
    v3: u64,
}

fn shRound(st: *SipState) {
    let v0 = st.*.v0;
    let v1 = st.*.v1;
    let v2 = st.*.v2;
    let v3 = st.*.v3;

    v0 = shAdd64(v0, v1);
    v1 = shRotl64(v1, 13 as u32);
    v1 = v1 ^ v0;
    v0 = shRotl64(v0, 32 as u32);

    v2 = shAdd64(v2, v3);
    v3 = shRotl64(v3, 16 as u32);
    v3 = v3 ^ v2;

    v0 = shAdd64(v0, v3);
    v3 = shRotl64(v3, 21 as u32);
    v3 = v3 ^ v0;

    v2 = shAdd64(v2, v1);
    v1 = shRotl64(v1, 17 as u32);
    v1 = v1 ^ v2;
    v2 = shRotl64(v2, 32 as u32);

    st.*.v0 = v0;
    st.*.v1 = v1;
    st.*.v2 = v2;
    st.*.v3 = v3;
}

fn shRun(k0: u64, k1: u64, data: []const u8, c_rounds: usize, d_rounds: usize): u64 {
    const c0 = shU64(0x736F6D65 as u32, 0x70736575 as u32);
    const c1 = shU64(0x646F7261 as u32, 0x6E646F6D as u32);
    const c2 = shU64(0x6C796765 as u32, 0x6E657261 as u32);
    const c3 = shU64(0x74656462 as u32, 0x79746573 as u32);

    let st = SipState{
        v0: c0 ^ k0,
        v1: c1 ^ k1,
        v2: c2 ^ k0,
        v3: c3 ^ k1,
    };

    const fullBlocks = data.len / 8;
    let blk: usize = 0;
    while (blk < fullBlocks) {
        const m = shReadLe64(data, blk * 8);
        st.v3 = st.v3 ^ m;
        let r: usize = 0;
        while (r < c_rounds) {
            shRound(&st);
            r += 1;
        }
        st.v0 = st.v0 ^ m;
        blk += 1;
    }

    // Final block: length mod 256 in top byte + remaining 0..7 bytes in little-endian.
    const tailBase = fullBlocks * 8;
    const rem = data.len - tailBase;
    let b: u64 = ((data.len & 0xFF) as u64) << (56 as u6);
    if (rem >= 7) { b = b | ((data[tailBase + 6] as u64) << (48 as u6)); }
    if (rem >= 6) { b = b | ((data[tailBase + 5] as u64) << (40 as u6)); }
    if (rem >= 5) { b = b | ((data[tailBase + 4] as u64) << (32 as u6)); }
    if (rem >= 4) { b = b | ((data[tailBase + 3] as u64) << (24 as u6)); }
    if (rem >= 3) { b = b | ((data[tailBase + 2] as u64) << (16 as u6)); }
    if (rem >= 2) { b = b | ((data[tailBase + 1] as u64) << (8 as u6)); }
    if (rem >= 1) { b = b | (data[tailBase + 0] as u64); }

    st.v3 = st.v3 ^ b;
    let rc: usize = 0;
    while (rc < c_rounds) {
        shRound(&st);
        rc += 1;
    }
    st.v0 = st.v0 ^ b;

    // Finalization.
    st.v2 = st.v2 ^ (0xFF as u64);
    let rd: usize = 0;
    while (rd < d_rounds) {
        shRound(&st);
        rd += 1;
    }

    return st.v0 ^ st.v1 ^ st.v2 ^ st.v3;
}

/// Compute SipHash-2-4 (2 compression rounds, 4 finalization rounds) over `data` with 128-bit key `(k0, k1)`.
pub fn siphash24(k0: u64, k1: u64, data: []const u8): u64 {
    return shRun(k0, k1, data, 2, 4);
}

/// Compute SipHash-1-3 (1 compression round, 3 finalization rounds; Rust HashMap default) over `data`.
pub fn siphash13(k0: u64, k1: u64, data: []const u8): u64 {
    return shRun(k0, k1, data, 1, 3);
}

/// Compute SipHash-2-4 from a 16-byte little-endian key array.
pub fn siphash24Key16(key: [16]u8, data: []const u8): u64 {
    const k0 = shReadLe64(key[0..16], 0);
    const k1 = shReadLe64(key[0..16], 8);
    return siphash24(k0, k1, data);
}

/// Compute SipHash-1-3 from a 16-byte little-endian key array.
pub fn siphash13Key16(key: [16]u8, data: []const u8): u64 {
    const k0 = shReadLe64(key[0..16], 0);
    const k1 = shReadLe64(key[0..16], 8);
    return siphash13(k0, k1, data);
}

export {
    siphash24,
    siphash13,
    siphash24Key16,
    siphash13Key16,
};
