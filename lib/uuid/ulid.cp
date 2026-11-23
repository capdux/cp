// Source: cp/lib/uuid/ulid.cp (ULID 128-bit Crockford Base32 Sortable ID)
// lib/uuid/ulid.cp
// Pure CP ULID (Universally Unique Lexicographically Sortable Identifier) with Crockford Base32

const std = @import("std");
import { Uuid } from "./uuid.cp";

const CROCKFORD_UPPER: []const u8 = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
const CROCKFORD_LOWER: []const u8 = "0123456789abcdefghjkmnpqrstvwxyz";

fn ulidFillCsprng(buf: []u8) {
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

fn ulidCurrentUnixMs(): u64 {
    let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
    * = std.os.linux.clock_gettime(.REALTIME, &ts);
    const sec_i64: i64 = ts.sec as i64;
    const nsec_i64: i64 = ts.nsec as i64;
    if (sec_i64 < 0) return 0;
    const sec_u64 = u64(sec_i64);
    const ms_part = u64(@divTrunc(nsec_i64, 1000000));
    return sec_u64 * 1000 + ms_part;
}

fn crockfordDecodeChar(c: u8): u8? {
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
        else => null,
    };
}

let ulid_state_lock: std.atomic.Value(u32) = std.atomic.Value(u32).init(0);
let ulid_last_ms: u64 = 0;
let ulid_last_rand: [10]u8 = [0; 10];
let ulid_initialized: bool = false;

fn increment80BitBe(rand_bytes: *[10]u8): bool {
    let idx: usize = 10;
    while (idx > 0) {
        idx -= 1;
        if (rand_bytes[idx] != 0xFF) {
            rand_bytes[idx] += 1;
            return true;
        }
        rand_bytes[idx] = 0;
    }
    return false;
}

pub struct Ulid {
    bytes: [16]u8 = [0; 16];

    /// Returns the Nil ULID (all zero bytes).
    static fn nil(): Ulid {
        return Ulid { bytes: [0; 16] };
    }

    /// Returns the Max ULID (all 0xFF bytes, `7ZZZZZZZZZZZZZZZZZZZZZZZZZ`).
    static fn max(): Ulid {
        return Ulid { bytes: [0xFF; 16] };
    }

    /// Constructs a `Ulid` from 16 raw big-endian bytes.
    static fn fromBytes(raw: [16]u8): Ulid {
        return Ulid { bytes: raw };
    }

    /// Generates a ULID at an explicit Unix epoch millisecond timestamp with 80-bit CSPRNG entropy.
    static fn generateAt(unix_ms: u64): Ulid {
        let b: [16]u8 = undefined;
        ulidFillCsprng(b[6..16]);

        const ts = unix_ms & 0x0000_FFFF_FFFF_FFFF;
        b[0] = ((ts >> 40) & 0xFF) as u8;
        b[1] = ((ts >> 32) & 0xFF) as u8;
        b[2] = ((ts >> 24) & 0xFF) as u8;
        b[3] = ((ts >> 16) & 0xFF) as u8;
        b[4] = ((ts >> 8) & 0xFF) as u8;
        b[5] = (ts & 0xFF) as u8;
        return Ulid { bytes: b };
    }

    /// Generates a strictly monotonic, thread-safe ULID using current wall-clock time.
    static fn generate(): Ulid {
        let fresh_rand: [10]u8 = undefined;
        ulidFillCsprng(&fresh_rand);

        while (ulid_state_lock.cmpxchgWeak(0, 1, .acquire, .monotonic) != null) {
            std.atomic.spinLoopHint();
        }
        defer ulid_state_lock.store(0, .release);

        let ts: u64 = ulidCurrentUnixMs() & 0x0000_FFFF_FFFF_FFFF;
        if (!ulid_initialized || ts > ulid_last_ms) {
            ulid_last_ms = ts;
            @memcpy(ulid_last_rand[0..10], fresh_rand[0..10]);
            ulid_initialized = true;
        } else {
            if (!increment80BitBe(&ulid_last_rand)) {
                ulid_last_ms = (ulid_last_ms + 1) & 0x0000_FFFF_FFFF_FFFF;
                @memcpy(ulid_last_rand[0..10], fresh_rand[0..10]);
            }
            ts = ulid_last_ms;
        }

        let b: [16]u8 = undefined;
        b[0] = ((ts >> 40) & 0xFF) as u8;
        b[1] = ((ts >> 32) & 0xFF) as u8;
        b[2] = ((ts >> 24) & 0xFF) as u8;
        b[3] = ((ts >> 16) & 0xFF) as u8;
        b[4] = ((ts >> 8) & 0xFF) as u8;
        b[5] = (ts & 0xFF) as u8;
        @memcpy(b[6..16], ulid_last_rand[0..10]);
        return Ulid { bytes: b };
    }

    /// Parses a 26-character Crockford Base32 ULID (case-insensitive; maps I/L->1, O->0; rejects overflow).
    static fn parse(str: []const u8): Ulid? {
        if (str.len != 26) return null;
        const first = crockfordDecodeChar(str[0]) ?? return null;
        if (first > 7) return null; // 128-bit overflow guard (max prefix digit is '7')

        let acc: u128 = first as u128;
        for (i of 1..26) {
            const v = crockfordDecodeChar(str[i]) ?? return null;
            acc = (acc << 5) | (v as u128);
        }

        let b: [16]u8 = undefined;
        std.mem.writeInt(u128, &b, acc, .big);
        return Ulid { bytes: b };
    }

    /// Alias for `Ulid.parse`.
    static fn decode(str: []const u8): Ulid? {
        return Ulid.parse(str);
    }

    fn formatWithAlphabet(buf: []u8, alphabet: []const u8): []const u8 {
        if (buf.len < 26) return "";
        const v: u128 = std.mem.readInt(u128, &_.bytes, .big);
        for (i of 0..26) {
            const shift: u7 = @intCast((25 - i) * 5);
            const idx: usize = @intCast((v >> shift) & 0x1F);
            buf[i] = alphabet[idx];
        }
        return buf[0..26];
    }

    /// Encodes the ULID as a 26-character uppercase Crockford Base32 string into `buf`.
    fn format(buf: []u8): []const u8 {
        return _.formatWithAlphabet(buf, CROCKFORD_UPPER);
    }

    /// Alias for `format`.
    fn encode(buf: []u8): []const u8 {
        return _.formatWithAlphabet(buf, CROCKFORD_UPPER);
    }

    /// Encodes the ULID as a 26-character lowercase Crockford Base32 string into `buf`.
    fn formatLower(buf: []u8): []const u8 {
        return _.formatWithAlphabet(buf, CROCKFORD_LOWER);
    }

    /// Allocates and returns a 26-character uppercase Crockford Base32 string.
    fn toStringAlloc(alloc: std.mem.Allocator): ([]u8)! {
        let out = alloc.alloc(u8, 26)?;
        * = _.format(out);
        return out;
    }

    /// Extracts the 48-bit Unix epoch millisecond timestamp.
    fn timestampMs(): u64 {
        return ((_.bytes[0] as u64) << 40)
            | ((_.bytes[1] as u64) << 32)
            | ((_.bytes[2] as u64) << 24)
            | ((_.bytes[3] as u64) << 16)
            | ((_.bytes[4] as u64) << 8)
            | (_.bytes[5] as u64);
    }

    /// Returns true if all 16 bytes are zero.
    fn isNil(): bool {
        for (i of 0..16) {
            if (_.bytes[i] != 0) return false;
        }
        return true;
    }

    /// Returns true if both ULIDs have identical 16-byte representations.
    fn eql(other: Ulid): bool {
        return std.mem.eql(u8, &_.bytes, &other.bytes);
    }

    /// Lexicographical unsigned byte comparison: -1 if less, 0 if equal, 1 if greater.
    fn cmp(other: Ulid): i32 {
        for (i of 0..16) {
            if (_.bytes[i] < other.bytes[i]) return -1;
            if (_.bytes[i] > other.bytes[i]) return 1;
        }
        return 0;
    }

    /// Losslessly converts this 128-bit ULID to a `Uuid` preserving all 16 bytes.
    fn toUuid(): Uuid {
        return Uuid { bytes: _.bytes };
    }

    /// Losslessly constructs a `Ulid` from a 128-bit `Uuid` preserving all 16 bytes.
    static fn fromUuid(u: Uuid): Ulid {
        return Ulid { bytes: u.bytes };
    }
}
