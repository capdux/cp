// @cap/short-uuid — Zero-allocation 128-bit UUID lossless short ID translator for CP
// Clean-room MIT implementation inspired by npm `short-uuid` and `uuid-translator`.
// Accelerated with 16-lane @Vector(16, u8) SIMD hex/alphabet checks, comptime 256-entry Hex/Base58/Base62/Base90 LUTs, and undefined stack buffers.

/// Flickr Base58 alphabet (58 chars, omits ambiguous `0`, `O`, `I`, `l`).
pub const FLICKR_BASE58: string = "123456789abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ";

/// URL-safe Base62 alphabet (62 chars: `0-9`, `A-Z`, `a-z`).
pub const URL_BASE62: string = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

/// RFC 6265 Cookie-safe Base90 alphabet (90 chars).
pub const COOKIE_BASE90: string = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#$%&'()*+-./:<=>?@[]^_`{|}~";

type SuVec16 = @Vector(16, u8);

fn suBuildHexDecodeLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [255; 256];
    let c: usize = 48;
    while (c <= 57) {
        lut[c] = (c - 48) as u8;
        c += 1;
    }
    c = 65;
    while (c <= 70) {
        lut[c] = (c - 55) as u8;
        c += 1;
    }
    c = 97;
    while (c <= 102) {
        lut[c] = (c - 87) as u8;
        c += 1;
    }
    return lut;
}

fn suBuildHexHiLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    const hex = "0123456789abcdef";
    let lut: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        lut[i] = hex[(i >> 4) & 0x0F];
        i += 1;
    }
    return lut;
}

fn suBuildHexLoLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    const hex = "0123456789abcdef";
    let lut: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        lut[i] = hex[i & 0x0F];
        i += 1;
    }
    return lut;
}

fn suBuildAlphabetInvLut(alphabet: string) -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [255; 256];
    let i: usize = 0;
    while (i < alphabet.len) {
        lut[alphabet[i] as usize] = i as u8;
        i += 1;
    }
    return lut;
}

const SU_HEX_DECODE_LUT: [256]u8 = suBuildHexDecodeLut();
const SU_HEX_HI: [256]u8 = suBuildHexHiLut();
const SU_HEX_LO: [256]u8 = suBuildHexLoLut();
const SU_FLICKR58_INV: [256]u8 = suBuildAlphabetInvLut(FLICKR_BASE58);
const SU_BASE62_INV: [256]u8 = suBuildAlphabetInvLut(URL_BASE62);
const SU_BASE90_INV: [256]u8 = suBuildAlphabetInvLut(COOKIE_BASE90);

fn suStrEqSimd(a: string, b: string) -> bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va: SuVec16 = a[i .. i + 16][0..16].*;
        const vb: SuVec16 = b[i .. i + 16][0..16].*;
        if (!@reduce(.And, va == vb)) {
            return false;
        }
        i += 16;
    }
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn suIsHex16Simd(s: string, off: usize) -> bool {
    const v: SuVec16 = s[off .. off + 16][0..16].*;
    const v48: SuVec16 = @splat(48 as u8);
    const v57: SuVec16 = @splat(57 as u8);
    const v65: SuVec16 = @splat(65 as u8);
    const v70: SuVec16 = @splat(70 as u8);
    const v97: SuVec16 = @splat(97 as u8);
    const v102: SuVec16 = @splat(102 as u8);
    const m_dig = (v >= v48) & (v <= v57);
    const m_up = (v >= v65) & (v <= v70);
    const m_lo = (v >= v97) & (v <= v102);
    return @reduce(.And, m_dig | m_up | m_lo);
}

// ---------------------------------------------------------------------------
// Hex / UUID Byte Helpers
// ---------------------------------------------------------------------------

/// Parses a standard 36-char hyphenated UUID or 32-char compact hex UUID (case-insensitive)
/// into 16 big-endian bytes in `out16`. Returns `false` on invalid input or small buffer.
pub fn uuidToBytes(uuid: string, out16: []u8) -> bool {
    if (out16.len < 16) {
        return false;
    }
    if (uuid.len == 36) {
        if (uuid[8] != '-' || uuid[13] != '-' || uuid[18] != '-' || uuid[23] != '-') {
            return false;
        }
        let byte_idx: usize = 0;
        let i: usize = 0;
        while (i < 36) {
            if (i == 8 || i == 13 || i == 18 || i == 23) {
                i += 1;
                continue;
            }
            const hi = SU_HEX_DECODE_LUT[uuid[i] as usize];
            const lo = SU_HEX_DECODE_LUT[uuid[i + 1] as usize];
            if ((hi | lo) >= 16) {
                return false;
            }
            out16[byte_idx] = (hi << 4) | lo;
            byte_idx += 1;
            i += 2;
        }
        return byte_idx == 16;
    }
    if (uuid.len == 32) {
        if (!suIsHex16Simd(uuid, 0) || !suIsHex16Simd(uuid, 16)) {
            return false;
        }
        let byte_idx: usize = 0;
        while (byte_idx < 16) {
            const hi = SU_HEX_DECODE_LUT[uuid[byte_idx * 2] as usize];
            const lo = SU_HEX_DECODE_LUT[uuid[byte_idx * 2 + 1] as usize];
            out16[byte_idx] = (hi << 4) | lo;
            byte_idx += 1;
        }
        return true;
    }
    return false;
}

/// Formats 16 big-endian bytes into a canonical lowercase 36-char UUID (`8-4-4-4-12`).
pub fn bytesToUuid(bytes16: []u8, out36: []u8) -> string {
    if (bytes16.len < 16 || out36.len < 36) {
        return out36[0..0];
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < 16) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            out36[pos] = '-';
            pos += 1;
        }
        const b = bytes16[i] as usize;
        out36[pos] = SU_HEX_HI[b];
        out36[pos + 1] = SU_HEX_LO[b];
        pos += 2;
        i += 1;
    }
    return out36[0..36];
}

// ---------------------------------------------------------------------------
// 128-bit Base-N Arithmetic Helpers
// ---------------------------------------------------------------------------

fn validateAlphabet(alphabet: string) -> bool {
    if (alphabet.len < 2 || alphabet.len > 128) {
        return false;
    }
    if (suStrEqSimd(alphabet, FLICKR_BASE58) || suStrEqSimd(alphabet, URL_BASE62) || suStrEqSimd(alphabet, COOKIE_BASE90)) {
        return true;
    }
    let seen: [256]u8 = undefined;
    @memset(seen[0..256], 0);
    let i: usize = 0;
    while (i < alphabet.len) {
        const idx = alphabet[i] as usize;
        if (seen[idx] != 0) {
            return false;
        }
        seen[idx] = 1;
        i += 1;
    }
    return true;
}

/// Computes the exact fixed character width needed to encode any 128-bit value in `base`.
/// (e.g. Base58 -> 22, Base62 -> 22, Base90 -> 20).
fn maxEncodedLen(base: u32) -> usize {
    if (base == 58 || base == 62) { return 22; }
    if (base == 90) { return 20; }
    let max_val: [16]u8 = [
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    ];
    let digits: usize = 0;
    while (true) {
        let rem: u32 = 0;
        let non_zero = false;
        let i: usize = 0;
        while (i < 16) {
            const acc: u32 = (rem << 8) | (max_val[i] as u32);
            const q = (acc / base) as u8;
            rem = acc % base;
            max_val[i] = q;
            if (q != 0) {
                non_zero = true;
            }
            i += 1;
        }
        digits += 1;
        if (!non_zero) {
            break;
        }
    }
    return digits;
}

fn shortenBytesFast(bytes: []u8, alphabet: string, fixed_len: usize, out: []u8) -> string? {
    if (out.len < fixed_len || bytes.len < 16) {
        return null;
    }
    const base = alphabet.len as u32;
    let step: usize = 0;
    while (step < fixed_len) {
        let rem: u32 = 0;
        let i: usize = 0;
        while (i < 16) {
            const acc: u32 = (rem << 8) | (bytes[i] as u32);
            bytes[i] = (acc / base) as u8;
            rem = acc % base;
            i += 1;
        }
        out[fixed_len - 1 - step] = alphabet[rem as usize];
        step += 1;
    }
    return out[0..fixed_len];
}

fn expandWithLut(short_id: string, inv: []const u8, base: u32, out36: []u8) -> string? {
    let bytes: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < short_id.len) {
        const d = inv[short_id[i] as usize];
        if (d == 255) {
            return null;
        }
        let carry: u32 = d as u32;
        let j: usize = 16;
        while (j > 0) {
            j -= 1;
            const acc: u32 = (bytes[j] as u32) * base + carry;
            bytes[j] = (acc & 0xFF) as u8;
            carry = acc >> 8;
        }
        if (carry != 0) {
            return null;
        }
        i += 1;
    }
    return bytesToUuid(bytes[0..16], out36);
}

// ---------------------------------------------------------------------------
// Public Shorten / Expand API
// ---------------------------------------------------------------------------

/// Encodes a 128-bit UUID into a fixed-length short ID using `alphabet` (e.g. `FLICKR_BASE58`,
/// `URL_BASE62`, `COOKIE_BASE90`), writing into `out`.
pub fn shortenUuidWith(uuid: string, alphabet: string, out: []u8) -> string? {
    if (!validateAlphabet(alphabet)) {
        return null;
    }
    let bytes: [16]u8 = undefined;
    if (!uuidToBytes(uuid, bytes[0..16])) {
        return null;
    }
    const fixed_len = maxEncodedLen(alphabet.len as u32);
    return shortenBytesFast(bytes[0..16], alphabet, fixed_len, out);
}

/// Decodes a short ID using `alphabet` back into a canonical 36-char lowercase UUID in `out36`.
/// Returns `null` on invalid alphabet, empty input, unknown character, 128-bit overflow, or small buffer.
pub fn expandUuidWith(short_id: string, alphabet: string, out36: []u8) -> string? {
    if (alphabet.len < 2 || alphabet.len > 128 || short_id.len == 0 || out36.len < 36) {
        return null;
    }
    if (suStrEqSimd(alphabet, FLICKR_BASE58)) {
        return expandWithLut(short_id, SU_FLICKR58_INV[0..256], 58, out36);
    }
    if (suStrEqSimd(alphabet, URL_BASE62)) {
        return expandWithLut(short_id, SU_BASE62_INV[0..256], 62, out36);
    }
    if (suStrEqSimd(alphabet, COOKIE_BASE90)) {
        return expandWithLut(short_id, SU_BASE90_INV[0..256], 90, out36);
    }

    // Build O(1) inverse LUT while validating alphabet uniqueness in a single O(N) pass
    let inv: [256]u8 = undefined;
    @memset(inv[0..256], 255);
    let a: usize = 0;
    while (a < alphabet.len) {
        const ch = alphabet[a] as usize;
        if (inv[ch] != 255) {
            return null;
        }
        inv[ch] = a as u8;
        a += 1;
    }

    return expandWithLut(short_id, inv[0..256], alphabet.len as u32, out36);
}

/// Compresses a 128-bit UUID into a fixed 22-character `FLICKR_BASE58` short ID in `out`.
pub fn shortenUuid(uuid: string, out: []u8) -> string? {
    let bytes: [16]u8 = undefined;
    if (!uuidToBytes(uuid, bytes[0..16])) {
        return null;
    }
    return shortenBytesFast(bytes[0..16], FLICKR_BASE58, 22, out);
}

/// Expands a `FLICKR_BASE58` short ID back into a canonical 36-character lowercase UUID in `out36`.
pub fn expandUuid(short_id: string, out36: []u8) -> string? {
    if (short_id.len == 0 || out36.len < 36) {
        return null;
    }
    return expandWithLut(short_id, SU_FLICKR58_INV[0..256], 58, out36);
}
