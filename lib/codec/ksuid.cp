// Source: cp/lib/ksuid/ksuid.cp (Pure CP Segment KSUID 20-byte / 27-char Base62)
const std = @import("std");

pub const KSUID_EPOCH: i64 = 1400000000;
pub const KSUID_BYTE_LEN: usize = 20;
pub const KSUID_ENCODED_LEN: usize = 27;
pub const KSUID_PAYLOAD_LEN: usize = 16;

pub const KsuidError = error{
    InvalidLength,
    InvalidCharacter,
    Overflow,
    TimestampOutOfRange,
};

const BASE62_CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

fn charToBase62(c: u8): KsuidError!u8 {
    if (c >= '0' and c <= '9') { return c - '0'; }
    if (c >= 'A' and c <= 'Z') { return @as(u8, 10) + (c - 'A'); }
    if (c >= 'a' and c <= 'z') { return @as(u8, 36) + (c - 'a'); }
    return KsuidError.InvalidCharacter;
}

pub const Ksuid = struct {
    bytes: [20]u8,

    pub fn nil(): Ksuid {
        let z: [20]u8 = undefined;
        @memset(&z, 0);
        return Ksuid{ .bytes = z };
    }

    pub fn fromBytes(raw: [20]u8): Ksuid {
        return Ksuid{ .bytes = raw };
    }

    pub fn fromParts(unix_sec: i64, rand_payload: [16]u8): KsuidError!Ksuid {
        if (unix_sec < KSUID_EPOCH) { return KsuidError.TimestampOutOfRange; }
        const diff = unix_sec - KSUID_EPOCH;
        if (diff > 0xFFFF_FFFF) { return KsuidError.TimestampOutOfRange; }
        let b: [20]u8 = undefined;
        std.mem.writeInt(u32, b[0..4], @intCast(diff), .big);
        @memcpy(b[4..20], &rand_payload);
        return Ksuid{ .bytes = b };
    }

    pub fn generateAt(unix_sec: i64): KsuidError!Ksuid {
        let rand_payload: [16]u8 = undefined;
        let threaded = std.Io.Threaded.init(std.heap.page_allocator, .{});
        defer threaded.deinit();
        threaded.io().random(&rand_payload);
        return Ksuid.fromParts(unix_sec, rand_payload);
    }

    pub fn generate(): KsuidError!Ksuid {
        let threaded = std.Io.Threaded.init(std.heap.page_allocator, .{});
        defer threaded.deinit();
        const ts_ns = std.Io.Clock.real.now(threaded.io()).toSeconds();
        return Ksuid.generateAt(ts_ns);
    }

    pub fn timestampUnix(): i64 {
        const rel: i64 = @intCast(std.mem.readInt(u32, _.bytes[0..4], .big));
        return KSUID_EPOCH + rel;
    }

    pub fn payload(): [16]u8 {
        let p: [16]u8 = undefined;
        @memcpy(&p, _.bytes[4..20]);
        return p;
    }

    pub fn equals(other: Ksuid): bool {
        return std.mem.eql(u8, &_.bytes, &other.bytes);
    }

    pub fn compare(other: Ksuid): i8 {
        let i: usize = 0;
        while (i < 20) : (i += 1) {
            if (_.bytes[i] < other.bytes[i]) { return -1; }
            if (_.bytes[i] > other.bytes[i]) { return 1; }
        }
        return 0;
    }

    pub fn encode(): [27]u8 {
        // Convert 20-byte big-endian (5 x u32 words) to base62 via repeated division
        let words: [5]u32 = [
            std.mem.readInt(u32, _.bytes[0..4], .big),
            std.mem.readInt(u32, _.bytes[4..8], .big),
            std.mem.readInt(u32, _.bytes[8..12], .big),
            std.mem.readInt(u32, _.bytes[12..16], .big),
            std.mem.readInt(u32, _.bytes[16..20], .big),
        ];

        let out: [27]u8 = undefined;
        @memset(&out, '0');
        let out_idx: usize = 27;

        while (out_idx > 0) {
            let rem: u64 = 0;
            let all_zero = true;
            let w: usize = 0;
            while (w < 5) : (w += 1) {
                const cur: u64 = (rem << 32) | @as(u64, words[w]);
                words[w] = @intCast(cur / 62);
                rem = cur % 62;
                if (words[w] != 0) { all_zero = false; }
            }
            out_idx -= 1;
            const r_idx: usize = @intCast(rem);
            out[out_idx] = BASE62_CHARS[r_idx];
            if (all_zero) { break; }
        }
        return out;
    }

    pub fn decode(s: []const u8): KsuidError!Ksuid {
        if (s.len != KSUID_ENCODED_LEN) { return KsuidError.InvalidLength; }
        let words: [5]u32 = [ 0, 0, 0, 0, 0 ];

        for (c of s) {
            const digit = try charToBase62(c);
            let carry: u64 = digit;
            let w: usize = 5;
            while (w > 0) {
                w -= 1;
                const prod: u64 = @as(u64, words[w]) * 62 + carry;
                words[w] = @truncate(prod & 0xFFFF_FFFF);
                carry = prod >> 32;
            }
            if (carry != 0) { return KsuidError.Overflow; }
        }

        let b: [20]u8 = undefined;
        std.mem.writeInt(u32, b[0..4], words[0], .big);
        std.mem.writeInt(u32, b[4..8], words[1], .big);
        std.mem.writeInt(u32, b[8..12], words[2], .big);
        std.mem.writeInt(u32, b[12..16], words[3], .big);
        std.mem.writeInt(u32, b[16..20], words[4], .big);
        return Ksuid{ .bytes = b };
    }
};
