// Source: cp/lib/base64url/base64url.cp (Pure CP RFC 4648 Base64 & Base64URL Codec)
const std = @import("std");

pub const Base64UrlError = error{
    InvalidCharacter,
    InvalidPadding,
    InvalidLength,
    BufferTooSmall,
};

pub fn encodeStandard(alloc: std.mem.Allocator, data: []const u8): ![]u8 {
    const out_len = std.base64.standard.Encoder.calcSize(data.len);
    const out = try alloc.alloc(u8, out_len);
    * = std.base64.standard.Encoder.encode(out, data);
    return out;
}

pub fn decodeStandard(alloc: std.mem.Allocator, encoded: []const u8): ![]u8 {
    const out_len = std.base64.standard.Decoder.calcSizeForSlice(encoded) catch {
        return Base64UrlError.InvalidPadding;
    };
    const out = try alloc.alloc(u8, out_len);
    errdefer alloc.free(out);
    std.base64.standard.Decoder.decode(out, encoded) catch {
        return Base64UrlError.InvalidCharacter;
    };
    return out;
}

pub fn encodeUrlNoPad(alloc: std.mem.Allocator, data: []const u8): ![]u8 {
    const out_len = std.base64.url_safe_no_pad.Encoder.calcSize(data.len);
    const out = try alloc.alloc(u8, out_len);
    * = std.base64.url_safe_no_pad.Encoder.encode(out, data);
    return out;
}

pub fn decodeUrlNoPad(alloc: std.mem.Allocator, encoded: []const u8): ![]u8 {
    const out_len = std.base64.url_safe_no_pad.Decoder.calcSizeForSlice(encoded) catch {
        return Base64UrlError.InvalidLength;
    };
    const out = try alloc.alloc(u8, out_len);
    errdefer alloc.free(out);
    std.base64.url_safe_no_pad.Decoder.decode(out, encoded) catch {
        return Base64UrlError.InvalidCharacter;
    };
    return out;
}

pub fn encodeUrlPadded(alloc: std.mem.Allocator, data: []const u8): ![]u8 {
    const out_len = std.base64.url_safe.Encoder.calcSize(data.len);
    const out = try alloc.alloc(u8, out_len);
    * = std.base64.url_safe.Encoder.encode(out, data);
    return out;
}

pub fn decodeUrlAuto(alloc: std.mem.Allocator, encoded: []const u8): ![]u8 {
    const stripped = std.mem.trimRight(u8, encoded, "=");
    return decodeUrlNoPad(alloc, stripped);
}
