// Source: cp/lib/argon2/engine.cp (RFC 9106 Argon2 KDF, PHC encoding/decoding, and constant-time verification)

const std = @import("std");
import {
    ARGON2_VERSION,
    DEFAULT_MAX_M_COST_KIB,
    Argon2Error,
    Argon2Variant,
    Argon2Params,
    PhcParsed,
} from "./types.cp";

fn toStdMode(variant: Argon2Variant): std.crypto.argon2.Mode {
    return switch (variant) {
        .Argon2d => .argon2d,
        .Argon2i => .argon2i,
        .Argon2id => .argon2id,
    };
}

pub fn hashRaw(
    alloc: std.mem.Allocator,
    password: []const u8,
    salt: []const u8,
    params: Argon2Params,
): Argon2Error![]u8 {
    try params.validate(salt.len);

    const out = alloc.alloc(u8, params.out_len) catch return Argon2Error.OutOfMemory;
    errdefer alloc.free(out);

    zig {
        var threaded: std.Io.Threaded = .init(alloc, .{});
        defer threaded.deinit();

        const kdf_params: std.crypto.argon2.Params = .{
            .t = params.t_cost,
            .m = params.m_cost_kib,
            .p = params.lanes,
            .secret = if (params.secret.len > 0) params.secret else null,
            .ad = if (params.ad.len > 0) params.ad else null,
        };

        std.crypto.argon2.kdf(
            alloc,
            out,
            password,
            salt,
            kdf_params,
            toStdMode(params.variant),
            threaded.io(),
        ) catch |err| switch (err) {
            error.OutOfMemory => return Argon2Error.OutOfMemory,
            error.OutputTooLong => return Argon2Error.OutputTooLong,
            error.WeakParameters => return Argon2Error.InvalidParams,
        };
    }

    return out;
}

pub fn hashPhc(
    alloc: std.mem.Allocator,
    password: []const u8,
    salt: []const u8,
    params: Argon2Params,
): Argon2Error![]u8 {
    const raw = try hashRaw(alloc, password, salt, params);
    defer alloc.free(raw);

    const b64_enc = std.base64.standard_no_pad.Encoder;
    const salt_b64_len = b64_enc.calcSize(salt.len);
    const hash_b64_len = b64_enc.calcSize(raw.len);

    const salt_b64 = alloc.alloc(u8, salt_b64_len) catch return Argon2Error.OutOfMemory;
    defer alloc.free(salt_b64);
    * = b64_enc.encode(salt_b64, salt);

    const hash_b64 = alloc.alloc(u8, hash_b64_len) catch return Argon2Error.OutOfMemory;
    defer alloc.free(hash_b64);
    * = b64_enc.encode(hash_b64, raw);

    return std.fmt.allocPrint(
        alloc,
        "${s}$v={d}$m={d},t={d},p={d}${s}${s}",
        .{
            params.variant.toPhcId(),
            ARGON2_VERSION,
            params.m_cost_kib,
            params.t_cost,
            params.lanes,
            salt_b64,
            hash_b64,
        },
    ) catch return Argon2Error.OutOfMemory;
}

pub fn hashPhcRandom(
    alloc: std.mem.Allocator,
    password: []const u8,
    params: Argon2Params,
): Argon2Error![]u8 {
    let salt: [16]u8 = undefined;
    zig {
        var threaded: std.Io.Threaded = .init(alloc, .{});
        defer threaded.deinit();
        threaded.io().random(&salt);
    }
    return hashPhc(alloc, password, &salt, params);
}

fn decodeB64Alloc(alloc: std.mem.Allocator, encoded: []const u8): Argon2Error![]u8 {
    if (encoded.len == 0) return Argon2Error.InvalidPhcFormat;
    const b64_dec = std.base64.standard_no_pad.Decoder;
    const decoded_len = b64_dec.calcSizeForSlice(encoded) catch return Argon2Error.InvalidPhcFormat;
    const out = alloc.alloc(u8, decoded_len) catch return Argon2Error.OutOfMemory;
    errdefer alloc.free(out);
    b64_dec.decode(out, encoded) catch return Argon2Error.InvalidPhcFormat;
    return out;
}

pub fn parsePhc(
    alloc: std.mem.Allocator,
    phc_str: []const u8,
    max_m_cost_kib: u32,
): Argon2Error!PhcParsed {
    if (phc_str.len < 2 or phc_str[0] != '$') {
        return Argon2Error.InvalidPhcFormat;
    }

    let it = std.mem.splitScalar(u8, phc_str[1..], '$');
    const variant_part = it.next() orelse return Argon2Error.InvalidPhcFormat;
    const version_part = it.next() orelse return Argon2Error.InvalidPhcFormat;
    const params_part = it.next() orelse return Argon2Error.InvalidPhcFormat;
    const salt_part = it.next() orelse return Argon2Error.InvalidPhcFormat;
    const hash_part = it.next() orelse return Argon2Error.InvalidPhcFormat;
    if (it.next() != null) {
        return Argon2Error.InvalidPhcFormat;
    }

    const variant = try Argon2Variant.fromPhcId(variant_part);

    if (!std.mem.startsWith(u8, version_part, "v=")) {
        return Argon2Error.InvalidPhcFormat;
    }
    const ver = std.fmt.parseInt(u32, version_part[2..], 10) catch return Argon2Error.InvalidPhcFormat;
    if (ver != ARGON2_VERSION) {
        return Argon2Error.UnsupportedVersion;
    }

    let p_it = std.mem.splitScalar(u8, params_part, ',');
    const m_part = p_it.next() orelse return Argon2Error.InvalidPhcFormat;
    const t_part = p_it.next() orelse return Argon2Error.InvalidPhcFormat;
    const l_part = p_it.next() orelse return Argon2Error.InvalidPhcFormat;
    if (p_it.next() != null) {
        return Argon2Error.InvalidPhcFormat;
    }

    if (!std.mem.startsWith(u8, m_part, "m=") or
        !std.mem.startsWith(u8, t_part, "t=") or
        !std.mem.startsWith(u8, l_part, "p="))
    {
        return Argon2Error.InvalidPhcFormat;
    }

    const m_cost = std.fmt.parseInt(u32, m_part[2..], 10) catch return Argon2Error.InvalidPhcFormat;
    const t_cost = std.fmt.parseInt(u32, t_part[2..], 10) catch return Argon2Error.InvalidPhcFormat;
    const lanes = std.fmt.parseInt(u24, l_part[2..], 10) catch return Argon2Error.InvalidPhcFormat;

    if (t_cost < 1 or lanes < 1) {
        return Argon2Error.InvalidParams;
    }
    const min_m: u64 = @as(u64, 8) * @as(u64, lanes);
    if (@as(u64, m_cost) < min_m) {
        return Argon2Error.InvalidParams;
    }
    if (m_cost > max_m_cost_kib) {
        return Argon2Error.MemoryLimitExceeded;
    }

    const salt = try decodeB64Alloc(alloc, salt_part);
    errdefer alloc.free(salt);
    if (salt.len < 8) {
        return Argon2Error.InvalidSalt;
    }

    const hash = try decodeB64Alloc(alloc, hash_part);
    errdefer alloc.free(hash);
    if (hash.len < 4) {
        return Argon2Error.OutputTooShort;
    }

    return PhcParsed{
        .variant = variant,
        .version = ver,
        .m_cost_kib = m_cost,
        .t_cost = t_cost,
        .lanes = lanes,
        .salt = salt,
        .hash = hash,
    };
}

pub fn verifyPhcWithLimit(
    alloc: std.mem.Allocator,
    password: []const u8,
    phc_str: []const u8,
    max_m_cost_kib: u32,
): Argon2Error!bool {
    let parsed = try parsePhc(alloc, phc_str, max_m_cost_kib);
    defer parsed.deinit(alloc);

    const params = Argon2Params{
        .variant = parsed.variant,
        .t_cost = parsed.t_cost,
        .m_cost_kib = parsed.m_cost_kib,
        .lanes = parsed.lanes,
        .out_len = parsed.hash.len,
        .max_m_cost_kib = max_m_cost_kib,
    };

    const computed = try hashRaw(alloc, password, parsed.salt, params);
    defer alloc.free(computed);

    if (computed.len != parsed.hash.len) {
        return false;
    }

    let diff: u8 = 0;
    let i: usize = 0;
    while (i < computed.len) : (i += 1) {
        diff |= computed[i] ^ parsed.hash[i];
    }
    return diff == 0;
}

pub fn verifyPhc(
    alloc: std.mem.Allocator,
    password: []const u8,
    phc_str: []const u8,
): Argon2Error!bool {
    return verifyPhcWithLimit(alloc, password, phc_str, DEFAULT_MAX_M_COST_KIB);
}
