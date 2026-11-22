// Source: cp/lib/codec/bech32_crockford.cp (Pure CP BIP-173/BIP-350 Bech32/Bech32m & Crockford Base32 Codec)
// Zero-C-dependency Bech32/Bech32m checksummed addresses and human-friendly Crockford Base32 encoding.

const std = @import("std");
import { CodecError } from "./base5885.cp";

const BECH32_CHARSET: []const u8 = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
const CROCKFORD_ALPHABET: []const u8 = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
const CROCKFORD_CHECK_ALPHABET: []const u8 = "0123456789ABCDEFGHJKMNPQRSTVWXYZ*~$=U";

/// Checksum constant distinguishing BIP-173 (`Bech32`) from BIP-350 (`Bech32m`).
pub enum Bech32Variant: u32 {
    Bech32 = 1,
    Bech32m = 0x2bc830a3,
}

/// Decoded result returned by `Bech32.decode` and `Bech32.decodeBytes`.
pub struct Bech32Decoded {
    hrp: string = "",
    data: []u8 = [],
    variant: Bech32Variant = Bech32Variant.Bech32,
}

fn bech32CharVal(c_raw: u8): u8 {
    let c = c_raw;
    if (c >= 'A' && c <= 'Z') {
        c += 32;
    }
    return switch (c) {
        'q' => 0,  'p' => 1,  'z' => 2,  'r' => 3,
        'y' => 4,  '9' => 5,  'x' => 6,  '8' => 7,
        'g' => 8,  'f' => 9,  '2' => 10, 't' => 11,
        'v' => 12, 'd' => 13, 'w' => 14, '0' => 15,
        's' => 16, '3' => 17, 'j' => 18, 'n' => 19,
        '5' => 20, '4' => 21, 'k' => 22, 'h' => 23,
        'c' => 24, 'e' => 25, '6' => 26, 'm' => 27,
        'u' => 28, 'a' => 29, '7' => 30, 'l' => 31,
        else => 0xFF,
    };
}

fn crockfordVal(c: u8): u8 {
    return switch (c) {
        '0' | 'O' | 'o' => 0,
        '1' | 'I' | 'i' | 'L' | 'l' => 1,
        '2' => 2,
        '3' => 3,
        '4' => 4,
        '5' => 5,
        '6' => 6,
        '7' => 7,
        '8' => 8,
        '9' => 9,
        'A' | 'a' => 10,
        'B' | 'b' => 11,
        'C' | 'c' => 12,
        'D' | 'd' => 13,
        'E' | 'e' => 14,
        'F' | 'f' => 15,
        'G' | 'g' => 16,
        'H' | 'h' => 17,
        'J' | 'j' => 18,
        'K' | 'k' => 19,
        'M' | 'm' => 20,
        'N' | 'n' => 21,
        'P' | 'p' => 22,
        'Q' | 'q' => 23,
        'R' | 'r' => 24,
        'S' | 's' => 25,
        'T' | 't' => 26,
        'V' | 'v' => 27,
        'W' | 'w' => 28,
        'X' | 'x' => 29,
        'Y' | 'y' => 30,
        'Z' | 'z' => 31,
        else => 0xFF,
    };
}

fn crockfordCheckVal(c: u8): u8 {
    const base = crockfordVal(c);
    if (base != 0xFF) {
        return base;
    }
    return switch (c) {
        '*' => 32,
        '~' => 33,
        '$' => 34,
        '=' => 35,
        'U' | 'u' => 36,
        else => 0xFF,
    };
}

fn verifyBech32Checksum(hrp_lower: []const u8, data_and_chk: []const u8): Bech32Variant? {
    let chk: u32 = 1;
    // Expand HRP high 3 bits
    for (c of hrp_lower) {
        chk = bech32PolymodStep(chk, c >> 5);
    }
    // Separator 0
    chk = bech32PolymodStep(chk, 0);
    // Expand HRP low 5 bits
    for (c of hrp_lower) {
        chk = bech32PolymodStep(chk, c & 0x1F);
    }
    // Data + checksum 5-bit values
    for (v of data_and_chk) {
        chk = bech32PolymodStep(chk, v);
    }
    if (chk == 1) return Bech32Variant.Bech32;
    if (chk == 0x2bc830a3) return Bech32Variant.Bech32m;
    return null;
}

fn bech32PolymodStep(chk_in: u32, val: u8): u32 {
    const top = chk_in >> 25;
    let chk = ((chk_in & 0x1ffffff) << 5) ^ (val as u32);
    if ((top & 1) != 0) chk ^= 0x3b6a57b2;
    if ((top & 2) != 0) chk ^= 0x26508e6d;
    if ((top & 4) != 0) chk ^= 0x1ea119fa;
    if ((top & 8) != 0) chk ^= 0x3d4233dd;
    if ((top & 16) != 0) chk ^= 0x2a1462b3;
    return chk;
}

fn createBech32Checksum(hrp_lower: []const u8, data_5bit: []const u8, variant: Bech32Variant): [6]u8 {
    let chk: u32 = 1;
    for (c of hrp_lower) {
        chk = bech32PolymodStep(chk, c >> 5);
    }
    chk = bech32PolymodStep(chk, 0);
    for (c of hrp_lower) {
        chk = bech32PolymodStep(chk, c & 0x1F);
    }
    for (v of data_5bit) {
        chk = bech32PolymodStep(chk, v);
    }
    for (_i of 0..6) {
        chk = bech32PolymodStep(chk, 0);
    }
    const pm = chk ^ (variant as u32);
    let out: [6]u8 = [0; 6];
    let i: usize = 0;
    while (i < 6) : (i += 1) {
        const shift: u5 = @intCast((5 - i) * 5);
        out[i] = ((pm >> shift) & 31) as u8;
    }
    return out;
}

/// Pure CP BIP-173 (Bech32) and BIP-350 (Bech32m) encoder and decoder.
pub struct Bech32 {
    /// Expands an HRP into 5-bit values for BCH checksum calculation per BIP-173.
    static fn hrpExpand(hrp: []const u8): ([]u8) ! CodecError {
        return Bech32.hrpExpandAlloc(std.heap.page_allocator, hrp);
    }

    /// Expands an HRP into 5-bit values using `alloc`.
    static fn hrpExpandAlloc(alloc: std.mem.Allocator, hrp: []const u8): ([]u8) ! CodecError {
        let out = alloc.alloc(u8, hrp.len * 2 + 1) catch return CodecError.OutOfMemory;
        let i: usize = 0;
        while (i < hrp.len) : (i += 1) {
            out[i] = hrp[i] >> 5;
        }
        out[hrp.len] = 0;
        let j: usize = 0;
        while (j < hrp.len) : (j += 1) {
            out[hrp.len + 1 + j] = hrp[j] & 0x1F;
        }
        return out;
    }

    /// Computes the BIP-173 BCH `polymod` checksum over a slice of 5-bit values.
    static fn polymod(values: []const u8): u32 {
        let chk: u32 = 1;
        for (v of values) {
            chk = bech32PolymodStep(chk, v);
        }
        return chk;
    }

    /// Converts bit groups between bit widths `from_bits` and `to_bits` (1..8), with optional zero-padding.
    static fn convertBits(
        data: []const u8,
        from_bits: u8,
        to_bits: u8,
        pad: bool,
    ): ([]u8) ! CodecError {
        return Bech32.convertBitsAlloc(std.heap.page_allocator, data, from_bits, to_bits, pad);
    }

    /// Converts bit groups between bit widths `from_bits` and `to_bits` (1..8) using `alloc`.
    static fn convertBitsAlloc(
        alloc: std.mem.Allocator,
        data: []const u8,
        from_bits: u8,
        to_bits: u8,
        pad: bool,
    ): ([]u8) ! CodecError {
        if (from_bits == 0 || from_bits > 8 || to_bits == 0 || to_bits > 8) {
            return CodecError.InvalidLength;
        }

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let acc: u32 = 0;
        let bits: u8 = 0;
        const to_shift: u5 = @intCast(to_bits);
        const from_shift: u5 = @intCast(from_bits);
        const acc_shift: u5 = @intCast(from_bits + to_bits - 1);
        const maxv: u32 = ((1 as u32) << to_shift) - 1;
        const max_acc: u32 = ((1 as u32) << acc_shift) - 1;

        for (value of data) {
            if (from_bits < 8) {
                const check_shift: u3 = @intCast(from_bits);
                if ((value >> check_shift) != 0) {
                    return CodecError.InvalidCharacter;
                }
            }
            acc = ((acc << from_shift) | (value as u32)) & max_acc;
            bits += from_bits;
            while (bits >= to_bits) {
                bits -= to_bits;
                const b_shift: u5 = @intCast(bits);
                const item: u8 = ((acc >> b_shift) & maxv) as u8;
                out.append(alloc, item) catch return CodecError.OutOfMemory;
            }
        }

        if (pad) {
            if (bits > 0) {
                const shift: u5 = @intCast(to_bits - bits);
                const item: u8 = ((acc << shift) & maxv) as u8;
                out.append(alloc, item) catch return CodecError.OutOfMemory;
            }
        } else {
            if (bits >= from_bits) {
                return CodecError.InvalidLength;
            }
            if (bits > 0) {
                const shift: u5 = @intCast(to_bits - bits);
                if (((acc << shift) & maxv) != 0) {
                    return CodecError.InvalidLength;
                }
            }
        }

        return out.toOwnedSlice(alloc) catch return CodecError.OutOfMemory;
    }

    /// Encodes `hrp` and 5-bit `data_5bit` words into a Bech32 or Bech32m string.
    static fn encode(
        hrp: []const u8,
        data_5bit: []const u8,
        variant: Bech32Variant,
    ): ([]u8) ! CodecError {
        return Bech32.encodeAlloc(std.heap.page_allocator, hrp, data_5bit, variant);
    }

    /// Encodes `hrp` and 5-bit `data_5bit` words using `alloc`.
    static fn encodeAlloc(
        alloc: std.mem.Allocator,
        hrp: []const u8,
        data_5bit: []const u8,
        variant: Bech32Variant,
    ): ([]u8) ! CodecError {
        if (hrp.len == 0 || hrp.len > 83) {
            return CodecError.InvalidLength;
        }
        const total_len = hrp.len + 1 + data_5bit.len + 6;
        if (total_len > 90) {
            return CodecError.InvalidLength;
        }

        let has_lower = false;
        let has_upper = false;
        for (c of hrp) {
            if (c < 33 || c > 126) {
                return CodecError.InvalidCharacter;
            }
            if (c >= 'a' && c <= 'z') has_lower = true;
            if (c >= 'A' && c <= 'Z') has_upper = true;
        }
        if (has_lower && has_upper) {
            return CodecError.MixedCase;
        }

        for (v of data_5bit) {
            if (v >= 32) {
                return CodecError.Overflow;
            }
        }

        let out = alloc.alloc(u8, total_len) catch return CodecError.OutOfMemory;
        errdefer alloc.free(out);

        for (i of 0..hrp.len) {
            let c = hrp[i];
            if (c >= 'A' && c <= 'Z') {
                c += 32;
            }
            out[i] = c;
        }
        const hrp_lower = out[0..hrp.len];
        const chk = createBech32Checksum(hrp_lower, data_5bit, variant);

        out[hrp.len] = '1';
        let pos = hrp.len + 1;
        for (v of data_5bit) {
            out[pos] = BECH32_CHARSET[v as usize];
            pos += 1;
        }
        let ci: usize = 0;
        while (ci < 6) : (ci += 1) {
            out[pos] = BECH32_CHARSET[chk[ci] as usize];
            pos += 1;
        }
        return out;
    }

    /// Decodes a Bech32 or Bech32m string, rejecting mixed case, invalid HRP, length > 90, and bad checksums.
    static fn decode(encoded: []const u8): Bech32Decoded ! CodecError {
        return Bech32.decodeAlloc(std.heap.page_allocator, encoded);
    }

    /// Decodes a Bech32 or Bech32m string using `alloc`.
    static fn decodeAlloc(alloc: std.mem.Allocator, encoded: []const u8): Bech32Decoded ! CodecError {
        if (encoded.len < 8 || encoded.len > 90) {
            return CodecError.InvalidLength;
        }

        let has_lower = false;
        let has_upper = false;
        let sep_pos_opt: usize? = null;

        let idx: usize = 0;
        while (idx < encoded.len) : (idx += 1) {
            const c = encoded[idx];
            if (c < 33 || c > 126) {
                return CodecError.InvalidCharacter;
            }
            if (c >= 'a' && c <= 'z') has_lower = true;
            if (c >= 'A' && c <= 'Z') has_upper = true;
            if (c == '1') {
                sep_pos_opt = idx;
            }
        }

        if (has_lower && has_upper) {
            return CodecError.MixedCase;
        }

        if (sep_pos_opt == null) {
            return CodecError.InvalidPrefix;
        }
        const sep_pos = sep_pos_opt.?;
        if (sep_pos == 0) {
            return CodecError.InvalidPrefix;
        }
        if (sep_pos + 7 > encoded.len) {
            return CodecError.InvalidLength;
        }

        let hrp_lower = alloc.alloc(u8, sep_pos) catch return CodecError.OutOfMemory;
        errdefer alloc.free(hrp_lower);
        let h_idx: usize = 0;
        while (h_idx < sep_pos) : (h_idx += 1) {
            let c = encoded[h_idx];
            if (c >= 'A' && c <= 'Z') {
                c += 32;
            }
            hrp_lower[h_idx] = c;
        }

        const data_total = encoded.len - sep_pos - 1;
        let all_vals = alloc.alloc(u8, data_total) catch return CodecError.OutOfMemory;
        defer alloc.free(all_vals);

        let d_idx: usize = 0;
        while (d_idx < data_total) : (d_idx += 1) {
            const v = bech32CharVal(encoded[sep_pos + 1 + d_idx]);
            if (v == 0xFF) {
                return CodecError.InvalidCharacter;
            }
            all_vals[d_idx] = v;
        }

        const variant_opt = verifyBech32Checksum(hrp_lower, all_vals);
        if (variant_opt == null) {
            return CodecError.InvalidChecksum;
        }
        const variant = variant_opt.?;
        const payload_len = data_total - 6;
        let payload = alloc.alloc(u8, payload_len) catch return CodecError.OutOfMemory;
        if (payload_len > 0) {
            @memcpy(payload, all_vals[0..payload_len]);
        }

        return Bech32Decoded {
            hrp: hrp_lower,
            data: payload,
            variant: variant,
        };
    }

    /// Convenience wrapper that packs 8-bit `bytes` into 5-bit words and encodes with Bech32/Bech32m.
    static fn encodeBytes(
        hrp: []const u8,
        bytes: []const u8,
        variant: Bech32Variant,
    ): ([]u8) ! CodecError {
        return Bech32.encodeBytesAlloc(std.heap.page_allocator, hrp, bytes, variant);
    }

    /// Convenience wrapper that packs 8-bit `bytes` into 5-bit words and encodes using `alloc`.
    static fn encodeBytesAlloc(
        alloc: std.mem.Allocator,
        hrp: []const u8,
        bytes: []const u8,
        variant: Bech32Variant,
    ): ([]u8) ! CodecError {
        const data5 = Bech32.convertBitsAlloc(alloc, bytes, 8, 5, true)?;
        defer alloc.free(data5);
        return Bech32.encodeAlloc(alloc, hrp, data5, variant);
    }

    /// Convenience wrapper that decodes Bech32/Bech32m and unpacks the 5-bit payload back to 8-bit bytes.
    static fn decodeBytes(encoded: []const u8): Bech32Decoded ! CodecError {
        return Bech32.decodeBytesAlloc(std.heap.page_allocator, encoded);
    }

    /// Convenience wrapper that decodes Bech32/Bech32m and unpacks to 8-bit bytes using `alloc`.
    static fn decodeBytesAlloc(alloc: std.mem.Allocator, encoded: []const u8): Bech32Decoded ! CodecError {
        const decoded = Bech32.decodeAlloc(alloc, encoded)?;
        errdefer alloc.free(decoded.hrp);
        defer alloc.free(decoded.data);

        const raw_bytes = Bech32.convertBitsAlloc(alloc, decoded.data, 5, 8, false)?;
        return Bech32Decoded {
            hrp: decoded.hrp,
            data: raw_bytes,
            variant: decoded.variant,
        };
    }
}

/// Pure CP human-friendly Crockford Base32 codec (`I`/`L`->`1`, `O`->`0`, `-` ignored, optional mod-37 check symbol).
pub struct Crockford32 {
    /// Encodes arbitrary `bytes` into uppercase Crockford Base32 without padding.
    static fn encode(bytes: []const u8): ([]u8) ! CodecError {
        return Crockford32.encodeAlloc(std.heap.page_allocator, bytes);
    }

    /// Encodes arbitrary `bytes` into uppercase Crockford Base32 using `alloc`.
    static fn encodeAlloc(alloc: std.mem.Allocator, bytes: []const u8): ([]u8) ! CodecError {
        if (bytes.len == 0) {
            return alloc.alloc(u8, 0) catch return CodecError.OutOfMemory;
        }
        const vals = Bech32.convertBitsAlloc(alloc, bytes, 8, 5, true)?;
        defer alloc.free(vals);

        let out = alloc.alloc(u8, vals.len) catch return CodecError.OutOfMemory;
        for (i of 0..vals.len) {
            out[i] = CROCKFORD_ALPHABET[vals[i] as usize];
        }
        return out;
    }

    /// Decodes Crockford Base32 text (ignoring `-`, case-insensitive, mapping `I`/`L`->`1` and `O`->`0`).
    static fn decode(text: []const u8): ([]u8) ! CodecError {
        return Crockford32.decodeAlloc(std.heap.page_allocator, text);
    }

    /// Decodes Crockford Base32 text using `alloc`.
    static fn decodeAlloc(alloc: std.mem.Allocator, text: []const u8): ([]u8) ! CodecError {
        let vals: std.ArrayList(u8) = .empty;
        defer vals.deinit(alloc);

        for (c of text) {
            if (c == '-') continue;
            const v = crockfordVal(c);
            if (v == 0xFF) {
                return CodecError.InvalidCharacter;
            }
            vals.append(alloc, v) catch return CodecError.OutOfMemory;
        }

        if (vals.items.len == 0) {
            return alloc.alloc(u8, 0) catch return CodecError.OutOfMemory;
        }
        return Bech32.convertBitsAlloc(alloc, vals.items, 5, 8, false);
    }

    /// Encodes an unsigned 64-bit integer with a trailing Crockford modulo-37 check symbol (`*~$=U`).
    static fn encodeU64Check(value: u64): ([]u8) ! CodecError {
        return Crockford32.encodeU64CheckAlloc(std.heap.page_allocator, value);
    }

    /// Encodes an unsigned 64-bit integer with a trailing Crockford modulo-37 check symbol using `alloc`.
    static fn encodeU64CheckAlloc(alloc: std.mem.Allocator, value: u64): ([]u8) ! CodecError {
        const chk_idx = (value % 37) as usize;
        let rev: [14]u8 = [0; 14];
        let count: usize = 0;
        let v = value;

        if (v == 0) {
            rev[0] = '0';
            count = 1;
        } else {
            while (v > 0) {
                rev[count] = CROCKFORD_ALPHABET[(v % 32) as usize];
                count += 1;
                v /= 32;
            }
        }

        let out = alloc.alloc(u8, count + 1) catch return CodecError.OutOfMemory;
        let i: usize = 0;
        while (i < count) : (i += 1) {
            out[i] = rev[count - 1 - i];
        }
        out[count] = CROCKFORD_CHECK_ALPHABET[chk_idx];
        return out;
    }

    /// Decodes a Crockford Base32 integer with trailing modulo-37 check symbol (`*~$=U`), verifying integrity.
    static fn decodeU64Check(text: []const u8): u64 ! CodecError {
        let filtered: [64]u8 = [0; 64];
        let n: usize = 0;

        for (c of text) {
            if (c == '-') continue;
            if (n >= filtered.len) {
                return CodecError.Overflow;
            }
            filtered[n] = c;
            n += 1;
        }

        if (n < 2) {
            return CodecError.InvalidLength;
        }

        const chk_val = crockfordCheckVal(filtered[n - 1]);
        if (chk_val == 0xFF) {
            return CodecError.InvalidCharacter;
        }

        let acc: u64 = 0;
        const max_u64 = std.math.maxInt(u64);
        for (i of 0 .. n - 1) {
            const d = crockfordVal(filtered[i]);
            if (d == 0xFF) {
                return CodecError.InvalidCharacter;
            }
            const d64 = d as u64;
            if (acc > (max_u64 - d64) / 32) {
                return CodecError.Overflow;
            }
            acc = acc * 32 + d64;
        }

        if ((acc % 37) != (chk_val as u64)) {
            return CodecError.InvalidChecksum;
        }
        return acc;
    }
}
