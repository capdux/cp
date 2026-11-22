// Origin: Pure-CP Stdlib (Zero-Alloc Hamming(7,4), SECDED(8,4) & SECDED(72,64) ECC Codec) | Created: 2026-11-22

/// Decoding status for SECDED (Single Error Correction, Double Error Detection) codes.
pub enum EccStatus: u8 {
    ok = 0,
    corrected = 1,
    double_error = 2,
}

/// Result of decoding a 7-bit Hamming(7,4) codeword.
pub struct Hamming4Decode {
    /// Decoded 4-bit nibble in the lower 4 bits (0..15).
    data: u8,
    /// True if a single-bit error was detected and corrected.
    corrected: bool,
    /// 1-indexed bit position corrected (1..7), or 0 if no error occurred.
    error_bit: u8,
}

/// Result of decoding an 8-bit extended SECDED Hamming(8,4) codeword.
pub struct Secded4Decode {
    /// Decoded 4-bit nibble in the lower 4 bits (0..15).
    data: u8,
    /// Error detection/correction status.
    status: EccStatus,
    /// 1-indexed bit position corrected (1..8), or 0 if no error or double error.
    error_bit: u8,
}

/// Result of decoding a 72-bit server DRAM ECC SECDED Hamming(72,64) codeword.
pub struct Secded64Decode {
    /// Decoded (and single-bit corrected) 64-bit data word.
    data: u64,
    /// Decoded (and single-bit corrected) 8-bit ECC byte.
    ecc: u8,
    /// Error detection/correction status.
    status: EccStatus,
    /// 1-indexed bit position corrected (1..72), or 0 if no error or double error.
    error_bit: u8,
}

// Parity generator masks for standard 1-indexed Hamming(71,64) data bit positions (3..71, skipping powers of 2).
const H72_MASK_0: u64 = 0xAB55555556AAAD5B;
const H72_MASK_1: u64 = 0xCD9999999B33366D;
const H72_MASK_2: u64 = 0xF1E1E1E1E3C3C78E;
const H72_MASK_3: u64 = 0x01FE01FE03FC07F0;
const H72_MASK_4: u64 = 0x01FFFE0003FFF800;
const H72_MASK_5: u64 = 0x01FFFFFFFC000000;
const H72_MASK_6: u64 = 0xFE00000000000000;

fn parity8(v: u8): u8 {
    let x: u8 = v ^ (v >> 4);
    x = x ^ (x >> 2);
    x = x ^ (x >> 1);
    return x & 1;
}

fn parity64(v: u64): u8 {
    let x: u64 = v ^ (v >> 32);
    x = x ^ (x >> 16);
    x = x ^ (x >> 8);
    x = x ^ (x >> 4);
    x = x ^ (x >> 2);
    x = x ^ (x >> 1);
    return (x & 1) as u8;
}

fn hamming74Syndrome(code7: u8): u8 {
    const b1: u8 = (code7 >> 0) & 1;
    const b2: u8 = (code7 >> 1) & 1;
    const b3: u8 = (code7 >> 2) & 1;
    const b4: u8 = (code7 >> 3) & 1;
    const b5: u8 = (code7 >> 4) & 1;
    const b6: u8 = (code7 >> 5) & 1;
    const b7: u8 = (code7 >> 6) & 1;

    const s1: u8 = b1 ^ b3 ^ b5 ^ b7;
    const s2: u8 = b2 ^ b3 ^ b6 ^ b7;
    const s4: u8 = b4 ^ b5 ^ b6 ^ b7;
    return s1 | (s2 << 1) | (s4 << 2);
}

fn hamming74ExtractData(code7: u8): u8 {
    const d1: u8 = (code7 >> 2) & 1;
    const d2: u8 = (code7 >> 4) & 1;
    const d3: u8 = (code7 >> 5) & 1;
    const d4: u8 = (code7 >> 6) & 1;
    return d1 | (d2 << 1) | (d3 << 2) | (d4 << 3);
}

/// Encodes the lower 4 bits of `nibble` into a 7-bit Hamming(7,4) codeword (bit 7 is 0).
/// Bit layout (1-indexed positions 1..7 -> bits 0..6): [p1, p2, d1, p4, d2, d3, d4].
pub fn hamming74Encode(nibble: u8): u8 {
    const d1: u8 = (nibble >> 0) & 1;
    const d2: u8 = (nibble >> 1) & 1;
    const d3: u8 = (nibble >> 2) & 1;
    const d4: u8 = (nibble >> 3) & 1;

    const p1: u8 = d1 ^ d2 ^ d4;
    const p2: u8 = d1 ^ d3 ^ d4;
    const p4: u8 = d2 ^ d3 ^ d4;

    return (p1 << 0)
        | (p2 << 1)
        | (d1 << 2)
        | (p4 << 3)
        | (d2 << 4)
        | (d3 << 5)
        | (d4 << 6);
}

/// Decodes a 7-bit Hamming(7,4) codeword, correcting any single-bit flip in positions 1..7.
pub fn hamming74Decode(code: u8): Hamming4Decode {
    const c: u8 = code & 0x7F;
    const syndrome: u8 = hamming74Syndrome(c);
    if (syndrome == 0) {
        return Hamming4Decode {
            data: hamming74ExtractData(c),
            corrected: false,
            error_bit: 0,
        };
    }

    const bit_idx: u3 = (syndrome - 1) as u3;
    const fixed: u8 = c ^ ((1 as u8) << bit_idx);
    return Hamming4Decode {
        data: hamming74ExtractData(fixed),
        corrected: true,
        error_bit: syndrome,
    };
}

/// Encodes the lower 4 bits of `nibble` into an 8-bit extended SECDED Hamming(8,4) codeword.
/// Bits 0..6 carry Hamming(7,4) and bit 7 carries the overall even-parity bit.
pub fn secded84Encode(nibble: u8): u8 {
    const c7: u8 = hamming74Encode(nibble);
    const p0: u8 = parity8(c7);
    return c7 | (p0 << 7);
}

/// Decodes an 8-bit extended SECDED Hamming(8,4) codeword.
/// Corrects any single-bit error (positions 1..8) and detects any double-bit error.
pub fn secded84Decode(code: u8): Secded4Decode {
    const syndrome: u8 = hamming74Syndrome(code & 0x7F);
    const overall: u8 = parity8(code);

    if (syndrome == 0) {
        if (overall == 0) {
            return Secded4Decode {
                data: hamming74ExtractData(code),
                status: EccStatus.ok,
                error_bit: 0,
            };
        }
        // Overall parity bit (position 8, bit 7) flipped; data bits are intact.
        return Secded4Decode {
            data: hamming74ExtractData(code),
            status: EccStatus.corrected,
            error_bit: 8,
        };
    }

    if (overall == 0) {
        // Non-zero syndrome with even overall parity indicates an uncorrectable double-bit error.
        return Secded4Decode {
            data: hamming74ExtractData(code),
            status: EccStatus.double_error,
            error_bit: 0,
        };
    }

    const bit_idx: u3 = (syndrome - 1) as u3;
    const fixed: u8 = code ^ ((1 as u8) << bit_idx);
    return Secded4Decode {
        data: hamming74ExtractData(fixed),
        status: EccStatus.corrected,
        error_bit: syndrome,
    };
}

/// Encodes `src` into `dst` using SECDED Hamming(8,4), expanding each byte into 2 bytes
/// (high nibble first, then low nibble). Returns written length (`src.len * 2`), or `null`
/// if `dst` is too small.
pub fn secded84EncodeBytes(src: []const u8, dst: *[]u8): usize? {
    const required: usize = src.len * 2;
    if (dst.*.len < required) {
        return null;
    }

    let i: usize = 0;
    while (i < src.len) {
        const b: u8 = src[i];
        dst.*[i * 2] = secded84Encode((b >> 4) & 0x0F);
        dst.*[i * 2 + 1] = secded84Encode(b & 0x0F);
        i += 1;
    }
    return required;
}

/// Decodes SECDED Hamming(8,4) byte stream `src` into `dst`.
/// Automatically corrects any single-bit flip per codeword byte.
/// Returns written length (`src.len / 2`), or `null` if `src.len` is odd, `dst` is too small,
/// or any codeword has an uncorrectable double-bit error.
pub fn secded84DecodeBytes(src: []const u8, dst: *[]u8): usize? {
    if ((src.len & 1) != 0) {
        return null;
    }
    const out_len: usize = src.len / 2;
    if (dst.*.len < out_len) {
        return null;
    }

    let i: usize = 0;
    while (i < out_len) {
        const hi: Secded4Decode = secded84Decode(src[i * 2]);
        if (hi.status == EccStatus.double_error) {
            return null;
        }
        const lo: Secded4Decode = secded84Decode(src[i * 2 + 1]);
        if (lo.status == EccStatus.double_error) {
            return null;
        }
        dst.*[i] = (hi.data << 4) | lo.data;
        i += 1;
    }
    return out_len;
}

fn computeHamming7(data: u64): u8 {
    const p1: u8 = parity64(data & H72_MASK_0);
    const p2: u8 = parity64(data & H72_MASK_1);
    const p4: u8 = parity64(data & H72_MASK_2);
    const p8: u8 = parity64(data & H72_MASK_3);
    const p16: u8 = parity64(data & H72_MASK_4);
    const p32: u8 = parity64(data & H72_MASK_5);
    const p64: u8 = parity64(data & H72_MASK_6);
    return (p1 << 0)
        | (p2 << 1)
        | (p4 << 2)
        | (p8 << 3)
        | (p16 << 4)
        | (p32 << 5)
        | (p64 << 6);
}

fn posToDataBit72(pos: u8): u8 {
    if (pos == 3) {
        return 0;
    }
    if (pos < 8) {
        return pos - 4;
    }
    if (pos < 16) {
        return pos - 5;
    }
    if (pos < 32) {
        return pos - 6;
    }
    if (pos < 64) {
        return pos - 7;
    }
    return pos - 8;
}

/// Computes the 8-bit server DRAM ECC SECDED Hamming(72,64) check byte for a 64-bit data word.
/// Bits 0..6 are the 7 Hamming parity bits (positions 1, 2, 4, 8, 16, 32, 64);
/// bit 7 is the overall even-parity bit (position 72) across all 72 bits.
pub fn secded7264Encode(data: u64): u8 {
    const p7: u8 = computeHamming7(data);
    const p_overall: u8 = (parity64(data) ^ parity8(p7)) & 1;
    return p7 | (p_overall << 7);
}

/// Decodes and validates a 64-bit data word paired with its 8-bit SECDED Hamming(72,64) ECC byte.
/// - No error: returns `EccStatus.ok`, `error_bit = 0`.
/// - Single-bit error in either `data` (64 bits) or `ecc` (8 bits): corrects it, returns `EccStatus.corrected`
///   and the 1-indexed `error_bit` in `1..72`.
/// - Double-bit error (or out-of-range multi-bit syndrome): returns `EccStatus.double_error`, `error_bit = 0`.
pub fn secded7264Decode(data: u64, ecc: u8): Secded64Decode {
    const expected_p7: u8 = computeHamming7(data);
    const syndrome: u8 = (ecc & 0x7F) ^ expected_p7;
    const overall: u8 = (parity64(data) ^ parity8(ecc)) & 1;

    if (syndrome == 0) {
        if (overall == 0) {
            return Secded64Decode {
                data: data,
                ecc: ecc,
                status: EccStatus.ok,
                error_bit: 0,
            };
        }
        // Overall parity bit (position 72, ecc bit 7) flipped.
        return Secded64Decode {
            data: data,
            ecc: ecc ^ 0x80,
            status: EccStatus.corrected,
            error_bit: 72,
        };
    }

    if (overall == 0 || syndrome > 71) {
        // Non-zero syndrome with even overall parity (or syndrome out of 1..71 range)
        // indicates an uncorrectable double/multi-bit error.
        return Secded64Decode {
            data: data,
            ecc: ecc,
            status: EccStatus.double_error,
            error_bit: 0,
        };
    }

    // Check if syndrome is a power of 2 (1, 2, 4, 8, 16, 32, 64) -> ECC parity bit flip.
    if ((syndrome & (syndrome - 1)) == 0) {
        let ecc_mask: u8 = 0;
        if (syndrome == 1) {
            ecc_mask = 0x01;
        } else if (syndrome == 2) {
            ecc_mask = 0x02;
        } else if (syndrome == 4) {
            ecc_mask = 0x04;
        } else if (syndrome == 8) {
            ecc_mask = 0x08;
        } else if (syndrome == 16) {
            ecc_mask = 0x10;
        } else if (syndrome == 32) {
            ecc_mask = 0x20;
        } else {
            ecc_mask = 0x40;
        }
        return Secded64Decode {
            data: data,
            ecc: ecc ^ ecc_mask,
            status: EccStatus.corrected,
            error_bit: syndrome,
        };
    }

    // Otherwise syndrome points to one of the 64 data bits in positions 3..71.
    const data_bit: u6 = posToDataBit72(syndrome) as u6;
    const fixed_data: u64 = data ^ ((1 as u64) << data_bit);
    return Secded64Decode {
        data: fixed_data,
        ecc: ecc,
        status: EccStatus.corrected,
        error_bit: syndrome,
    };
}
