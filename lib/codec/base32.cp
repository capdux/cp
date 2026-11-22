// Source: cp/lib/base32/base32.cp (Pure CP RFC 4648 Base32, Base32hex & Crockford Base32)
const std = @import("std");

pub const Base32Error = error{
    InvalidCharacter,
    InvalidPadding,
    InvalidLength,
    BufferTooSmall,
};

pub const Variant = enum {
    Standard,
    Hex,
    Crockford,
};

const STD_ALPHA = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
const HEX_ALPHA = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
const CROCK_ALPHA = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

fn alphabetFor(v: Variant): []const u8 {
    switch (v) {
        .Standard => return STD_ALPHA,
        .Hex => return HEX_ALPHA,
        .Crockford => return CROCK_ALPHA,
    }
}

fn decodeChar(c: u8, v: Variant): Base32Error!u8 {
    switch (v) {
        .Standard => {
            if (c >= 'A' and c <= 'Z') { return c - 'A'; }
            if (c >= 'a' and c <= 'z') { return c - 'a'; }
            if (c >= '2' and c <= '7') { return 26 + (c - '2'); }
            return Base32Error.InvalidCharacter;
        },
        .Hex => {
            if (c >= '0' and c <= '9') { return c - '0'; }
            if (c >= 'A' and c <= 'V') { return 10 + (c - 'A'); }
            if (c >= 'a' and c <= 'v') { return 10 + (c - 'a'); }
            return Base32Error.InvalidCharacter;
        },
        .Crockford => {
            const up = std.ascii.toUpper(c);
            if (up >= '0' and up <= '9') { return up - '0'; }
            if (up == 'O') { return 0; }
            if (up == 'I' or up == 'L') { return 1; }
            if (up >= 'A' and up <= 'H') { return 10 + (up - 'A'); }
            if (up == 'J' or up == 'K') { return 18 + (up - 'J'); }
            if (up == 'M' or up == 'N') { return 20 + (up - 'M'); }
            if (up >= 'P' and up <= 'T') { return 22 + (up - 'P'); }
            if (up >= 'V' and up <= 'Z') { return 27 + (up - 'V'); }
            return Base32Error.InvalidCharacter;
        },
    }
}

pub fn encodedLen(input_len: usize, padded: bool): usize {
    if (input_len == 0) { return 0; }
    if (padded) {
        return ((input_len + 4) / 5) * 8;
    }
    const full_groups = input_len / 5;
    const rem = input_len % 5;
    let extra: usize = 0;
    if (rem == 1) { extra = 2; }
    else if (rem == 2) { extra = 4; }
    else if (rem == 3) { extra = 5; }
    else if (rem == 4) { extra = 7; }
    return full_groups * 8 + extra;
}

pub fn encodeAlloc(alloc: std.mem.Allocator, data: []const u8, variant: Variant, padded: bool): ![]u8 {
    const out_len = encodedLen(data.len, padded);
    let out = try alloc.alloc(u8, out_len);
    errdefer alloc.free(out);

    const alpha = alphabetFor(variant);
    let i: usize = 0;
    let out_pos: usize = 0;
    let buf: u16 = 0;
    let bits: u5 = 0;

    while (i < data.len) : (i += 1) {
        buf = (buf << 8) | @as(u16, data[i]);
        bits += 8;
        while (bits >= 5) {
            bits -= 5;
            const shift: u4 = @intCast(bits);
            const idx: usize = @intCast((buf >> shift) & 0x1F);
            out[out_pos] = alpha[idx];
            out_pos += 1;
        }
    }
    if (bits > 0) {
        const shift: u4 = @intCast(5 - bits);
        const idx: usize = @intCast((buf << shift) & 0x1F);
        out[out_pos] = alpha[idx];
        out_pos += 1;
    }
    if (padded) {
        while (out_pos < out_len) : (out_pos += 1) {
            out[out_pos] = '=';
        }
    }
    return out;
}

pub fn decodeAlloc(alloc: std.mem.Allocator, encoded: []const u8, variant: Variant): ![]u8 {
    let list = std.ArrayList(u8).empty;
    errdefer list.deinit(alloc);

    let buf: u16 = 0;
    let bits: u5 = 0;
    let saw_pad = false;

    for (c of encoded) {
        if (c == '=') {
            saw_pad = true;
            continue;
        }
        if (variant == .Crockford and c == '-') {
            continue;
        }
        if (saw_pad) {
            return Base32Error.InvalidPadding;
        }
        const val = try decodeChar(c, variant);
        buf = (buf << 5) | @as(u16, val);
        bits += 5;
        if (bits >= 8) {
            bits -= 8;
            const shift: u4 = @intCast(bits);
            const byte_val: u8 = @intCast((buf >> shift) & 0xFF);
            try list.append(alloc, byte_val);
        }
    }
    return list.toOwnedSlice(alloc);
}
