// Source: cp/lib/uuid/uuid.cp (RFC 9562 UUID v4, v5 SHA-1 & v7 Time-Ordered)
// Reference-hardened against uuid-rs/uuid (MIT/Apache-2.0)
// lib/uuid/uuid.cp
// Pure CP RFC 9562 / RFC 4122 UUID v4, v5 (SHA-1) & v7 (Time-Ordered Monotonic) engine
// with 16-lane @Vector(16, u8) SIMD hot paths for hex validation, case normalization,
// 16-byte equality, and hex formatting.

const std = @import("std");

type Vec16u8 = @Vector(16, u8);
type Vec16u3 = @Vector(16, u3);

pub const VARIANT_NCS: u8 = 0;
pub const VARIANT_RFC4122: u8 = 2;
pub const VARIANT_RFC9562: u8 = 2;
pub const VARIANT_MICROSOFT: u8 = 6;
pub const VARIANT_FUTURE: u8 = 7;

pub const VERSION_NIL: u8 = 0;
pub const VERSION_MAC: u8 = 1;
pub const VERSION_DCE: u8 = 2;
pub const VERSION_MD5: u8 = 3;
pub const VERSION_RANDOM: u8 = 4;
pub const VERSION_SHA1: u8 = 5;
pub const VERSION_SORT_MAC: u8 = 6;
pub const VERSION_SORT_RAND: u8 = 7;
pub const VERSION_CUSTOM: u8 = 8;
pub const VERSION_MAX: u8 = 15;

fn fillCsprng(buf: []u8) {
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

fn currentUnixMs(): u64 {
    let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
    * = std.os.linux.clock_gettime(.REALTIME, &ts);
    const sec_i64: i64 = ts.sec as i64;
    const nsec_i64: i64 = ts.nsec as i64;
    if (sec_i64 < 0) return 0;
    const sec_u64 = u64(sec_i64);
    const ms_part = u64(@divTrunc(nsec_i64, 1000000));
    return sec_u64 * 1000 + ms_part;
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return false;
    }
    return true;
}

/// 16-lane `@Vector(16, u8)` SIMD equality comparison for 16-byte UUID payloads.
pub fn simdEql16(a: *const [16]u8, b: *const [16]u8): bool {
    const va: Vec16u8 = a.*;
    const vb: Vec16u8 = b.*;
    return @reduce(.And, va == vb);
}

/// 16-lane `@Vector(16, u8)` SIMD ASCII hexadecimal pre-check for a 16-byte chunk.
pub fn simdIsHex16(chunk: *const [16]u8): bool {
    const v: Vec16u8 = chunk.*;
    const v_zero: Vec16u8 = @splat('0');
    const v_a: Vec16u8 = @splat('a');
    const v_case: Vec16u8 = @splat(0x20);
    const v_10: Vec16u8 = @splat(10);
    const v_6: Vec16u8 = @splat(6);
    const v_one: Vec16u8 = @splat(1);
    const v_z: Vec16u8 = @splat(0);

    const digit: Vec16u8 = v -% v_zero;
    const alpha: Vec16u8 = (v | v_case) -% v_a;
    const is_digit = digit < v_10;
    const is_alpha = alpha < v_6;

    const ok_alpha: Vec16u8 = @select(u8, is_alpha, v_one, v_z);
    const ok: Vec16u8 = @select(u8, is_digit, v_one, ok_alpha);
    return @reduce(.And, ok == v_one);
}

/// 16-lane `@Vector(16, u8)` SIMD ASCII hexadecimal pre-check for a 32-byte simple UUID slice.
pub fn simdIsHex32(input: []const u8): bool {
    if (input.len != 32) return false;
    let c0: [16]u8 = undefined;
    let c1: [16]u8 = undefined;
    @memcpy(&c0, input[0..16]);
    @memcpy(&c1, input[16..32]);
    return simdIsHex16(&c0) && simdIsHex16(&c1);
}

/// 16-lane `@Vector(16, u8)` SIMD ASCII uppercase-to-lowercase normalization for 16 bytes.
pub fn simdAsciiToLower16(chunk: *const [16]u8): [16]u8 {
    const v: Vec16u8 = chunk.*;
    const v_A: Vec16u8 = @splat('A');
    const v_26: Vec16u8 = @splat(26);
    const v_case: Vec16u8 = @splat(0x20);
    const is_upper = (v -% v_A) < v_26;
    const lowered: Vec16u8 = @select(u8, is_upper, v | v_case, v);
    return lowered;
}

/// 16-lane `@Vector(16, u8)` SIMD lowercase normalization + hex validation for 32-byte simple UUIDs.
/// Returns `true` and writes 32 lowercase hex bytes to `out` iff all 32 input bytes are valid ASCII hex.
pub fn simdNormalizeHexLower32(input: []const u8, out: *[32]u8): bool {
    if (input.len != 32) return false;
    let c0: [16]u8 = undefined;
    let c1: [16]u8 = undefined;
    @memcpy(&c0, input[0..16]);
    @memcpy(&c1, input[16..32]);
    if (!simdIsHex16(&c0) || !simdIsHex16(&c1)) return false;
    const l0 = simdAsciiToLower16(&c0);
    const l1 = simdAsciiToLower16(&c1);
    @memcpy(out[0..16], &l0);
    @memcpy(out[16..32], &l1);
    return true;
}

/// 16-lane `@Vector(16, u8)` SIMD decoding of 16 ASCII hex characters into 16 4-bit nibbles.
fn simdDecodeHexChunk16(chunk: *const [16]u8, out: *[16]u8): bool {
    const v: Vec16u8 = chunk.*;
    const v_zero: Vec16u8 = @splat('0');
    const v_a: Vec16u8 = @splat('a');
    const v_case: Vec16u8 = @splat(0x20);
    const v_10: Vec16u8 = @splat(10);
    const v_6: Vec16u8 = @splat(6);
    const v_one: Vec16u8 = @splat(1);
    const v_z: Vec16u8 = @splat(0);

    const digit: Vec16u8 = v -% v_zero;
    const alpha: Vec16u8 = (v | v_case) -% v_a;
    const is_digit = digit < v_10;
    const is_alpha = alpha < v_6;

    const ok_alpha: Vec16u8 = @select(u8, is_alpha, v_one, v_z);
    const ok: Vec16u8 = @select(u8, is_digit, v_one, ok_alpha);
    if (!@reduce(.And, ok == v_one)) {
        return false;
    }

    const alpha_val: Vec16u8 = alpha +% v_10;
    const nibbles: Vec16u8 = @select(u8, is_digit, digit, alpha_val);
    const arr: [16]u8 = nibbles;
    @memcpy(out, &arr);
    return true;
}

/// 16-lane `@Vector(16, u8)` SIMD decoding of 32 contiguous ASCII hex bytes into 16 UUID bytes.
/// Returns `true` and writes 16 decoded bytes to `out` iff all 32 characters are valid ASCII hex.
pub fn simdDecodeHex32(hex: *const [32]u8, out: *[16]u8): bool {
    let n0: [16]u8 = undefined;
    let n1: [16]u8 = undefined;
    let c0: [16]u8 = undefined;
    let c1: [16]u8 = undefined;
    @memcpy(&c0, hex[0..16]);
    @memcpy(&c1, hex[16..32]);
    if (!simdDecodeHexChunk16(&c0, &n0)) return false;
    if (!simdDecodeHexChunk16(&c1, &n1)) return false;

    for (i of 0..8) {
        out[i] = (n0[i * 2] << 4) | n0[i * 2 + 1];
    }
    for (i of 0..8) {
        out[8 + i] = (n1[i * 2] << 4) | n1[i * 2 + 1];
    }
    return true;
}

/// 16-lane `@Vector(16, u8)` SIMD hex encoding of 16 UUID bytes into 32 ASCII hex bytes.
pub fn simdFormatHex32(bytes: *const [16]u8, out: *[32]u8, upper: bool) {
    const v: Vec16u8 = bytes.*;
    const v_shift4: Vec16u3 = @splat(4);
    const v_mask0f: Vec16u8 = @splat(0x0F);
    const v_10: Vec16u8 = @splat(10);
    const v_0: Vec16u8 = @splat('0');
    const alpha_base_u8: u8 = if (upper) (55 as u8) else (87 as u8);
    const v_alpha_base: Vec16u8 = @splat(alpha_base_u8);

    const hi_nib: Vec16u8 = v >> v_shift4;
    const lo_nib: Vec16u8 = v & v_mask0f;

    const hi_ascii_vec: Vec16u8 = @select(u8, hi_nib < v_10, hi_nib + v_0, hi_nib + v_alpha_base);
    const lo_ascii_vec: Vec16u8 = @select(u8, lo_nib < v_10, lo_nib + v_0, lo_nib + v_alpha_base);

    const hi_arr: [16]u8 = hi_ascii_vec;
    const lo_arr: [16]u8 = lo_ascii_vec;

    for (i of 0..16) {
        out[i * 2] = hi_arr[i];
        out[i * 2 + 1] = lo_arr[i];
    }
}

fn parseHyphenatedInner(s: []const u8, out: *[16]u8): bool {
    if (s.len != 36) return false;
    if (s[8] != '-' || s[13] != '-' || s[18] != '-' || s[23] != '-') {
        return false;
    }

    let hex: [32]u8 = undefined;
    @memcpy(hex[0..8], s[0..8]);
    @memcpy(hex[8..12], s[9..13]);
    @memcpy(hex[12..16], s[14..18]);
    @memcpy(hex[16..20], s[19..23]);
    @memcpy(hex[20..32], s[24..36]);
    return simdDecodeHex32(&hex, out);
}

let v7_state_lock: std.atomic.Value(u32) = std.atomic.Value(u32).init(0);
let v7_last_ms: u64 = 0;
let v7_last_rand_a: u16 = 0;
let v7_initialized: bool = false;

pub struct Uuid {
    bytes: [16]u8 = [0; 16];

    /// Returns the Nil UUID (`00000000-0000-0000-0000-000000000000`, RFC 9562 Section 5.9).
    static fn nil(): Uuid {
        return Uuid { bytes: [0; 16] };
    }

    /// Returns the Max UUID (`ffffffff-ffff-ffff-ffff-ffffffffffff`, RFC 9562 Section 5.10).
    static fn max(): Uuid {
        return Uuid { bytes: [0xFF; 16] };
    }

    /// Constructs a `Uuid` directly from a 16-byte big-endian array.
    static fn fromBytes(raw: [16]u8): Uuid {
        return Uuid { bytes: raw };
    }

    /// Constructs a `Uuid` from a 128-bit unsigned integer (big-endian layout, matching `uuid-rs`).
    static fn fromU128(v: u128): Uuid {
        let b: [16]u8 = undefined;
        std.mem.writeInt(u128, &b, v, .big);
        return Uuid { bytes: b };
    }

    /// Generates an RFC 9562 UUID v4 using Linux `getrandom` CSPRNG.
    static fn v4(): Uuid {
        let b: [16]u8 = undefined;
        fillCsprng(&b);
        b[6] = (b[6] & 0x0F) | 0x40;
        b[8] = (b[8] & 0x3F) | 0x80;
        return Uuid { bytes: b };
    }

    /// Generates a deterministic RFC 9562 UUID v5 using SHA-1(namespace || name).
    static fn v5(namespace: Uuid, name: []const u8): Uuid {
        let hasher = std.crypto.hash.Sha1.init(.{});
        hasher.update(&namespace.bytes);
        hasher.update(name);
        let digest: [20]u8 = undefined;
        hasher.final(&digest);

        let b: [16]u8 = undefined;
        @memcpy(b[0..16], digest[0..16]);
        b[6] = (b[6] & 0x0F) | 0x50;
        b[8] = (b[8] & 0x3F) | 0x80;
        return Uuid { bytes: b };
    }

    /// Generates a stateless RFC 9562 UUID v7 at an explicit Unix epoch millisecond timestamp.
    static fn v7At(unix_ms: u64): Uuid {
        let b: [16]u8 = undefined;
        fillCsprng(&b);

        const ts = unix_ms & 0x0000_FFFF_FFFF_FFFF;
        b[0] = ((ts >> 40) & 0xFF) as u8;
        b[1] = ((ts >> 32) & 0xFF) as u8;
        b[2] = ((ts >> 24) & 0xFF) as u8;
        b[3] = ((ts >> 16) & 0xFF) as u8;
        b[4] = ((ts >> 8) & 0xFF) as u8;
        b[5] = (ts & 0xFF) as u8;

        b[6] = (b[6] & 0x0F) | 0x70;
        b[8] = (b[8] & 0x3F) | 0x80;
        return Uuid { bytes: b };
    }

    /// Generates a strictly monotonic, thread-safe RFC 9562 UUID v7 using current wall-clock time.
    static fn v7(): Uuid {
        let b: [16]u8 = undefined;
        fillCsprng(&b);

        const raw_rand_a: u16 = ((b[6] as u16) << 8) | (b[7] as u16);
        const seeded_rand_a: u16 = raw_rand_a & 0x07FF; // Keep MSB 0 for monotonic headroom

        while (v7_state_lock.cmpxchgWeak(0, 1, .acquire, .monotonic) != null) {
            std.atomic.spinLoopHint();
        }
        defer v7_state_lock.store(0, .release);

        let ts: u64 = currentUnixMs() & 0x0000_FFFF_FFFF_FFFF;
        let rand_a: u16 = seeded_rand_a;

        if (!v7_initialized || ts > v7_last_ms) {
            v7_last_ms = ts;
            v7_last_rand_a = seeded_rand_a;
            v7_initialized = true;
        } else {
            let next_rand = v7_last_rand_a + 1;
            if (next_rand > 0x0FFF) {
                v7_last_ms = (v7_last_ms + 1) & 0x0000_FFFF_FFFF_FFFF;
                next_rand = seeded_rand_a;
            }
            v7_last_rand_a = next_rand;
            ts = v7_last_ms;
            rand_a = next_rand;
        }

        b[0] = ((ts >> 40) & 0xFF) as u8;
        b[1] = ((ts >> 32) & 0xFF) as u8;
        b[2] = ((ts >> 24) & 0xFF) as u8;
        b[3] = ((ts >> 16) & 0xFF) as u8;
        b[4] = ((ts >> 8) & 0xFF) as u8;
        b[5] = (ts & 0xFF) as u8;

        b[6] = ((0x70 | ((rand_a >> 8) & 0x0F)) as u8);
        b[7] = (rand_a & 0xFF) as u8;
        b[8] = (b[8] & 0x3F) | 0x80;
        return Uuid { bytes: b };
    }

    /// Parses a strict 32-character Simple (non-hyphenated) hexadecimal UUID string.
    static fn parseSimple(input: []const u8): Uuid? {
        if (input.len != 32) return null;
        let hex: [32]u8 = undefined;
        @memcpy(&hex, input);
        let raw: [16]u8 = undefined;
        if (!simdDecodeHex32(&hex, &raw)) return null;
        return Uuid { bytes: raw };
    }

    /// Parses a strict 36-character Hyphenated (`8-4-4-4-12`) UUID string.
    static fn parseHyphenated(input: []const u8): Uuid? {
        let raw: [16]u8 = undefined;
        if (!parseHyphenatedInner(input, &raw)) return null;
        return Uuid { bytes: raw };
    }

    /// Parses a strict 38-character Braced (`{8-4-4-4-12}`) Microsoft GUID / UUID string.
    static fn parseBraced(input: []const u8): Uuid? {
        if (input.len != 38) return null;
        if (input[0] != '{' || input[37] != '}') return null;
        let raw: [16]u8 = undefined;
        if (!parseHyphenatedInner(input[1..37], &raw)) return null;
        return Uuid { bytes: raw };
    }

    /// Parses a strict 45-character URN (`urn:uuid:8-4-4-4-12`, case-insensitive prefix) UUID string.
    static fn parseUrn(input: []const u8): Uuid? {
        if (input.len != 45) return null;
        if (!asciiEqlIgnoreCase(input[0..9], "urn:uuid:")) return null;
        let raw: [16]u8 = undefined;
        if (!parseHyphenatedInner(input[9..45], &raw)) return null;
        return Uuid { bytes: raw };
    }

    /// Parses any of the 4 standard UUID text formats matching `uuid-rs/uuid` strict rules:
    /// - Simple (32B): `67e5504410b1426f9247bb680e5fe0c8`
    /// - Hyphenated (36B): `67e55044-10b1-426f-9247-bb680e5fe0c8`
    /// - Braced (38B): `{67e55044-10b1-426f-9247-bb680e5fe0c8}`
    /// - URN (45B, case-insensitive prefix): `urn:uuid:67e55044-10b1-426f-9247-bb680e5fe0c8`
    static fn parse(input: []const u8): Uuid? {
        return switch (input.len) {
            32 => Uuid.parseSimple(input),
            36 => Uuid.parseHyphenated(input),
            38 => Uuid.parseBraced(input),
            45 => Uuid.parseUrn(input),
            else => null,
        };
    }

    /// Returns a copy of the underlying 16-byte big-endian representation.
    fn asBytes(): [16]u8 {
        return _.bytes;
    }

    /// Returns the UUID interpreted as a big-endian 128-bit unsigned integer.
    fn asU128(): u128 {
        const val = std.mem.readInt(u128, &_.bytes, .big);
        return val;
    }

    /// Returns true if all 16 bytes are zero (16-lane SIMD accelerated).
    fn isNil(): bool {
        const v: Vec16u8 = _.bytes;
        const z: Vec16u8 = @splat(0);
        return @reduce(.And, v == z);
    }

    /// Returns true if all 16 bytes are 0xFF (16-lane SIMD accelerated).
    fn isMax(): bool {
        const v: Vec16u8 = _.bytes;
        const m: Vec16u8 = @splat(0xFF);
        return @reduce(.And, v == m);
    }

    /// Returns the raw 4-bit version nibble (e.g. 4, 5, 7).
    fn version(): u8 {
        return (_.bytes[6] >> 4) & 0x0F;
    }

    /// Returns the RFC 9562 variant code (2 for standard RFC 4122/9562, 0 for NCS, 6 for Microsoft, 7 for Future).
    fn variant(): u8 {
        const b = _.bytes[8];
        if ((b & 0x80) == 0) return VARIANT_NCS;
        if ((b & 0xC0) == 0x80) return VARIANT_RFC9562;
        if ((b & 0xE0) == 0xC0) return VARIANT_MICROSOFT;
        return VARIANT_FUTURE;
    }

    /// Returns the RFC 9562 variant code (alias matching `uuid-rs/uuid` `get_variant()`).
    fn getVariant(): u8 {
        return _.variant();
    }

    /// Returns the validated RFC 9562 version (matching `uuid-rs/uuid` `get_version()`):
    /// `VERSION_NIL` (0) for Nil UUID, `VERSION_MAX` (15) for Max UUID,
    /// `1..=8` when `variant() == VARIANT_RFC9562`, otherwise `null`.
    fn getVersion(): u8? {
        if (_.isNil()) return VERSION_NIL;
        if (_.isMax()) return VERSION_MAX;
        if (_.variant() != VARIANT_RFC9562) return null;
        const v = _.version();
        if (v >= 1 && v <= 8) return v;
        return null;
    }

    /// Extracts the 48-bit Unix epoch millisecond timestamp if this UUID is a valid RFC 9562 version 7;
    /// otherwise returns null.
    fn extractV7TimestampMs(): u64? {
        if (_.version() != 7 || _.variant() != VARIANT_RFC9562) return null;
        return ((_.bytes[0] as u64) << 40)
            | ((_.bytes[1] as u64) << 32)
            | ((_.bytes[2] as u64) << 24)
            | ((_.bytes[3] as u64) << 16)
            | ((_.bytes[4] as u64) << 8)
            | (_.bytes[5] as u64);
    }

    /// Returns true if both UUIDs have identical 16-byte representations (16-lane SIMD accelerated).
    fn eql(other: Uuid): bool {
        const eq = simdEql16(&_.bytes, &other.bytes);
        return eq;
    }

    /// Lexicographical unsigned byte comparison: -1 if less, 0 if equal, 1 if greater.
    fn cmp(other: Uuid): i32 {
        for (i of 0..16) {
            if (_.bytes[i] < other.bytes[i]) return -1;
            if (_.bytes[i] > other.bytes[i]) return 1;
        }
        return 0;
    }

    fn formatHex(buf: []u8, upper: bool): []const u8 {
        if (buf.len < 36) return "";
        let hex32: [32]u8 = undefined;
        simdFormatHex32(&_.bytes, &hex32, upper);

        @memcpy(buf[0..8], hex32[0..8]);
        buf[8] = '-';
        @memcpy(buf[9..13], hex32[8..12]);
        buf[13] = '-';
        @memcpy(buf[14..18], hex32[12..16]);
        buf[18] = '-';
        @memcpy(buf[19..23], hex32[16..20]);
        buf[23] = '-';
        @memcpy(buf[24..36], hex32[20..32]);
        return buf[0..36];
    }

    /// Formats the UUID as lowercase 36-character canonical hyphenated string into `buf`.
    fn format(buf: []u8): []const u8 {
        return _.formatHex(buf, false);
    }

    /// Formats the UUID as uppercase 36-character canonical hyphenated string into `buf`.
    fn formatUpper(buf: []u8): []const u8 {
        return _.formatHex(buf, true);
    }

    /// Formats the UUID as lowercase 32-character compact/simple hex string into `buf`.
    fn formatCompact(buf: []u8): []const u8 {
        if (buf.len < 32) return "";
        let hex32: [32]u8 = undefined;
        simdFormatHex32(&_.bytes, &hex32, false);
        @memcpy(buf[0..32], &hex32);
        return buf[0..32];
    }

    /// Alias for `formatCompact`: formats as lowercase 32-character Simple hex string.
    fn formatSimple(buf: []u8): []const u8 {
        return _.formatCompact(buf);
    }

    /// Formats the UUID as uppercase 32-character Simple hex string into `buf`.
    fn formatSimpleUpper(buf: []u8): []const u8 {
        if (buf.len < 32) return "";
        let hex32: [32]u8 = undefined;
        simdFormatHex32(&_.bytes, &hex32, true);
        @memcpy(buf[0..32], &hex32);
        return buf[0..32];
    }

    /// Formats the UUID as lowercase 38-character Braced string `{8-4-4-4-12}` into `buf`.
    fn formatBraced(buf: []u8): []const u8 {
        if (buf.len < 38) return "";
        buf[0] = '{';
        * = _.formatHex(buf[1..37], false);
        buf[37] = '}';
        return buf[0..38];
    }

    /// Formats the UUID as uppercase 38-character Braced string `{8-4-4-4-12}` into `buf`.
    fn formatBracedUpper(buf: []u8): []const u8 {
        if (buf.len < 38) return "";
        buf[0] = '{';
        * = _.formatHex(buf[1..37], true);
        buf[37] = '}';
        return buf[0..38];
    }

    /// Formats the UUID as lowercase `urn:uuid:...` (45 characters) into `buf`.
    fn formatUrn(buf: []u8): []const u8 {
        if (buf.len < 45) return "";
        @memcpy(buf[0..9], "urn:uuid:");
        * = _.formatHex(buf[9..], false);
        return buf[0..45];
    }

    /// Formats the UUID as `urn:uuid:` + uppercase hyphenated UUID (45 characters) into `buf`.
    fn formatUrnUpper(buf: []u8): []const u8 {
        if (buf.len < 45) return "";
        @memcpy(buf[0..9], "urn:uuid:");
        * = _.formatHex(buf[9..], true);
        return buf[0..45];
    }

    /// Allocates and returns a canonical lowercase 36-character UUID string.
    fn toStringAlloc(alloc: std.mem.Allocator): ([]u8)! {
        let out = alloc.alloc(u8, 36)?;
        * = _.format(out);
        return out;
    }
}

pub const NIL: Uuid = Uuid { bytes: [0; 16] };
pub const MAX: Uuid = Uuid { bytes: [0xFF; 16] };

pub const NAMESPACE_DNS: Uuid = Uuid {
    bytes: [0x6b, 0xa7, 0xb8, 0x10, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8],
};

pub const NAMESPACE_URL: Uuid = Uuid {
    bytes: [0x6b, 0xa7, 0xb8, 0x11, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8],
};

pub const NAMESPACE_OID: Uuid = Uuid {
    bytes: [0x6b, 0xa7, 0xb8, 0x12, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8],
};

pub const NAMESPACE_X500: Uuid = Uuid {
    bytes: [0x6b, 0xa7, 0xb8, 0x14, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8],
};
