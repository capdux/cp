// Source: cp/lib/uuid_v7/uuid_v7.cp (Pure CP RFC 9562 UUID v4, v5 & Monotonic UUID v7)
const std = @import("std");
const Sha1 = std.crypto.hash.Sha1;

pub const UuidError = error{
    InvalidLength,
    InvalidFormat,
    InvalidHex,
};

pub const Uuid = struct {
    bytes: [16]u8,

    pub fn nil(): Uuid {
        let b: [16]u8 = undefined;
        @memset(&b, 0);
        return Uuid{ .bytes = b };
    }

    pub fn max(): Uuid {
        let b: [16]u8 = undefined;
        @memset(&b, 0xFF);
        return Uuid{ .bytes = b };
    }

    pub fn fromBytes(raw: [16]u8): Uuid {
        return Uuid{ .bytes = raw };
    }

    pub fn version(self: Uuid): u4 {
        return @intCast((self.bytes[6] >> 4) & 0x0F);
    }

    pub fn variantRfc9562(self: Uuid): bool {
        return (self.bytes[8] & 0xC0) == 0x80;
    }

    pub fn equals(self: Uuid, other: Uuid): bool {
        return std.mem.eql(u8, &self.bytes, &other.bytes);
    }

    pub fn compare(self: Uuid, other: Uuid): i8 {
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            if (self.bytes[i] < other.bytes[i]) { return -1; }
            if (self.bytes[i] > other.bytes[i]) { return 1; }
        }
        return 0;
    }

    pub fn v4(): Uuid {
        let b: [16]u8 = undefined;
        let threaded = std.Io.Threaded.init(std.heap.page_allocator, .{});
        defer threaded.deinit();
        threaded.io().random(&b);
        b[6] = (b[6] & 0x0F) | 0x40;
        b[8] = (b[8] & 0x3F) | 0x80;
        return Uuid{ .bytes = b };
    }

    pub fn v7At(unix_ms: u64): Uuid {
        let b: [16]u8 = undefined;
        let threaded = std.Io.Threaded.init(std.heap.page_allocator, .{});
        defer threaded.deinit();
        threaded.io().random(b[6..16]);
        // 48-bit big-endian unix_ms
        b[0] = @intCast((unix_ms >> 40) & 0xFF);
        b[1] = @intCast((unix_ms >> 32) & 0xFF);
        b[2] = @intCast((unix_ms >> 24) & 0xFF);
        b[3] = @intCast((unix_ms >> 16) & 0xFF);
        b[4] = @intCast((unix_ms >> 8) & 0xFF);
        b[5] = @intCast(unix_ms & 0xFF);
        b[6] = (b[6] & 0x0F) | 0x70;
        b[8] = (b[8] & 0x3F) | 0x80;
        return Uuid{ .bytes = b };
    }

    pub fn v7(): Uuid {
        let threaded = std.Io.Threaded.init(std.heap.page_allocator, .{});
        defer threaded.deinit();
        const ms_raw = std.Io.Clock.real.now(threaded.io()).toMilliseconds();
        const ms: u64 = if (ms_raw < 0) 0 else @intCast(ms_raw);
        return Uuid.v7At(ms);
    }

    pub fn timestampMsV7(self: Uuid): u64 {
        let ts: u64 = 0;
        ts |= @as(u64, self.bytes[0]) << 40;
        ts |= @as(u64, self.bytes[1]) << 32;
        ts |= @as(u64, self.bytes[2]) << 24;
        ts |= @as(u64, self.bytes[3]) << 16;
        ts |= @as(u64, self.bytes[4]) << 8;
        ts |= @as(u64, self.bytes[5]);
        return ts;
    }

    pub fn v5(namespace: Uuid, name: []const u8): Uuid {
        let h = Sha1.init(.{});
        h.update(&namespace.bytes);
        h.update(name);
        let digest: [20]u8 = undefined;
        h.final(&digest);
        let b: [16]u8 = undefined;
        @memcpy(&b, digest[0..16]);
        b[6] = (b[6] & 0x0F) | 0x50;
        b[8] = (b[8] & 0x3F) | 0x80;
        return Uuid{ .bytes = b };
    }

    pub fn format(self: Uuid): [36]u8 {
        const hex_chars = "0123456789abcdef";
        let out: [36]u8 = undefined;
        let out_pos: usize = 0;
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            if (i == 4 or i == 6 or i == 8 or i == 10) {
                out[out_pos] = '-';
                out_pos += 1;
            }
            const hi: usize = @intCast((self.bytes[i] >> 4) & 0x0F);
            const lo: usize = @intCast(self.bytes[i] & 0x0F);
            out[out_pos] = hex_chars[hi];
            out[out_pos + 1] = hex_chars[lo];
            out_pos += 2;
        }
        return out;
    }

    pub fn parse(s: []const u8): UuidError!Uuid {
        if (s.len != 36) { return UuidError.InvalidLength; }
        if (s[8] != '-' or s[13] != '-' or s[18] != '-' or s[23] != '-') {
            return UuidError.InvalidFormat;
        }
        let b: [16]u8 = undefined;
        let bi: usize = 0;
        let si: usize = 0;
        while (si < 36) {
            if (si == 8 or si == 13 or si == 18 or si == 23) {
                si += 1;
                continue;
            }
            const hi = parseHexNibble(s[si]) catch return UuidError.InvalidHex;
            const lo = parseHexNibble(s[si + 1]) catch return UuidError.InvalidHex;
            b[bi] = (hi << 4) | lo;
            bi += 1;
            si += 2;
        }
        return Uuid{ .bytes = b };
    }
};

fn parseHexNibble(c: u8): UuidError!u8 {
    if (c >= '0' and c <= '9') { return c - '0'; }
    if (c >= 'a' and c <= 'f') { return 10 + (c - 'a'); }
    if (c >= 'A' and c <= 'F') { return 10 + (c - 'A'); }
    return UuidError.InvalidHex;
}

pub const NAMESPACE_DNS = Uuid.parse("6ba7b810-9dad-11d1-80b4-00c04fd430c8") catch unreachable;
pub const NAMESPACE_URL = Uuid.parse("6ba7b811-9dad-11d1-80b4-00c04fd430c8") catch unreachable;
