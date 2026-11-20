// Source: cp/lib/uuid/uuid.cp (RFC 9562 UUID v4, v5 SHA-1 & v7 Time-Ordered)
// lib/uuid/uuid.cp
// Pure CP RFC 9562 / RFC 4122 UUID v4, v5 (SHA-1) & v7 (Time-Ordered Monotonic) engine

const std = @import("std");

const HEX_LOWER: []const u8 = "0123456789abcdef";
const HEX_UPPER: []const u8 = "0123456789ABCDEF";

pub const VARIANT_NCS: u8 = 0;
pub const VARIANT_RFC4122: u8 = 2;
pub const VARIANT_RFC9562: u8 = 2;
pub const VARIANT_MICROSOFT: u8 = 6;
pub const VARIANT_FUTURE: u8 = 7;

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
    zig {
        _ = std.os.linux.clock_gettime(.REALTIME, &ts);
    }
    const sec_i64: i64 = ts.sec as i64;
    const nsec_i64: i64 = ts.nsec as i64;
    if (sec_i64 < 0) return 0;
    const sec_u64 = u64(sec_i64);
    const ms_part = u64(@divTrunc(nsec_i64, 1000000));
    return sec_u64 * 1000 + ms_part;
}

fn hexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return (c - 'a') + 10;
    if (c >= 'A' && c <= 'F') return (c - 'A') + 10;
    return null;
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

    /// Parses standard hyphenated (36), compact (32), `urn:uuid:` prefixed, or `{...}` braced UUID strings.
    static fn parse(input: []const u8): Uuid? {
        let s = input;
        if (s.len >= 9 && asciiEqlIgnoreCase(s[0..9], "urn:uuid:")) {
            s = s[9..];
        } else if (s.len >= 2 && s[0] == '{' && s[s.len - 1] == '}') {
            s = s[1 .. s.len - 1];
        }

        let b: [16]u8 = undefined;
        if (s.len == 32) {
            for (i of 0..16) {
                const hi = hexNibble(s[i * 2]) ?? return null;
                const lo = hexNibble(s[i * 2 + 1]) ?? return null;
                b[i] = (hi << 4) | lo;
            }
            return Uuid { bytes: b };
        }

        if (s.len == 36) {
            if (s[8] != '-' || s[13] != '-' || s[18] != '-' || s[23] != '-') {
                return null;
            }
            let pos: usize = 0;
            for (i of 0..16) {
                if (pos == 8 || pos == 13 || pos == 18 || pos == 23) {
                    pos += 1;
                }
                const hi = hexNibble(s[pos]) ?? return null;
                const lo = hexNibble(s[pos + 1]) ?? return null;
                b[i] = (hi << 4) | lo;
                pos += 2;
            }
            return Uuid { bytes: b };
        }

        return null;
    }

    /// Returns true if all 16 bytes are zero.
    fn isNil(): bool {
        for (i of 0..16) {
            if (_.bytes[i] != 0) return false;
        }
        return true;
    }

    /// Returns true if all 16 bytes are 0xFF.
    fn isMax(): bool {
        for (i of 0..16) {
            if (_.bytes[i] != 0xFF) return false;
        }
        return true;
    }

    /// Returns the 4-bit version field (e.g. 4, 5, 7).
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

    /// Extracts the 48-bit Unix epoch millisecond timestamp if this UUID is version 7; otherwise returns null.
    fn extractV7TimestampMs(): u64? {
        if (_.version() != 7) return null;
        return ((_.bytes[0] as u64) << 40)
            | ((_.bytes[1] as u64) << 32)
            | ((_.bytes[2] as u64) << 24)
            | ((_.bytes[3] as u64) << 16)
            | ((_.bytes[4] as u64) << 8)
            | (_.bytes[5] as u64);
    }

    /// Returns true if both UUIDs have identical 16-byte representations.
    fn eql(other: Uuid): bool {
        return std.mem.eql(u8, &_.bytes, &other.bytes);
    }

    /// Lexicographical unsigned byte comparison: -1 if less, 0 if equal, 1 if greater.
    fn cmp(other: Uuid): i32 {
        for (i of 0..16) {
            if (_.bytes[i] < other.bytes[i]) return -1;
            if (_.bytes[i] > other.bytes[i]) return 1;
        }
        return 0;
    }

    fn formatHex(buf: []u8, table: []const u8): []const u8 {
        if (buf.len < 36) return "";
        let pos: usize = 0;
        for (i of 0..16) {
            if (i == 4 || i == 6 || i == 8 || i == 10) {
                buf[pos] = '-';
                pos += 1;
            }
            const byte = _.bytes[i];
            buf[pos] = table[(byte >> 4) & 0x0F];
            buf[pos + 1] = table[byte & 0x0F];
            pos += 2;
        }
        return buf[0..36];
    }

    /// Formats the UUID as lowercase 36-character canonical hyphenated string into `buf`.
    fn format(buf: []u8): []const u8 {
        return _.formatHex(buf, HEX_LOWER);
    }

    /// Formats the UUID as uppercase 36-character canonical hyphenated string into `buf`.
    fn formatUpper(buf: []u8): []const u8 {
        return _.formatHex(buf, HEX_UPPER);
    }

    /// Formats the UUID as lowercase 32-character compact hex string into `buf`.
    fn formatCompact(buf: []u8): []const u8 {
        if (buf.len < 32) return "";
        for (i of 0..16) {
            const byte = _.bytes[i];
            buf[i * 2] = HEX_LOWER[(byte >> 4) & 0x0F];
            buf[i * 2 + 1] = HEX_LOWER[byte & 0x0F];
        }
        return buf[0..32];
    }

    /// Formats the UUID as `urn:uuid:...` (45 characters) into `buf`.
    fn formatUrn(buf: []u8): []const u8 {
        if (buf.len < 45) return "";
        @memcpy(buf[0..9], "urn:uuid:");
        * = _.formatHex(buf[9..], HEX_LOWER);
        return buf[0..45];
    }

    /// Allocates and returns a canonical lowercase 36-character UUID string.
    fn toStringAlloc(alloc: std.mem.Allocator): ([]u8)! {
        let out = alloc.alloc(u8, 36)?;
        * = _.format(out);
        return out;
    }
}

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
