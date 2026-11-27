// @cap/base16ct — Pure CP zero-allocation constant-time Base16 / Hex codec
// Parity with RustCrypto `base16ct` (table-free branchless arithmetic masks, zero lookup tables).
// Accelerated with 16-lane @Vector(16, u8) constant-time SIMD encode/decode/zeroing and @memcpy.

type B16Vec16 = @Vector(16, u8);

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
    const limit = if (n < out.len) n else out.len;
    const z_arr: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i + 16 <= limit) {
        @memcpy(out[i .. i + 16], z_arr[0..16]);
        i += 16;
    }
    while (i < limit) {
        out[i] = 0;
        i += 1;
    }
}

pub fn encodedLen(n: usize): usize {
    return n * 2;
}

fn b16EncodeSimd(src: []u8, out: []u8, alpha_offset: u8): usize {
    let need = encodedLen(src.len);
    if (need == 0) {
        return 0;
    }
    if (out.len < need) {
        return 0;
    }
    const v_48: B16Vec16 = @splat(48 as u8);
    const v_10: B16Vec16 = @splat(10 as u8);
    const v_off: B16Vec16 = @splat(alpha_offset);
    const v_zero: B16Vec16 = @splat(0 as u8);

    let i: usize = 0;
    let o: usize = 0;
    while (i + 8 <= src.len) {
        let nibs: [16]u8 = undefined;
        nibs[0] = src[i] >> 4;
        nibs[1] = src[i] & 0x0F;
        nibs[2] = src[i + 1] >> 4;
        nibs[3] = src[i + 1] & 0x0F;
        nibs[4] = src[i + 2] >> 4;
        nibs[5] = src[i + 2] & 0x0F;
        nibs[6] = src[i + 3] >> 4;
        nibs[7] = src[i + 3] & 0x0F;
        nibs[8] = src[i + 4] >> 4;
        nibs[9] = src[i + 4] & 0x0F;
        nibs[10] = src[i + 5] >> 4;
        nibs[11] = src[i + 5] & 0x0F;
        nibs[12] = src[i + 6] >> 4;
        nibs[13] = src[i + 6] & 0x0F;
        nibs[14] = src[i + 7] >> 4;
        nibs[15] = src[i + 7] & 0x0F;

        const vn: B16Vec16 = nibs;
        const ascii_v: B16Vec16 = vn + v_48 + @select(u8, vn >= v_10, v_off, v_zero);
        const arr: [16]u8 = ascii_v;
        @memcpy(out[o .. o + 16], arr[0..16]);
        i += 8;
        o += 16;
    }
    while (i < src.len) {
        let b = src[i];
        if (alpha_offset == 39) {
            out[o] = b16EncodeNibbleLower(b >> 4);
            out[o + 1] = b16EncodeNibbleLower(b & 0x0F);
        } else {
            out[o] = b16EncodeNibbleUpper(b >> 4);
            out[o + 1] = b16EncodeNibbleUpper(b & 0x0F);
        }
        i += 1;
        o += 2;
    }
    return o;
}

pub fn encodeLower(src: []u8, out: []u8): usize {
    return b16EncodeSimd(src, out, 39);
}

pub fn encodeUpper(src: []u8, out: []u8): usize {
    return b16EncodeSimd(src, out, 7);
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

    const v_zero: B16Vec16 = @splat(0 as u8);
    const v_one: B16Vec16 = @splat(1 as u8);
    const v_48: B16Vec16 = @splat(48 as u8);
    const v_57: B16Vec16 = @splat(57 as u8);
    const v_97: B16Vec16 = @splat(97 as u8);
    const v_102: B16Vec16 = @splat(102 as u8);
    const v_87: B16Vec16 = @splat(87 as u8);
    const v_65: B16Vec16 = @splat(65 as u8);
    const v_70: B16Vec16 = @splat(70 as u8);
    const v_55: B16Vec16 = @splat(55 as u8);

    let err_vec: B16Vec16 = v_zero;
    let err: u32 = 0;
    let i: usize = 0;
    let oi: usize = 0;

    while (i + 16 <= hex.len) {
        const v: B16Vec16 = hex[i .. i + 16][0..16].*;
        const m_dig: B16Vec16 = @select(u8, v >= v_48, v_one, v_zero) & @select(u8, v <= v_57, v_one, v_zero);
        let m_low: B16Vec16 = v_zero;
        if (allow_lower) {
            m_low = @select(u8, v >= v_97, v_one, v_zero) & @select(u8, v <= v_102, v_one, v_zero);
        }
        let m_upp: B16Vec16 = v_zero;
        if (allow_upper) {
            m_upp = @select(u8, v >= v_65, v_one, v_zero) & @select(u8, v <= v_70, v_one, v_zero);
        }
        const valid: B16Vec16 = m_dig | m_low | m_upp;
        err_vec = err_vec | (valid ^ v_one);

        const nib_v: B16Vec16 = ((v -% v_48) * m_dig) | ((v -% v_87) * m_low) | ((v -% v_55) * m_upp);
        const vals: [16]u8 = nib_v;
        out[oi] = (vals[0] << 4) | vals[1];
        out[oi + 1] = (vals[2] << 4) | vals[3];
        out[oi + 2] = (vals[4] << 4) | vals[5];
        out[oi + 3] = (vals[6] << 4) | vals[7];
        out[oi + 4] = (vals[8] << 4) | vals[9];
        out[oi + 5] = (vals[10] << 4) | vals[11];
        out[oi + 6] = (vals[12] << 4) | vals[13];
        out[oi + 7] = (vals[14] << 4) | vals[15];
        i += 16;
        oi += 8;
    }

    if (@reduce(.Or, err_vec != v_zero)) {
        err = err | 0x80;
    }

    while (i < hex.len) {
        let hi = b16DecodeNibble(hex[i], allow_lower, allow_upper);
        let lo = b16DecodeNibble(hex[i + 1], allow_lower, allow_upper);
        err = err | hi | lo;
        out[oi] = (((hi & 0x0F) << 4) | (lo & 0x0F)) as u8;
        i += 2;
        oi += 1;
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
