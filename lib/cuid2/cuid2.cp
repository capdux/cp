// Source: cp/lib/cuid2/cuid2.cp (Pure CP CUID2 Collision-Resistant Base36 ID Engine)
const std = @import("std");

pub const CUID2_DEFAULT_LEN: usize = 24;
pub const CUID2_MIN_LEN: usize = 4;
pub const CUID2_MAX_LEN: usize = 32;

pub const Cuid2Error = error{
    InvalidLength,
    InvalidCharacter,
};

const BASE36_CHARS = "0123456789abcdefghijklmnopqrstuvwxyz";
const ALPHA_CHARS = "abcdefghijklmnopqrstuvwxyz";

pub fn encodeBase36U128(val: u128, out: []u8): usize {
    if (val == 0) {
        if (out.len > 0) {
            out[0] = '0';
            return 1;
        }
        return 0;
    }
    let tmp: [32]u8 = undefined;
    let idx: usize = 0;
    let cur = val;
    while (cur > 0) {
        const rem: usize = @intCast(cur % 36);
        tmp[idx] = BASE36_CHARS[rem];
        idx += 1;
        cur /= 36;
    }
    let j: usize = 0;
    while (j < idx and j < out.len) : (j += 1) {
        out[j] = tmp[idx - 1 - j];
    }
    return j;
}

pub fn encodeBytesBase36(bytes: []const u8, out: []u8): usize {
    // Convert big-endian byte slice to base36 in chunks
    let acc: u128 = 0;
    let pos: usize = 0;
    for (b of bytes) {
        acc = (acc << 8) | @as(u128, b);
        if (acc >= 36 * 36 * 36 * 36 * 36 * 36) {
            let chunk: [16]u8 = undefined;
            const n = encodeBase36U128(acc, &chunk);
            let k: usize = 0;
            while (k < n and pos < out.len) : (k += 1) {
                out[pos] = chunk[k];
                pos += 1;
            }
            acc = 0;
        }
    }
    if (acc > 0 and pos < out.len) {
        let chunk: [16]u8 = undefined;
        const n = encodeBase36U128(acc, &chunk);
        let k: usize = 0;
        while (k < n and pos < out.len) : (k += 1) {
            out[pos] = chunk[k];
            pos += 1;
        }
    }
    return pos;
}

pub fn isValidCuid2(s: []const u8): bool {
    if (s.len < CUID2_MIN_LEN or s.len > CUID2_MAX_LEN) { return false; }
    if (s[0] < 'a' or s[0] > 'z') { return false; }
    for (c of s) {
        const is_digit = c >= '0' and c <= '9';
        const is_lower = c >= 'a' and c <= 'z';
        if (!is_digit and !is_lower) { return false; }
    }
    return true;
}

pub const Cuid2Generator = struct {
    counter: std.atomic.Value(u64),
    fingerprint: [16]u8,

    pub fn init(seed_entropy: []const u8): Cuid2Generator {
        let fp: [16]u8 = undefined;
        let hasher = std.crypto.hash.sha3.Sha3_512.init(.{});
        hasher.update(seed_entropy);
        let threaded = std.Io.Threaded.init(std.heap.page_allocator, .{});
        defer threaded.deinit();
        let rand_buf: [32]u8 = undefined;
        threaded.io().random(&rand_buf);
        hasher.update(&rand_buf);
        let digest: [64]u8 = undefined;
        hasher.final(&digest);
        @memcpy(&fp, digest[0..16]);

        const init_cnt = std.mem.readInt(u32, digest[16..20], .little);
        return Cuid2Generator{
            .counter = std.atomic.Value(u64).init(@as(u64, init_cnt)),
            .fingerprint = fp,
        };
    }

    pub fn nextInto(self: *Cuid2Generator, out: []u8): Cuid2Error![]const u8 {
        if (out.len < CUID2_MIN_LEN or out.len > CUID2_MAX_LEN) {
            return Cuid2Error.InvalidLength;
        }
        let threaded = std.Io.Threaded.init(std.heap.page_allocator, .{});
        defer threaded.deinit();
        const ts_ms: i64 = std.Io.Clock.real.now(threaded.io()).toMilliseconds();
        const cnt = self.counter.fetchAdd(1, .monotonic);

        let salt: [32]u8 = undefined;
        threaded.io().random(&salt);

        // First letter must be [a-z]
        const first_idx: usize = @intCast(salt[0] % 26);
        out[0] = ALPHA_CHARS[first_idx];

        let hasher = std.crypto.hash.sha3.Sha3_512.init(.{});
        let ts_bytes: [8]u8 = undefined;
        std.mem.writeInt(i64, &ts_bytes, ts_ms, .big);
        let cnt_bytes: [8]u8 = undefined;
        std.mem.writeInt(u64, &cnt_bytes, cnt, .big);
        hasher.update(&ts_bytes);
        hasher.update(&salt);
        hasher.update(&cnt_bytes);
        hasher.update(&self.fingerprint);

        let digest: [64]u8 = undefined;
        hasher.final(&digest);

        let b36: [128]u8 = undefined;
        const written = encodeBytesBase36(&digest, &b36);
        let i: usize = 1;
        while (i < out.len) : (i += 1) {
            if (i - 1 < written) {
                out[i] = b36[i - 1];
            } else {
                const fallback_idx: usize = @intCast(digest[i % 64] % 36);
                out[i] = BASE36_CHARS[fallback_idx];
            }
        }
        return out[0..out.len];
    }

    pub fn next24(self: *Cuid2Generator): [24]u8 {
        let buf: [24]u8 = undefined;
        * = self.nextInto(&buf) catch unreachable;
        return buf;
    }
};
