// Source: cp/lib/uuid/nanoid.cp (NanoID Unbiased CSPRNG Generator)
// lib/uuid/nanoid.cp
// Pure CP NanoID generator with unbiased rejection sampling (zero modulo bias)

const std = @import("std");

pub const NANOID_DEFAULT_SIZE: usize = 21;
pub const NANOID_URL_ALPHABET: []const u8 = "_-0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

fn nanoFillCsprng(buf: []u8) {
    let off: usize = 0;
    while (off < buf.len) {
        const rem = buf[off..];
        const rc = std.os.linux.getrandom(rem.ptr, rem.len, 0);
        const e = std.os.linux.errno(rc);
        if (e == .INTR) continue;
        if (e == .SUCCESS) {
            if (rc == 0) continue;
            off += rc;
            continue;
        }
        @panic("getrandom failed");
    }
}

pub static struct NanoId {
    /// Default 64-character URL-safe alphabet (`A-Za-z0-9_-`).
    const DEFAULT_ALPHABET: []const u8 = NANOID_URL_ALPHABET;
    /// Default NanoID character length (21).
    const DEFAULT_SIZE: usize = NANOID_DEFAULT_SIZE;

    /// Generates an unbiased NanoID into `buf` using `alphabet` (rejection sampling eliminates modulo bias).
    /// Supports alphabets of length 1..=256. Returns `""` if `alphabet.len == 0` or `alphabet.len > 256`.
    fn generateInto(buf: []u8, alphabet: []const u8 = NANOID_URL_ALPHABET): []const u8 {
        if (buf.len == 0) return buf[0..0];
        if (alphabet.len == 0 || alphabet.len > 256) return "";

        if (alphabet.len == 1) {
            const ch = alphabet[0];
            for (i of 0..buf.len) {
                buf[i] = ch;
            }
            return buf;
        }

        let mask_u16: u16 = 1;
        while (mask_u16 < ((alphabet.len - 1) as u16)) {
            mask_u16 = (mask_u16 << 1) | 1;
        }
        const mask: u8 = (mask_u16 & 0xFF) as u8;

        let entropy: [128]u8 = undefined;
        let written: usize = 0;
        while (written < buf.len) {
            nanoFillCsprng(&entropy);
            for (ei of 0..entropy.len) {
                const idx = (entropy[ei] & mask) as usize;
                if (idx < alphabet.len) {
                    buf[written] = alphabet[idx];
                    written += 1;
                    if (written == buf.len) break;
                }
            }
        }
        return buf;
    }

    /// Generates a default 21-character URL-safe NanoID into a fixed `[21]u8` buffer.
    fn generate(buf: *[21]u8): []const u8 {
        return NanoId.generateInto(buf, NANOID_URL_ALPHABET);
    }

    /// Allocates and generates an unbiased NanoID of length `len` using the default URL-safe alphabet.
    fn generateAlloc(alloc: std.mem.Allocator, len: usize = NANOID_DEFAULT_SIZE): ([]u8)! {
        let out = alloc.alloc(u8, len)?;
        if (len > 0) {
            * = NanoId.generateInto(out, NANOID_URL_ALPHABET);
        }
        return out;
    }

    /// Allocates and generates an unbiased NanoID of length `len` using a custom `alphabet`.
    fn generateCustomAlloc(alloc: std.mem.Allocator, len: usize, alphabet: []const u8): ([]u8)! {
        if (alphabet.len == 0 || alphabet.len > 256) {
            return err.InvalidAlphabet;
        }
        let out = alloc.alloc(u8, len)?;
        if (len > 0) {
            * = NanoId.generateInto(out, alphabet);
        }
        return out;
    }
}
