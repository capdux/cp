// Source: cp/lib/base58_check/base58_check.cp (Pure CP Bitcoin Base58 & Base58Check Double-SHA256 Engine)
const std = @import("std");
const Sha256 = std.crypto.hash.sha2.Sha256;

pub const Base58Error = error{
    InvalidCharacter,
    InvalidChecksum,
    PayloadTooShort,
};

const B58_ALPHA = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

fn decodeChar(c: u8): Base58Error!u8 {
    let i: usize = 0;
    while (i < 58) : (i += 1) {
        if (B58_ALPHA[i] == c) { return @intCast(i); }
    }
    return Base58Error.InvalidCharacter;
}

pub fn encode(alloc: std.mem.Allocator, data: []const u8): ![]u8 {
    let zeros: usize = 0;
    while (zeros < data.len and data[zeros] == 0) : (zeros += 1) {}

    const cap_len = (data.len - zeros) * 138 / 100 + 2;
    let digits = try alloc.alloc(u8, cap_len);
    defer alloc.free(digits);
    @memset(digits, 0);

    let d_len: usize = 0;
    let i = zeros;
    while (i < data.len) : (i += 1) {
        let carry: u32 = @intCast(data[i]);
        let j: usize = 0;
        while (j < d_len or carry != 0) : (j += 1) {
            const cur: u32 = @as(u32, digits[j]) * 256 + carry;
            digits[j] = @intCast(cur % 58);
            carry = cur / 58;
            if (j + 1 > d_len) { d_len = j + 1; }
        }
    }

    const total = zeros + d_len;
    let out = try alloc.alloc(u8, total);
    let pos: usize = 0;
    while (pos < zeros) : (pos += 1) {
        out[pos] = '1';
    }
    let k: usize = 0;
    while (k < d_len) : (k += 1) {
        const idx: usize = @intCast(digits[d_len - 1 - k]);
        out[zeros + k] = B58_ALPHA[idx];
    }
    return out;
}

pub fn decode(alloc: std.mem.Allocator, encoded: []const u8): ![]u8 {
    let zeros: usize = 0;
    while (zeros < encoded.len and encoded[zeros] == '1') : (zeros += 1) {}

    const cap_len = (encoded.len - zeros) * 733 / 1000 + 2;
    let bytes = try alloc.alloc(u8, cap_len);
    defer alloc.free(bytes);
    @memset(bytes, 0);

    let b_len: usize = 0;
    let i = zeros;
    while (i < encoded.len) : (i += 1) {
        let carry: u32 = @intCast(try decodeChar(encoded[i]));
        let j: usize = 0;
        while (j < b_len or carry != 0) : (j += 1) {
            const cur: u32 = @as(u32, bytes[j]) * 58 + carry;
            bytes[j] = @intCast(cur & 0xFF);
            carry = cur >> 8;
            if (j + 1 > b_len) { b_len = j + 1; }
        }
    }

    const total = zeros + b_len;
    let out = try alloc.alloc(u8, total);
    let pos: usize = 0;
    while (pos < zeros) : (pos += 1) {
        out[pos] = 0;
    }
    let k: usize = 0;
    while (k < b_len) : (k += 1) {
        out[zeros + k] = bytes[b_len - 1 - k];
    }
    return out;
}

fn doubleSha256(data: []const u8): [32]u8 {
    let first: [32]u8 = undefined;
    Sha256.hash(data, &first, .{});
    let second: [32]u8 = undefined;
    Sha256.hash(&first, &second, .{});
    return second;
}

pub fn encodeCheck(alloc: std.mem.Allocator, version: u8, payload: []const u8): ![]u8 {
    let buf = try alloc.alloc(u8, 1 + payload.len + 4);
    defer alloc.free(buf);
    buf[0] = version;
    @memcpy(buf[1 .. 1 + payload.len], payload);
    const chk = doubleSha256(buf[0 .. 1 + payload.len]);
    @memcpy(buf[1 + payload.len .. 1 + payload.len + 4], chk[0..4]);
    return encode(alloc, buf);
}

pub const CheckDecoded = struct {
    version: u8,
    payload: []u8,

    pub fn deinit(self: CheckDecoded, alloc: std.mem.Allocator) {
        alloc.free(self.payload);
    }
};

pub fn decodeCheck(alloc: std.mem.Allocator, encoded: []const u8): !CheckDecoded {
    const raw = try decode(alloc, encoded);
    defer alloc.free(raw);
    if (raw.len < 5) { return Base58Error.PayloadTooShort; }

    const body_len = raw.len - 4;
    const expected_chk = doubleSha256(raw[0..body_len]);
    if (!std.mem.eql(u8, raw[body_len..], expected_chk[0..4])) {
        return Base58Error.InvalidChecksum;
    }

    const payload = try alloc.dupe(u8, raw[1..body_len]);
    return CheckDecoded{
        .version = raw[0],
        .payload = payload,
    };
}
