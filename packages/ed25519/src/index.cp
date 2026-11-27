// @cap/ed25519 — RFC 8032 Ed25519 signature & key containers, canonical S < L check,
// RFC 8410 SPKI / PKCS#8 DER (OID 1.3.101.112) & hex codecs in pure CP.
// Ported from crates.io:ed25519 (https://github.com/RustCrypto/signatures)

pub const SIGNATURE_LENGTH: usize = 64;
pub const KEY_LENGTH: usize = 32;
pub const SPKI_DER_LENGTH: usize = 44;
pub const PKCS8_DER_LENGTH: usize = 48;

fn ed25Zero32(): [32]u8 {
    const z: u8 = 0;
    const arr: [32]u8 = [
        z, z, z, z, z, z, z, z, z, z, z, z, z, z, z, z,
        z, z, z, z, z, z, z, z, z, z, z, z, z, z, z, z,
    ];
    return arr;
}

fn ed25Zero64(): [64]u8 {
    const z: u8 = 0;
    const arr: [64]u8 = [
        z, z, z, z, z, z, z, z, z, z, z, z, z, z, z, z,
        z, z, z, z, z, z, z, z, z, z, z, z, z, z, z, z,
        z, z, z, z, z, z, z, z, z, z, z, z, z, z, z, z,
        z, z, z, z, z, z, z, z, z, z, z, z, z, z, z, z,
    ];
    return arr;
}

fn ed25GroupOrderL(): [32]u8 {
    // RFC 8032 Ed25519 subgroup order L = 2^252 + 27742317777372353535851937790883648493 (little-endian)
    const arr: [32]u8 = [
        0xED as u8, 0xD3 as u8, 0xF5 as u8, 0x5C as u8,
        0x1A as u8, 0x63 as u8, 0x12 as u8, 0x58 as u8,
        0xD6 as u8, 0x9C as u8, 0xF7 as u8, 0xA2 as u8,
        0xDE as u8, 0xF9 as u8, 0xDE as u8, 0x14 as u8,
        0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        0x00 as u8, 0x00 as u8, 0x00 as u8, 0x10 as u8,
    ];
    return arr;
}

fn ed25SpkiPrefix(): [12]u8 {
    // SEQUENCE(42) { SEQUENCE(5) { OID 1.3.101.112 }, BIT STRING(33, 0 unused bits) }
    const arr: [12]u8 = [
        0x30 as u8, 0x2A as u8,
        0x30 as u8, 0x05 as u8,
        0x06 as u8, 0x03 as u8, 0x2B as u8, 0x65 as u8, 0x70 as u8,
        0x03 as u8, 0x21 as u8, 0x00 as u8,
    ];
    return arr;
}

fn ed25Pkcs8Prefix(): [16]u8 {
    // SEQUENCE(46) { INTEGER 0, SEQUENCE(5) { OID 1.3.101.112 }, OCTET STRING(34) { OCTET STRING(32) } }
    const arr: [16]u8 = [
        0x30 as u8, 0x2E as u8,
        0x02 as u8, 0x01 as u8, 0x00 as u8,
        0x30 as u8, 0x05 as u8,
        0x06 as u8, 0x03 as u8, 0x2B as u8, 0x65 as u8, 0x70 as u8,
        0x04 as u8, 0x22 as u8,
        0x04 as u8, 0x20 as u8,
    ];
    return arr;
}

fn ed25HexNibble(ch: u8): i32 {
    if (ch >= 0x30 && ch <= 0x39) return (ch - 0x30) as i32;
    if (ch >= 0x61 && ch <= 0x66) return (ch - 0x61 + 10) as i32;
    if (ch >= 0x41 && ch <= 0x46) return (ch - 0x41 + 10) as i32;
    return -1;
}

fn ed25HexDigit(n: u8): u8 {
    if (n < 10) return 0x30 + n;
    return 0x61 + (n - 10);
}

fn ed25EncodeHex(src: []const u8, out: []u8): usize {
    const need = src.len * 2;
    if (out.len < need) return 0;
    let i: usize = 0;
    while (i < src.len) {
        const b = src[i];
        out[i * 2] = ed25HexDigit((b >> 4) & 0x0F);
        out[i * 2 + 1] = ed25HexDigit(b & 0x0F);
        i += 1;
    }
    return need;
}

/// 64-byte RFC 8032 Ed25519 signature container (R || S).
pub struct Ed25519Signature {
    bytes: [64]u8,

    static fn fromBytes(b: [64]u8): Ed25519Signature {
        return Ed25519Signature{ bytes: b };
    }

    static fn fromSlice(s: []const u8): Ed25519Signature? {
        if (s.len != SIGNATURE_LENGTH) return null;
        let arr: [64]u8 = ed25Zero64();
        let i: usize = 0;
        while (i < 64) {
            arr[i] = s[i];
            i += 1;
        }
        return Ed25519Signature{ bytes: arr };
    }

    static fn fromComponents(r: [32]u8, s: [32]u8): Ed25519Signature {
        let arr: [64]u8 = ed25Zero64();
        let i: usize = 0;
        while (i < 32) {
            arr[i] = r[i];
            arr[32 + i] = s[i];
            i += 1;
        }
        return Ed25519Signature{ bytes: arr };
    }

    fn rBytes(): [32]u8 {
        let out: [32]u8 = ed25Zero32();
        let i: usize = 0;
        while (i < 32) {
            out[i] = _.bytes[i];
            i += 1;
        }
        return out;
    }

    fn sBytes(): [32]u8 {
        let out: [32]u8 = ed25Zero32();
        let i: usize = 0;
        while (i < 32) {
            out[i] = _.bytes[32 + i];
            i += 1;
        }
        return out;
    }

    fn toBytes(): [64]u8 {
        const out: [64]u8 = _.bytes;
        return out;
    }

    /// Strict RFC 8032 malleability check: returns true iff scalar S < L.
    fn isCanonicalS(): bool {
        const order = ed25GroupOrderL();
        let k: usize = 0;
        while (k < 32) {
            const idx: usize = 31 - k;
            const sb: u8 = _.bytes[32 + idx];
            const lb: u8 = order[idx];
            if (sb < lb) return true;
            if (sb > lb) return false;
            k += 1;
        }
        // S == L is non-canonical
        return false;
    }

    static fn fromHex(hex: string): Ed25519Signature? {
        if (hex.len != 128) return null;
        let arr: [64]u8 = ed25Zero64();
        let i: usize = 0;
        while (i < 64) {
            const hi = ed25HexNibble(hex[i * 2]);
            const lo = ed25HexNibble(hex[i * 2 + 1]);
            if (hi < 0 || lo < 0) return null;
            arr[i] = ((hi as u8) << 4) | (lo as u8);
            i += 1;
        }
        return Ed25519Signature{ bytes: arr };
    }

    fn toHex(out: []u8): usize {
        const local: [64]u8 = _.bytes;
        return ed25EncodeHex(local[0..64], out);
    }

    fn eql(other: Ed25519Signature): bool {
        let diff: u8 = 0;
        let i: usize = 0;
        while (i < 64) {
            diff |= _.bytes[i] ^ other.bytes[i];
            i += 1;
        }
        return diff == 0;
    }
}

/// 32-byte RFC 8032 Ed25519 public key with RFC 8410 SPKI DER codec.
pub struct Ed25519PublicKey {
    bytes: [32]u8,

    static fn fromBytes(b: [32]u8): Ed25519PublicKey {
        return Ed25519PublicKey{ bytes: b };
    }

    static fn fromSlice(s: []const u8): Ed25519PublicKey? {
        if (s.len != KEY_LENGTH) return null;
        let arr: [32]u8 = ed25Zero32();
        let i: usize = 0;
        while (i < 32) {
            arr[i] = s[i];
            i += 1;
        }
        return Ed25519PublicKey{ bytes: arr };
    }

    fn toBytes(): [32]u8 {
        const out: [32]u8 = _.bytes;
        return out;
    }

    fn toSpkiDer(out: []u8): usize {
        if (out.len < SPKI_DER_LENGTH) return 0;
        const prefix = ed25SpkiPrefix();
        let i: usize = 0;
        while (i < 12) {
            out[i] = prefix[i];
            i += 1;
        }
        i = 0;
        while (i < 32) {
            out[12 + i] = _.bytes[i];
            i += 1;
        }
        return SPKI_DER_LENGTH;
    }

    static fn fromSpkiDer(der: []const u8): Ed25519PublicKey? {
        if (der.len != SPKI_DER_LENGTH) return null;
        const prefix = ed25SpkiPrefix();
        let i: usize = 0;
        while (i < 12) {
            if (der[i] != prefix[i]) return null;
            i += 1;
        }
        let arr: [32]u8 = ed25Zero32();
        i = 0;
        while (i < 32) {
            arr[i] = der[12 + i];
            i += 1;
        }
        return Ed25519PublicKey{ bytes: arr };
    }

    static fn fromHex(hex: string): Ed25519PublicKey? {
        if (hex.len != 64) return null;
        let arr: [32]u8 = ed25Zero32();
        let i: usize = 0;
        while (i < 32) {
            const hi = ed25HexNibble(hex[i * 2]);
            const lo = ed25HexNibble(hex[i * 2 + 1]);
            if (hi < 0 || lo < 0) return null;
            arr[i] = ((hi as u8) << 4) | (lo as u8);
            i += 1;
        }
        return Ed25519PublicKey{ bytes: arr };
    }

    fn toHex(out: []u8): usize {
        const local: [32]u8 = _.bytes;
        return ed25EncodeHex(local[0..32], out);
    }

    fn eql(other: Ed25519PublicKey): bool {
        let diff: u8 = 0;
        let i: usize = 0;
        while (i < 32) {
            diff |= _.bytes[i] ^ other.bytes[i];
            i += 1;
        }
        return diff == 0;
    }
}

/// 32-byte RFC 8032 Ed25519 secret key (seed) with RFC 8410 PKCS#8 v1 DER codec.
pub struct Ed25519SecretKey {
    bytes: [32]u8,

    static fn fromBytes(b: [32]u8): Ed25519SecretKey {
        return Ed25519SecretKey{ bytes: b };
    }

    static fn fromSlice(s: []const u8): Ed25519SecretKey? {
        if (s.len != KEY_LENGTH) return null;
        let arr: [32]u8 = ed25Zero32();
        let i: usize = 0;
        while (i < 32) {
            arr[i] = s[i];
            i += 1;
        }
        return Ed25519SecretKey{ bytes: arr };
    }

    fn toBytes(): [32]u8 {
        const out: [32]u8 = _.bytes;
        return out;
    }

    fn toPkcs8Der(out: []u8): usize {
        if (out.len < PKCS8_DER_LENGTH) return 0;
        const prefix = ed25Pkcs8Prefix();
        let i: usize = 0;
        while (i < 16) {
            out[i] = prefix[i];
            i += 1;
        }
        i = 0;
        while (i < 32) {
            out[16 + i] = _.bytes[i];
            i += 1;
        }
        return PKCS8_DER_LENGTH;
    }

    static fn fromPkcs8Der(der: []const u8): Ed25519SecretKey? {
        if (der.len != PKCS8_DER_LENGTH) return null;
        const prefix = ed25Pkcs8Prefix();
        let i: usize = 0;
        while (i < 16) {
            if (der[i] != prefix[i]) return null;
            i += 1;
        }
        let arr: [32]u8 = ed25Zero32();
        i = 0;
        while (i < 32) {
            arr[i] = der[16 + i];
            i += 1;
        }
        return Ed25519SecretKey{ bytes: arr };
    }

    static fn fromHex(hex: string): Ed25519SecretKey? {
        if (hex.len != 64) return null;
        let arr: [32]u8 = ed25Zero32();
        let i: usize = 0;
        while (i < 32) {
            const hi = ed25HexNibble(hex[i * 2]);
            const lo = ed25HexNibble(hex[i * 2 + 1]);
            if (hi < 0 || lo < 0) return null;
            arr[i] = ((hi as u8) << 4) | (lo as u8);
            i += 1;
        }
        return Ed25519SecretKey{ bytes: arr };
    }

    fn toHex(out: []u8): usize {
        const local: [32]u8 = _.bytes;
        return ed25EncodeHex(local[0..32], out);
    }

    fn eql(other: Ed25519SecretKey): bool {
        let diff: u8 = 0;
        let i: usize = 0;
        while (i < 32) {
            diff |= _.bytes[i] ^ other.bytes[i];
            i += 1;
        }
        return diff == 0;
    }
}
