// Reference Only: Functional overlap with lib/codec/base58_85.cp | Kept for comparison/analysis
// Source: cp/lib/base58/base58.cp (Pure CP Base58, Base58Check, and BIP-173/350 Bech32/Bech32m Codec)
// Zero external C dependency implementation using pure CP and std.

const std = @import("std");
import { sha256Raw } from "cap:crypto";

/// Unified error set for Base58, Base58Check, and Bech32/Bech32m encoding and decoding.
pub err Base58Error {
    InvalidCharacter,
    InvalidChecksum,
    InvalidLength,
    InvalidHrp,
    MixedCase,
    MissingSeparator,
    InvalidPadding,
    OutOfMemory,
}

const BASE58_ALPHABET: []const u8 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
const BECH32_CHARSET: []const u8 = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

/// Checksum constant distinguishing BIP-173 (`Bech32`) from BIP-350 (`Bech32m`).
pub enum Bech32Encoding: u32 {
    Bech32 = 1,
    Bech32m = 0x2bc830a3,
}

/// Decoded Bech32/Bech32m payload.
///
/// Note: `hrp` is normalized to lowercase ASCII, and `data` contains the raw 5-bit
/// data words (each in `0..31`) excluding the 6-character checksum. Use
/// `Bech32.convertBits(allocator, decoded.data, 5, 8, false)` to convert 5-bit
/// words into 8-bit payload bytes.
pub struct Bech32Decoded {
    hrp: []u8 = [],
    data: []u8 = [],
    encoding: Bech32Encoding = Bech32Encoding.Bech32,

    /// Releases `hrp` and `data` buffers using the same `allocator` passed to `Bech32.decode`.
    fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.hrp.len > 0) {
            allocator.free(_.hrp);
        }
        if (_.data.len > 0) {
            allocator.free(_.data);
        }
        _.hrp = [];
        _.data = [];
    }
}

fn base58Val(c: u8): u8 {
    if (c >= '1' && c <= '9') return c - '1';
    if (c >= 'A' && c <= 'H') return (c - 'A') + 9;
    if (c >= 'J' && c <= 'N') return (c - 'J') + 17;
    if (c >= 'P' && c <= 'Z') return (c - 'P') + 22;
    if (c >= 'a' && c <= 'k') return (c - 'a') + 33;
    if (c >= 'm' && c <= 'z') return (c - 'm') + 44;
    return 0xFF;
}

fn doubleSha256(data: []const u8): [32]u8 {
    let first: [32]u8 = undefined;
    let second: [32]u8 = undefined;
    sha256Raw(&first, data);
    sha256Raw(&second, first[0..32]);
    return second;
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

fn createBech32Checksum(hrp_lower: []const u8, data5: []const u8, encoding: Bech32Encoding): [6]u8 {
    let chk: u32 = 1;
    for (c of hrp_lower) {
        chk = bech32PolymodStep(chk, c >> 5);
    }
    chk = bech32PolymodStep(chk, 0);
    for (c of hrp_lower) {
        chk = bech32PolymodStep(chk, c & 0x1F);
    }
    for (v of data5) {
        chk = bech32PolymodStep(chk, v);
    }
    for (_i of 0..6) {
        chk = bech32PolymodStep(chk, 0);
    }
    const pm = chk ^ (encoding as u32);
    let out: [6]u8 = undefined;
    let i: usize = 0;
    while (i < 6) : (i += 1) {
        const shift: u5 = @intCast((5 - i) * 5);
        out[i] = ((pm >> shift) & 31) as u8;
    }
    return out;
}

fn verifyBech32Checksum(hrp_lower: []const u8, data_and_chk: []const u8): Bech32Encoding? {
    let chk: u32 = 1;
    for (c of hrp_lower) {
        chk = bech32PolymodStep(chk, c >> 5);
    }
    chk = bech32PolymodStep(chk, 0);
    for (c of hrp_lower) {
        chk = bech32PolymodStep(chk, c & 0x1F);
    }
    for (v of data_and_chk) {
        chk = bech32PolymodStep(chk, v);
    }
    if (chk == 1) return Bech32Encoding.Bech32;
    if (chk == 0x2bc830a3) return Bech32Encoding.Bech32m;
    return null;
}

/// Pure CP Base58 and Base58Check codec using the Bitcoin alphabet.
pub struct Base58 {
    /// Encodes `bytes` into Base58, preserving leading `0x00` bytes as `'1'`.
    static fn encode(allocator: std.mem.Allocator, bytes: []const u8): ([]u8) ! Base58Error {
        if (bytes.len == 0) {
            return allocator.alloc(u8, 0) catch return Base58Error.OutOfMemory;
        }

        let zeros: usize = 0;
        while (zeros < bytes.len && bytes[zeros] == 0) {
            zeros += 1;
        }

        const rem_len = bytes.len - zeros;
        const cap = (rem_len * 138) / 100 + 2;
        let digits = allocator.alloc(u8, cap) catch return Base58Error.OutOfMemory;
        defer allocator.free(digits);
        @memset(digits, 0);

        let size: usize = 0;
        let idx = zeros;
        while (idx < bytes.len) : (idx += 1) {
            let carry: u32 = bytes[idx] as u32;
            let j: usize = 0;
            while (j < size || carry != 0) : (j += 1) {
                if (j < size) {
                    carry += (digits[j] as u32) * 256;
                } else {
                    size += 1;
                }
                digits[j] = (carry % 58) as u8;
                carry /= 58;
            }
        }

        const out_len = zeros + size;
        let out = allocator.alloc(u8, out_len) catch return Base58Error.OutOfMemory;
        let k: usize = 0;
        while (k < zeros) : (k += 1) {
            out[k] = '1';
        }
        let d_idx = size;
        while (d_idx > 0) {
            d_idx -= 1;
            out[k] = BASE58_ALPHABET[digits[d_idx] as usize];
            k += 1;
        }
        return out;
    }

    /// Decodes a Base58 string `str` into raw bytes, preserving leading `'1'` as `0x00`.
    static fn decode(allocator: std.mem.Allocator, str: []const u8): ([]u8) ! Base58Error {
        if (str.len == 0) {
            return allocator.alloc(u8, 0) catch return Base58Error.OutOfMemory;
        }

        let zeros: usize = 0;
        while (zeros < str.len && str[zeros] == '1') {
            zeros += 1;
        }

        const rem_len = str.len - zeros;
        const cap = (rem_len * 733) / 1000 + 2;
        let bytes_rev = allocator.alloc(u8, cap) catch return Base58Error.OutOfMemory;
        defer allocator.free(bytes_rev);
        @memset(bytes_rev, 0);

        let size: usize = 0;
        let idx = zeros;
        while (idx < str.len) : (idx += 1) {
            const val = base58Val(str[idx]);
            if (val == 0xFF) {
                return Base58Error.InvalidCharacter;
            }
            let carry: u32 = val as u32;
            let j: usize = 0;
            while (j < size || carry != 0) : (j += 1) {
                if (j < size) {
                    carry += (bytes_rev[j] as u32) * 58;
                } else {
                    size += 1;
                }
                bytes_rev[j] = (carry & 0xFF) as u8;
                carry >>= 8;
            }
        }

        const out_len = zeros + size;
        let out = allocator.alloc(u8, out_len) catch return Base58Error.OutOfMemory;
        let k: usize = 0;
        while (k < zeros) : (k += 1) {
            out[k] = 0x00;
        }
        let b_idx = size;
        while (b_idx > 0) {
            b_idx -= 1;
            out[k] = bytes_rev[b_idx];
            k += 1;
        }
        return out;
    }

    /// Encodes `version_and_payload` with a 4-byte Double-SHA256 checksum suffix (Base58Check).
    static fn encodeCheck(allocator: std.mem.Allocator, version_and_payload: []const u8): ([]u8) ! Base58Error {
        const digest = doubleSha256(version_and_payload);
        let buf = allocator.alloc(u8, version_and_payload.len + 4) catch return Base58Error.OutOfMemory;
        defer allocator.free(buf);
        if (version_and_payload.len > 0) {
            @memcpy(buf[0..version_and_payload.len], version_and_payload);
        }
        @memcpy(buf[version_and_payload.len .. version_and_payload.len + 4], digest[0..4]);
        return Base58.encode(allocator, buf);
    }

    /// Decodes Base58Check `str`, verifying the 4-byte Double-SHA256 checksum.
    static fn decodeCheck(allocator: std.mem.Allocator, str: []const u8): ([]u8) ! Base58Error {
        const raw = Base58.decode(allocator, str)?;
        defer allocator.free(raw);

        if (raw.len < 4) {
            return Base58Error.InvalidLength;
        }

        const payload_len = raw.len - 4;
        const payload = raw[0..payload_len];
        const chk = raw[payload_len..raw.len];
        const expected = doubleSha256(payload);

        let diff: u8 = 0;
        for (i of 0..4) {
            diff |= chk[i] ^ expected[i];
        }
        if (diff != 0) {
            return Base58Error.InvalidChecksum;
        }

        let out = allocator.alloc(u8, payload_len) catch return Base58Error.OutOfMemory;
        if (payload_len > 0) {
            @memcpy(out, payload);
        }
        return out;
    }
}

/// Pure CP BIP-173 (Bech32) and BIP-350 (Bech32m) encoder and decoder.
pub struct Bech32 {
    /// Converts bit groups between `from_bits` and `to_bits` (1..8).
    /// When `pad` is `false`, rejects extraneous or non-zero padding bits with `InvalidPadding`.
    static fn convertBits(
        allocator: std.mem.Allocator,
        data: []const u8,
        from_bits: u8,
        to_bits: u8,
        pad: bool,
    ): ([]u8) ! Base58Error {
        if (from_bits == 0 || from_bits > 8 || to_bits == 0 || to_bits > 8) {
            return Base58Error.InvalidLength;
        }

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);

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
                    return Base58Error.InvalidCharacter;
                }
            }
            acc = ((acc << from_shift) | (value as u32)) & max_acc;
            bits += from_bits;
            while (bits >= to_bits) {
                bits -= to_bits;
                const b_shift: u5 = @intCast(bits);
                const item: u8 = ((acc >> b_shift) & maxv) as u8;
                out.append(allocator, item) catch return Base58Error.OutOfMemory;
            }
        }

        if (pad) {
            if (bits > 0) {
                const shift: u5 = @intCast(to_bits - bits);
                const item: u8 = ((acc << shift) & maxv) as u8;
                out.append(allocator, item) catch return Base58Error.OutOfMemory;
            }
        } else {
            if (bits >= from_bits) {
                return Base58Error.InvalidPadding;
            }
            if (bits > 0) {
                const shift: u5 = @intCast(to_bits - bits);
                if (((acc << shift) & maxv) != 0) {
                    return Base58Error.InvalidPadding;
                }
            }
        }

        return out.toOwnedSlice(allocator) catch return Base58Error.OutOfMemory;
    }

    /// Encodes `hrp` and 5-bit `data5` words into a Bech32 or Bech32m string.
    static fn encode(
        allocator: std.mem.Allocator,
        hrp: []const u8,
        data5: []const u8,
        encoding: Bech32Encoding,
    ): ([]u8) ! Base58Error {
        if (hrp.len == 0 || hrp.len > 83) {
            return Base58Error.InvalidHrp;
        }
        const total_len = hrp.len + 1 + data5.len + 6;
        if (total_len > 90) {
            return Base58Error.InvalidLength;
        }

        let has_lower = false;
        let has_upper = false;
        for (c of hrp) {
            if (c < 33 || c > 126) {
                return Base58Error.InvalidHrp;
            }
            if (c >= 'a' && c <= 'z') has_lower = true;
            if (c >= 'A' && c <= 'Z') has_upper = true;
        }
        if (has_lower && has_upper) {
            return Base58Error.MixedCase;
        }

        for (v of data5) {
            if (v >= 32) {
                return Base58Error.InvalidCharacter;
            }
        }

        let out = allocator.alloc(u8, total_len) catch return Base58Error.OutOfMemory;
        errdefer allocator.free(out);

        for (i of 0..hrp.len) {
            let c = hrp[i];
            if (c >= 'A' && c <= 'Z') {
                c += 32;
            }
            out[i] = c;
        }
        const hrp_lower = out[0..hrp.len];
        const chk = createBech32Checksum(hrp_lower, data5, encoding);

        out[hrp.len] = '1';
        let pos = hrp.len + 1;
        for (v of data5) {
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

    /// Decodes a Bech32 or Bech32m string, returning lowercase `hrp`, 5-bit `data`, and detected `encoding`.
    static fn decode(allocator: std.mem.Allocator, str: []const u8): Bech32Decoded ! Base58Error {
        if (str.len < 8 || str.len > 90) {
            return Base58Error.InvalidLength;
        }

        let has_lower = false;
        let has_upper = false;
        let sep_pos_opt: usize? = null;

        let idx: usize = 0;
        while (idx < str.len) : (idx += 1) {
            const c = str[idx];
            if (c >= 'a' && c <= 'z') has_lower = true;
            if (c >= 'A' && c <= 'Z') has_upper = true;
            if (c == '1') {
                sep_pos_opt = idx;
            }
        }

        if (has_lower && has_upper) {
            return Base58Error.MixedCase;
        }

        if (sep_pos_opt == null) {
            return Base58Error.MissingSeparator;
        }
        const sep_pos = sep_pos_opt.?;
        if (sep_pos == 0 || sep_pos > 83) {
            return Base58Error.InvalidHrp;
        }

        let h_check: usize = 0;
        while (h_check < sep_pos) : (h_check += 1) {
            const c = str[h_check];
            if (c < 33 || c > 126) {
                return Base58Error.InvalidHrp;
            }
        }

        if (sep_pos + 7 > str.len) {
            return Base58Error.InvalidLength;
        }

        let hrp_lower = allocator.alloc(u8, sep_pos) catch return Base58Error.OutOfMemory;
        errdefer allocator.free(hrp_lower);
        let h_idx: usize = 0;
        while (h_idx < sep_pos) : (h_idx += 1) {
            let c = str[h_idx];
            if (c >= 'A' && c <= 'Z') {
                c += 32;
            }
            hrp_lower[h_idx] = c;
        }

        const data_total = str.len - sep_pos - 1;
        let all_vals = allocator.alloc(u8, data_total) catch return Base58Error.OutOfMemory;
        defer allocator.free(all_vals);

        let d_idx: usize = 0;
        while (d_idx < data_total) : (d_idx += 1) {
            const v = bech32CharVal(str[sep_pos + 1 + d_idx]);
            if (v == 0xFF) {
                return Base58Error.InvalidCharacter;
            }
            all_vals[d_idx] = v;
        }

        const enc_opt = verifyBech32Checksum(hrp_lower, all_vals);
        if (enc_opt == null) {
            return Base58Error.InvalidChecksum;
        }
        const encoding = enc_opt.?;
        const payload_len = data_total - 6;
        let payload = allocator.alloc(u8, payload_len) catch return Base58Error.OutOfMemory;
        if (payload_len > 0) {
            @memcpy(payload, all_vals[0..payload_len]);
        }

        return Bech32Decoded {
            hrp: hrp_lower,
            data: payload,
            encoding: encoding,
        };
    }
}
