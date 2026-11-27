// packages/@cap/block-padding/src/index.cp
// Zero-allocation cryptographic block padding & cipher mode primitives in pure CP.
// Implements RFC 5652 §6.3 PKCS#7 (with constant-time unpad), ISO/IEC 7816-4, ANSI X9.23,
// ZeroPadding, 128-bit BE/LE AES-CTR counter increment, and CBC/CTR XOR block.
// Reuses @cap/subtle-ct for branchless constant-time verification and zeroize.
// Clean-room MIT implementation (RustCrypto `block-padding` + `cipher` parity).

import { Choice, ctEqU8, ctLtU64, zeroizeBytes } from "../../subtle/src/index.cp";

type BpVec16 = @Vector(16, u8);

pub const BP_ERR: i64 = -1;

fn bpCopyBytes(dst: []u8, src: []const u8) {
    if (src.len > 0) {
        @memcpy(dst[0..src.len], src);
    }
}

fn bpFillBytes(dst: []u8, val: u8) {
    let i: usize = 0;
    const v: BpVec16 = @splat(val);
    const chunk: [16]u8 = v;
    while (i + 16 <= dst.len) {
        @memcpy(dst[i .. i + 16], chunk[0..16]);
        i += 16;
    }
    while (i < dst.len) {
        dst[i] = val;
        i += 1;
    }
}

/// Applies RFC 5652 §6.3 PKCS#7 padding to `input` for `1 <= block_size <= 255`.
/// Always appends between `1` and `block_size` bytes (full block appended when already aligned or empty).
/// Returns total padded length on success, or `-1` on invalid `block_size` or insufficient `out`.
pub fn pkcs7Pad(input: []const u8, block_size: usize, out: []u8): i64 {
    if (block_size == 0 || block_size > 255) {
        return BP_ERR;
    }
    const pad_len = block_size - (input.len % block_size);
    const total = input.len + pad_len;
    if (out.len < total) {
        return BP_ERR;
    }
    bpCopyBytes(out[0..input.len], input);
    bpFillBytes(out[input.len..total], pad_len as u8);
    return total as i64;
}

/// Constant-time PKCS#7 unpadding resistant to padding-oracle timing side channels.
/// Always scans the last `block_size` bytes without secret-dependent branches using `@cap/subtle-ct`.
/// Returns unpadded plaintext length on success, or `-1` on invalid padding.
pub fn pkcs7UnpadCt(padded: []const u8, block_size: usize): i64 {
    if (block_size == 0 || block_size > 255 || padded.len == 0 || (padded.len % block_size) != 0) {
        return BP_ERR;
    }
    const pad_len_u8 = padded[padded.len - 1];
    const pad_len_u64 = pad_len_u8 as u64;

    // Constant-time range check: 1 <= pad_len <= block_size
    const not_zero = ctEqU8(pad_len_u8, 0 as u8).not();
    const le_bs = ctLtU64(pad_len_u64, (block_size as u64) + (1 as u64));
    let good: Choice = not_zero.and(le_bs);

    // Branchless scan over the entire trailing block
    let i: usize = 0;
    while (i < block_size) {
        const b = padded[padded.len - 1 - i];
        const in_pad = ctLtU64(i as u64, pad_len_u64);
        const eq_pad = ctEqU8(b, pad_len_u8);
        // If in_pad == 1 then eq_pad must be 1; if in_pad == 0 then don't care
        good = good.and(in_pad.not().or(eq_pad));
        i = i + 1;
    }

    if (!good.toBool()) {
        return BP_ERR;
    }
    return (padded.len - (pad_len_u8 as usize)) as i64;
}

/// Applies ISO/IEC 7816-4 padding (`0x80` followed by zero or more `0x00` bytes).
/// Returns total padded length on success, or `-1` on error.
pub fn iso7816Pad(input: []const u8, block_size: usize, out: []u8): i64 {
    if (block_size == 0) {
        return BP_ERR;
    }
    const pad_len = block_size - (input.len % block_size);
    const total = input.len + pad_len;
    if (out.len < total) {
        return BP_ERR;
    }
    bpCopyBytes(out[0..input.len], input);
    out[input.len] = 0x80 as u8;
    if (pad_len > 1) {
        bpFillBytes(out[input.len + 1 .. total], 0x00 as u8);
    }
    return total as i64;
}

/// Removes ISO/IEC 7816-4 padding from the last block.
/// Returns unpadded length on success, or `-1` if no valid `0x80` marker is found in the last block.
pub fn iso7816Unpad(padded: []const u8, block_size: usize): i64 {
    if (block_size == 0 || padded.len == 0 || (padded.len % block_size) != 0) {
        return BP_ERR;
    }
    let i: usize = 0;
    while (i < block_size) {
        const idx = padded.len - 1 - i;
        const b = padded[idx];
        if (b == (0x80 as u8)) {
            return idx as i64;
        }
        if (b != (0x00 as u8)) {
            return BP_ERR;
        }
        i = i + 1;
    }
    return BP_ERR;
}

/// Applies ANSI X9.23 padding (`pad_len - 1` zero bytes followed by `pad_len` as the last byte).
/// Returns total padded length on success, or `-1` on error.
pub fn ansiX923Pad(input: []const u8, block_size: usize, out: []u8): i64 {
    if (block_size == 0 || block_size > 255) {
        return BP_ERR;
    }
    const pad_len = block_size - (input.len % block_size);
    const total = input.len + pad_len;
    if (out.len < total) {
        return BP_ERR;
    }
    bpCopyBytes(out[0..input.len], input);
    if (pad_len > 1) {
        bpFillBytes(out[input.len .. total - 1], 0x00 as u8);
    }
    out[total - 1] = pad_len as u8;
    return total as i64;
}

/// Constant-time ANSI X9.23 unpadding: verifies `1 <= pad_len <= block_size` and that
/// all `pad_len - 1` preceding padding bytes are strictly `0x00`.
/// Returns unpadded length on success, or `-1` on invalid padding.
pub fn ansiX923Unpad(padded: []const u8, block_size: usize): i64 {
    if (block_size == 0 || block_size > 255 || padded.len == 0 || (padded.len % block_size) != 0) {
        return BP_ERR;
    }
    const pad_len_u8 = padded[padded.len - 1];
    const pad_len_u64 = pad_len_u8 as u64;

    const not_zero = ctEqU8(pad_len_u8, 0 as u8).not();
    const le_bs = ctLtU64(pad_len_u64, (block_size as u64) + (1 as u64));
    let good: Choice = not_zero.and(le_bs);

    // Check bytes at offset 1..pad_len-1 from end are all 0x00 in constant time
    let i: usize = 1;
    while (i < block_size) {
        const b = padded[padded.len - 1 - i];
        const in_pad = ctLtU64(i as u64, pad_len_u64);
        const is_zero = ctEqU8(b, 0x00 as u8);
        good = good.and(in_pad.not().or(is_zero));
        i = i + 1;
    }

    if (!good.toBool()) {
        return BP_ERR;
    }
    return (padded.len - (pad_len_u8 as usize)) as i64;
}

/// Applies ZeroPadding (`0x00` bytes appended up to a multiple of `block_size`).
/// For empty input, writes one full block of `0x00`; for non-empty already-aligned input, adds 0 bytes.
pub fn zeroPad(input: []const u8, block_size: usize, out: []u8): i64 {
    if (block_size == 0) {
        return BP_ERR;
    }
    let pad_len: usize = 0;
    if (input.len == 0) {
        pad_len = block_size;
    } else {
        const rem = input.len % block_size;
        if (rem != 0) {
            pad_len = block_size - rem;
        }
    }
    const total = input.len + pad_len;
    if (out.len < total) {
        return BP_ERR;
    }
    bpCopyBytes(out[0..input.len], input);
    if (pad_len > 0) {
        bpFillBytes(out[input.len..total], 0x00 as u8);
    }
    return total as i64;
}

/// Removes trailing `0x00` bytes from a ZeroPadded buffer.
/// Returns unpadded length on success, or `-1` on invalid block alignment.
pub fn zeroUnpad(padded: []const u8, block_size: usize): i64 {
    if (block_size == 0 || padded.len == 0 || (padded.len % block_size) != 0) {
        return BP_ERR;
    }
    let end = padded.len;
    const v_zero: BpVec16 = @splat(0 as u8);
    while (end >= 16) {
        const v: BpVec16 = padded[end - 16 .. end][0..16].*;
        if (@reduce(.Or, v != v_zero)) {
            break;
        }
        end -= 16;
    }
    while (end > 0 && padded[end - 1] == (0x00 as u8)) {
        end = end - 1;
    }
    return end as i64;
}

/// Increments a 128-bit big-endian counter block in place (wrapping modulo 2^128).
/// Overflow-safe in Debug mode. No-op if `counter.len < 16`.
pub fn ctrIncBe128(counter: []u8) {
    if (counter.len < 16) {
        return;
    }
    let carry: u16 = 1 as u16;
    let i: usize = 16;
    while (i > 0) {
        i = i - 1;
        const sum: u16 = (counter[i] as u16) + carry;
        counter[i] = (sum & (0xFF as u16)) as u8;
        carry = sum >> (8 as u4);
    }
}

/// Increments a 128-bit little-endian counter block in place (wrapping modulo 2^128).
/// Overflow-safe in Debug mode. No-op if `counter.len < 16`.
pub fn ctrIncLe128(counter: []u8) {
    if (counter.len < 16) {
        return;
    }
    let carry: u16 = 1 as u16;
    let i: usize = 0;
    while (i < 16) {
        const sum: u16 = (counter[i] as u16) + carry;
        counter[i] = (sum & (0xFF as u16)) as u8;
        carry = sum >> (8 as u4);
        i = i + 1;
    }
}

/// Computes `dst[i] = a[i] ^ b[i]` for CBC/CTR block operations.
/// Returns `false` if `a.len != b.len` or `dst.len < a.len`.
pub fn xorBlock(dst: []u8, a: []const u8, b: []const u8): bool {
    if (a.len != b.len || dst.len < a.len) {
        return false;
    }
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va: BpVec16 = a[i .. i + 16][0..16].*;
        const vb: BpVec16 = b[i .. i + 16][0..16].*;
        const out_arr: [16]u8 = va ^ vb;
        @memcpy(dst[i .. i + 16], out_arr[0..16]);
        i += 16;
    }
    while (i < a.len) {
        dst[i] = a[i] ^ b[i];
        i = i + 1;
    }
    return true;
}

/// Securely wipes a plaintext/keystream block using `@cap/subtle-ct` (`zeroizeBytes`).
pub fn wipeBlock(buf: []u8) {
    zeroizeBytes(buf);
}
