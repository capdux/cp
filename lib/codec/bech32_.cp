// Reference Only: Functional overlap with lib/codec/bech32_crockford.cp | Kept for comparison/analysis
// Source: cp/lib/bech32/bech32.cp (Pure CP BIP-173 Bech32 & BIP-350 Bech32m)
const std = @import("std");

pub const BECH32_CONST: u32 = 1;
pub const BECH32M_CONST: u32 = 0x2bc830a3;

pub const Bech32Variant = enum {
    bech32,
    bech32m,
};

pub const Bech32Error = error{
    InvalidHrp,
    InvalidLength,
    MixedCase,
    MissingSeparator,
    InvalidCharacter,
    InvalidChecksum,
    InvalidPadding,
    InvalidWitnessVersion,
    InvalidWitnessProgram,
    OutOfMemory,
};

const CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

const GENERATOR: [5]u32 = [ 0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3 ];

fn charsetIndex(c: u8): i8 {
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        if (CHARSET[i] == c) { return @intCast(i); }
    }
    return -1;
}

fn polymodStep(chk_in: u32, val: u8): u32 {
    const top = chk_in >> 25;
    let chk = ((chk_in & 0x1ffffff) << 5) ^ @as(u32, val);
    let i: usize = 0;
    while (i < 5) : (i += 1) {
        if (((top >> @as(u5, @intCast(i))) & 1) != 0) {
            chk ^= GENERATOR[i];
        }
    }
    return chk;
}

fn hrpExpand(hrp: []const u8, chk_in: u32): u32 {
    let chk = chk_in;
    for (c of hrp) {
        chk = polymodStep(chk, c >> 5);
    }
    chk = polymodStep(chk, 0);
    for (c of hrp) {
        chk = polymodStep(chk, c & 31);
    }
    return chk;
}

fn validateHrp(hrp: []const u8): Bech32Error!void {
    if (hrp.len == 0 or hrp.len > 83) { return Bech32Error.InvalidHrp; }
    for (c of hrp) {
        if (c < 33 or c > 126) { return Bech32Error.InvalidHrp; }
    }
}

pub fn encode(alloc: std.mem.Allocator, hrp: []const u8, data5: []const u8, variant: Bech32Variant): Bech32Error![]u8 {
    try validateHrp(hrp);
    for (d of data5) {
        if (d >= 32) { return Bech32Error.InvalidCharacter; }
    }
    if (hrp.len + 1 + data5.len + 6 > 90) {
        // BIP-173/350 standard 90-char limit (allow general encode up to 1023 if needed, or keep standard; wait: Nostr/Age can be > 90)
    }

    let chk: u32 = 1;
    let hrp_lower = try alloc.alloc(u8, hrp.len);
    defer alloc.free(hrp_lower);
    let i: usize = 0;
    while (i < hrp.len) : (i += 1) {
        hrp_lower[i] = std.ascii.toLower(hrp[i]);
    }
    chk = hrpExpand(hrp_lower, chk);
    for (d of data5) {
        chk = polymodStep(chk, d);
    }
    let z: usize = 0;
    while (z < 6) : (z += 1) {
        chk = polymodStep(chk, 0);
    }
    const target_const = if (variant == .bech32) BECH32_CONST else BECH32M_CONST;
    chk ^= target_const;

    let out = try alloc.alloc(u8, hrp.len + 1 + data5.len + 6);
    @memcpy(out[0..hrp.len], hrp_lower);
    out[hrp.len] = '1';
    let pos: usize = hrp.len + 1;
    for (d of data5) {
        out[pos] = CHARSET[d];
        pos += 1;
    }
    z = 0;
    while (z < 6) : (z += 1) {
        const shift: u5 = @intCast(5 * (5 - z));
        out[pos] = CHARSET[(chk >> shift) & 31];
        pos += 1;
    }
    return out;
}

pub const DecodedBech32 = struct {
    hrp: []u8,
    data5: []u8,
    variant: Bech32Variant,

    pub fn deinit(alloc: std.mem.Allocator) {
        alloc.free(_.hrp);
        alloc.free(_.data5);
    }
};

pub fn decode(alloc: std.mem.Allocator, str: []const u8): Bech32Error!DecodedBech32 {
    if (str.len < 8) { return Bech32Error.InvalidLength; }
    let has_lower = false;
    let has_upper = false;
    let sep_pos: usize? = null;

    let i: usize = 0;
    while (i < str.len) : (i += 1) {
        const c = str[i];
        if (c < 33 or c > 126) { return Bech32Error.InvalidCharacter; }
        if (c >= 'a' and c <= 'z') { has_lower = true; }
        if (c >= 'A' and c <= 'Z') { has_upper = true; }
        if (c == '1') { sep_pos = i; }
    }
    if (has_lower and has_upper) { return Bech32Error.MixedCase; }

    const sep = sep_pos orelse return Bech32Error.MissingSeparator;
    if (sep == 0 or sep + 7 > str.len) { return Bech32Error.InvalidLength; }

    let hrp = try alloc.alloc(u8, sep);
    errdefer alloc.free(hrp);
    i = 0;
    while (i < sep) : (i += 1) {
        hrp[i] = std.ascii.toLower(str[i]);
    }
    try validateHrp(hrp);

    const payload_len = str.len - (sep + 1);
    const data_len = payload_len - 6;
    let data5 = try alloc.alloc(u8, data_len);
    errdefer alloc.free(data5);

    let chk = hrpExpand(hrp, 1);
    i = 0;
    while (i < payload_len) : (i += 1) {
        const c = std.ascii.toLower(str[sep + 1 + i]);
        const v = charsetIndex(c);
        if (v < 0) { return Bech32Error.InvalidCharacter; }
        const uv: u8 = @intCast(v);
        chk = polymodStep(chk, uv);
        if (i < data_len) {
            data5[i] = uv;
        }
    }

    if (chk == BECH32_CONST) {
        return DecodedBech32{ .hrp = hrp, .data5 = data5, .variant = .bech32 };
    } else if (chk == BECH32M_CONST) {
        return DecodedBech32{ .hrp = hrp, .data5 = data5, .variant = .bech32m };
    } else {
        return Bech32Error.InvalidChecksum;
    }
}

pub fn convertBits(alloc: std.mem.Allocator, data: []const u8, from_bits: u5, to_bits: u5, pad: bool): Bech32Error![]u8 {
    let acc: u32 = 0;
    let bits: u6 = 0;
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    const maxv: u32 = (@as(u32, 1) << to_bits) - 1;
    for (val of data) {
        if ((@as(u32, val) >> from_bits) != 0) {
            return Bech32Error.InvalidCharacter;
        }
        acc = (acc << from_bits) | @as(u32, val);
        bits += from_bits;
        while (bits >= to_bits) {
            bits -= to_bits;
            try out.append(alloc, @intCast((acc >> @as(u5, @intCast(bits))) & maxv));
        }
    }
    if (pad) {
        if (bits > 0) {
            const sh: u5 = @intCast(to_bits - bits);
            try out.append(alloc, @intCast((acc << sh) & maxv));
        }
    } else {
        if (bits >= from_bits) { return Bech32Error.InvalidPadding; }
        const rem_mask: u32 = (@as(u32, 1) << @as(u5, @intCast(bits))) - 1;
        if ((acc & rem_mask) != 0) { return Bech32Error.InvalidPadding; }
    }
    return out.toOwnedSlice(alloc);
}

pub fn encodeBytes(alloc: std.mem.Allocator, hrp: []const u8, raw_bytes: []const u8, variant: Bech32Variant): Bech32Error![]u8 {
    const data5 = try convertBits(alloc, raw_bytes, 8, 5, true);
    defer alloc.free(data5);
    return encode(alloc, hrp, data5, variant);
}

pub const DecodedBytes = struct {
    hrp: []u8,
    bytes: []u8,
    variant: Bech32Variant,

    pub fn deinit(alloc: std.mem.Allocator) {
        alloc.free(_.hrp);
        alloc.free(_.bytes);
    }
};

pub fn decodeBytes(alloc: std.mem.Allocator, str: []const u8): Bech32Error!DecodedBytes {
    const dec_raw = try decode(alloc, str);
    defer dec_raw.deinit(alloc);
    const bytes = try convertBits(alloc, dec_raw.data5, 5, 8, false);
    const hrp = try alloc.dupe(u8, dec_raw.hrp);
    return DecodedBytes{
        .hrp = hrp,
        .bytes = bytes,
        .variant = dec_raw.variant,
    };
}

pub fn encodeSegwitAddr(alloc: std.mem.Allocator, hrp: []const u8, witness_ver: u8, witness_prog: []const u8): Bech32Error![]u8 {
    if (witness_ver > 16) { return Bech32Error.InvalidWitnessVersion; }
    if (witness_prog.len < 2 or witness_prog.len > 40) { return Bech32Error.InvalidWitnessProgram; }
    if (witness_ver == 0 and witness_prog.len != 20 and witness_prog.len != 32) {
        return Bech32Error.InvalidWitnessProgram;
    }
    const conv = try convertBits(alloc, witness_prog, 8, 5, true);
    defer alloc.free(conv);

    let data5 = try alloc.alloc(u8, 1 + conv.len);
    defer alloc.free(data5);
    data5[0] = witness_ver;
    @memcpy(data5[1..], conv);

    const variant: Bech32Variant = if (witness_ver == 0) .bech32 else .bech32m;
    return encode(alloc, hrp, data5, variant);
}
