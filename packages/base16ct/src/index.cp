// @cap/base16ct — Pure CP zero-allocation constant-time Base16 / Hex codec
// Parity with RustCrypto `base16ct` (table-free branchless arithmetic masks, zero lookup tables).

pub struct HexDecodeResult {
    pub len: usize,
    pub ok: bool,
}

fn b16InRange(c: u32, lo: u32, hi: u32): u32 {
    let ge_lo: u32 = ((c + 512) - lo) >> 9;
    let le_hi: u32 = ((hi + 512) - c) >> 9;
    return ge_lo & le_hi;
}

// Constant-time branchless 4-bit nibble -> lowercase hex ASCII (zero lookup tables)
fn b16EncodeNibbleLower(nib: u8): u8 {
    let n: u32 = (nib & 0x0F) as u32;
    let ge10: u32 = ((n + 512) - 10) >> 9;
    return (n + 48 + ge10 * 39) as u8;
}

// Constant-time branchless 4-bit nibble -> uppercase hex ASCII (zero lookup tables)
fn b16EncodeNibbleUpper(nib: u8): u8 {
    let n: u32 = (nib & 0x0F) as u32;
    let ge10: u32 = ((n + 512) - 10) >> 9;
    return (n + 48 + ge10 * 7) as u8;
}

// Constant-time branchless ASCII -> 4-bit nibble (sets bit 0x80 on invalid character)
fn b16DecodeNibble(ch: u8, allow_lower: bool, allow_upper: bool): u32 {
    let c: u32 = ch as u32;
    let m_dig = b16InRange(c, 48, 57);
    let m_low: u32 = 0;
    if (allow_lower) {
        m_low = b16InRange(c, 97, 102);
    }
    let m_upp: u32 = 0;
    if (allow_upper) {
        m_upp = b16InRange(c, 65, 70);
    }
    let valid = m_dig | m_low | m_upp;
    let val_i: i32 = (m_dig as i32) * ((c as i32) - 48)
        + (m_low as i32) * ((c as i32) - 87)
        + (m_upp as i32) * ((c as i32) - 55);
    return (val_i as u32) | ((valid ^ 1) << 7);
}

fn b16ZeroOut(out: []u8, n: usize) {
    let i: usize = 0;
    while (i < n && i < out.len) {
        out[i] = 0;
        i = i + 1;
    }
}

pub fn encodedLen(n: usize): usize {
    return n * 2;
}

pub fn encodeLower(src: []u8, out: []u8): usize {
    let need = encodedLen(src.len);
    if (need == 0) {
        return 0;
    }
    if (out.len < need) {
        return 0;
    }
    let i: usize = 0;
    let o: usize = 0;
    while (i < src.len) {
        let b = src[i];
        out[o] = b16EncodeNibbleLower(b >> 4);
        out[o + 1] = b16EncodeNibbleLower(b & 0x0F);
        i = i + 1;
        o = o + 2;
    }
    return o;
}

pub fn encodeUpper(src: []u8, out: []u8): usize {
    let need = encodedLen(src.len);
    if (need == 0) {
        return 0;
    }
    if (out.len < need) {
        return 0;
    }
    let i: usize = 0;
    let o: usize = 0;
    while (i < src.len) {
        let b = src[i];
        out[o] = b16EncodeNibbleUpper(b >> 4);
        out[o + 1] = b16EncodeNibbleUpper(b & 0x0F);
        i = i + 1;
        o = o + 2;
    }
    return o;
}

fn b16DecodeCore(hex: string, out: []u8, allow_lower: bool, allow_upper: bool): HexDecodeResult {
    if (hex.len == 0) {
        return HexDecodeResult { len: 0, ok: true };
    }
    if ((hex.len & 1) != 0) {
        b16ZeroOut(out, out.len);
        return HexDecodeResult { len: 0, ok: false };
    }
    let out_len = hex.len / 2;
    if (out.len < out_len) {
        return HexDecodeResult { len: 0, ok: false };
    }

    let err: u32 = 0;
    let i: usize = 0;
    let oi: usize = 0;
    while (i < hex.len) {
        let hi = b16DecodeNibble(hex[i], allow_lower, allow_upper);
        let lo = b16DecodeNibble(hex[i + 1], allow_lower, allow_upper);
        err = err | hi | lo;
        out[oi] = (((hi & 0x0F) << 4) | (lo & 0x0F)) as u8;
        i = i + 2;
        oi = oi + 1;
    }

    if ((err & 0x80) != 0) {
        b16ZeroOut(out, out_len);
        return HexDecodeResult { len: 0, ok: false };
    }
    return HexDecodeResult { len: out_len, ok: true };
}

pub fn decodeLower(hex: string, out: []u8): HexDecodeResult {
    return b16DecodeCore(hex, out, true, false);
}

pub fn decodeUpper(hex: string, out: []u8): HexDecodeResult {
    return b16DecodeCore(hex, out, false, true);
}

pub fn decodeMixed(hex: string, out: []u8): HexDecodeResult {
    return b16DecodeCore(hex, out, true, true);
}

export {
    HexDecodeResult,
    encodedLen,
    encodeLower,
    encodeUpper,
    decodeLower,
    decodeUpper,
    decodeMixed
};
