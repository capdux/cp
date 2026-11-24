// Reference Only: Functional overlap with lib/crypto/kdf.cp | Kept for comparison/analysis
// Source: native-cp-stdlib (RFC 7914 scrypt KDF)

import * as std from "std";

pub const DEFAULT_MAX_MEM_BYTES: usize = 64 * 1024 * 1024;
pub const DEFAULT_SALT_LEN: usize = 16;
pub const DEFAULT_HASH_LEN: usize = 32;
pub const MIN_LN: u6 = 1;
pub const MAX_LN: u6 = 22;

pub struct ScryptParams {
    ln: u6,
    r: u30,
    p: u30,
    max_mem_bytes: usize,

    pub static fn init(ln: u6, r: u30, p: u30, max_mem_bytes: usize): ScryptParams {
        return ScryptParams{
            .ln = ln,
            .r = r,
            .p = p,
            .max_mem_bytes = max_mem_bytes,
        };
    }

    pub fn fromN(n: u64, r: u30, p: u30, max_mem_bytes: usize): !ScryptParams {
        if (n < 2) {
            return error.InvalidParams;
        }
        if ((n & (n - 1)) != 0) {
            return error.InvalidParams;
        }
        const trailing = @ctz(n);
        if (trailing < MIN_LN || trailing > MAX_LN) {
            return error.InvalidParams;
        }
        const ln_val: u6 = @intCast(trailing);
        const params = ScryptParams{
            .ln = ln_val,
            .r = r,
            .p = p,
            .max_mem_bytes = max_mem_bytes,
        };
        try params.validate();
        return params;
    }

    pub fn interactive(): ScryptParams {
        return ScryptParams{
            .ln = 14,
            .r = 8,
            .p = 1,
            .max_mem_bytes = DEFAULT_MAX_MEM_BYTES,
        };
    }

    pub fn owasp(): ScryptParams {
        return ScryptParams{
            .ln = 17,
            .r = 8,
            .p = 1,
            .max_mem_bytes = 256 * 1024 * 1024,
        };
    }

    pub fn estimatedMemBytes(): !usize {
        if (_.ln < MIN_LN || _.ln > MAX_LN) {
            return error.InvalidParams;
        }
        const n_u128: u128 = @as(u128, 1) << _.ln;
        const r_u128: u128 = @as(u128, _.r);
        const mem_u128: u128 = @as(u128, 128) * r_u128 * n_u128;
        if (mem_u128 > @as(u128, std.math.maxInt(usize))) {
            return error.MemoryLimitExceeded;
        }
        return @intCast(mem_u128);
    }

    pub fn validate(): !void {
        if (_.ln < MIN_LN || _.ln > MAX_LN) {
            return error.InvalidParams;
        }
        if (_.r == 0 || _.p == 0) {
            return error.InvalidParams;
        }
        const rp: u64 = @as(u64, _.r) * @as(u64, _.p);
        if (rp >= (@as(u64, 1) << 30)) {
            return error.InvalidParams;
        }
        const est = try _.estimatedMemBytes();
        if (est > _.max_mem_bytes) {
            return error.MemoryLimitExceeded;
        }
    }

    pub fn toZigParams(): std.crypto.pwhash.scrypt.Params {
        return std.crypto.pwhash.scrypt.Params{
            .ln = _.ln,
            .r = _.r,
            .p = _.p,
        };
    }
}

pub struct ParsedPhc {
    params: ScryptParams,
    salt: []u8,
    hash: []u8,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.salt);
        alloc.free(_.hash);
    }
}

fn constantTimeEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let diff: u8 = 0;
    for (i of 0..a.len) {
        diff = diff | (a[i] ^ b[i]);
    }
    return std.crypto.timing_safe.eql([1]u8, .{diff}, .{0});
}

pub fn hashRaw(
    alloc: std.mem.Allocator,
    password: []const u8,
    salt: []const u8,
    params: ScryptParams,
    out_len: usize,
): ![]u8 {
    if (out_len == 0) {
        return error.InvalidParams;
    }
    try params.validate();

    const dk = try alloc.alloc(u8, out_len);
    errdefer alloc.free(dk);

    try std.crypto.pwhash.scrypt.kdf(
        alloc,
        dk,
        password,
        salt,
        params.toZigParams(),
    );
    return dk;
}

pub fn hashPhc(
    alloc: std.mem.Allocator,
    password: []const u8,
    salt: []const u8,
    params: ScryptParams,
    out_len: usize,
): ![]u8 {
    const dk = try hashRaw(alloc, password, salt, params, out_len);
    defer alloc.free(dk);

    const enc = std.base64.standard_no_pad.Encoder;
    const salt_b64_len = enc.calcSize(salt.len);
    const hash_b64_len = enc.calcSize(dk.len);

    const salt_b64 = try alloc.alloc(u8, salt_b64_len);
    defer alloc.free(salt_b64);
    * = enc.encode(salt_b64, salt);

    const hash_b64 = try alloc.alloc(u8, hash_b64_len);
    defer alloc.free(hash_b64);
    * = enc.encode(hash_b64, dk);

    return std.fmt.allocPrint(
        alloc,
        "$scrypt$ln={d},r={d},p={d}${s}${s}",
        .{ params.ln, params.r, params.p, salt_b64, hash_b64 },
    );
}

pub fn hashPhcRandom(
    alloc: std.mem.Allocator,
    password: []const u8,
    params: ScryptParams,
): ![]u8 {
    let salt: [DEFAULT_SALT_LEN]u8 = undefined;
    let io_threaded = std.Io.Threaded.init(alloc, .{});
    defer io_threaded.deinit();
    io_threaded.io().random(&salt);

    return hashPhc(alloc, password, &salt, params, DEFAULT_HASH_LEN);
}

pub fn parsePhc(alloc: std.mem.Allocator, phc: []const u8): !ParsedPhc {
    if (phc.len == 0 || phc[0] != '$') {
        return error.InvalidPhcFormat;
    }

    let it = std.mem.splitScalar(u8, phc[1..], '$');
    const id = it.next() ?? return error.InvalidPhcFormat;
    if (!std.mem.eql(u8, id, "scrypt")) {
        return error.InvalidPhcFormat;
    }

    const param_str = it.next() ?? return error.InvalidPhcFormat;
    const salt_b64 = it.next() ?? return error.InvalidPhcFormat;
    const hash_b64 = it.next() ?? return error.InvalidPhcFormat;
    if (it.next() != null) {
        return error.InvalidPhcFormat;
    }

    let ln_opt: u6? = null;
    let r_opt: u30? = null;
    let p_opt: u30? = null;

    let param_it = std.mem.splitScalar(u8, param_str, ',');
    while (kv := param_it.next()) {
        const eq_idx = std.mem.indexOfScalar(u8, kv, '=') ?? return error.InvalidPhcFormat;
        const key = kv[0..eq_idx];
        const val = kv[eq_idx + 1 ..];
        if (std.mem.eql(u8, key, "ln")) {
            if (ln_opt != null) return error.InvalidPhcFormat;
            ln_opt = std.fmt.parseInt(u6, val, 10) catch return error.InvalidPhcFormat;
        } else if (std.mem.eql(u8, key, "r")) {
            if (r_opt != null) return error.InvalidPhcFormat;
            r_opt = std.fmt.parseInt(u30, val, 10) catch return error.InvalidPhcFormat;
        } else if (std.mem.eql(u8, key, "p")) {
            if (p_opt != null) return error.InvalidPhcFormat;
            p_opt = std.fmt.parseInt(u30, val, 10) catch return error.InvalidPhcFormat;
        } else {
            return error.InvalidPhcFormat;
        }
    }

    const ln_val = ln_opt ?? return error.InvalidPhcFormat;
    const r_val = r_opt ?? return error.InvalidPhcFormat;
    const p_val = p_opt ?? return error.InvalidPhcFormat;

    const decoder = std.base64.standard_no_pad.Decoder;
    const salt_len = decoder.calcSizeForSlice(salt_b64) catch return error.InvalidPhcFormat;
    const hash_len = decoder.calcSizeForSlice(hash_b64) catch return error.InvalidPhcFormat;
    if (hash_len == 0) {
        return error.InvalidPhcFormat;
    }

    const salt_buf = try alloc.alloc(u8, salt_len);
    errdefer alloc.free(salt_buf);
    decoder.decode(salt_buf, salt_b64) catch return error.InvalidPhcFormat;

    const hash_buf = try alloc.alloc(u8, hash_len);
    errdefer alloc.free(hash_buf);
    decoder.decode(hash_buf, hash_b64) catch return error.InvalidPhcFormat;

    const params = ScryptParams.init(ln_val, r_val, p_val, DEFAULT_MAX_MEM_BYTES);
    return ParsedPhc{
        .params = params,
        .salt = salt_buf,
        .hash = hash_buf,
    };
}

pub fn verifyPhcWithLimit(
    alloc: std.mem.Allocator,
    password: []const u8,
    phc: []const u8,
    max_mem_bytes: usize,
): !bool {
    let parsed = try parsePhc(alloc, phc);
    defer parsed.deinit(alloc);

    let params = parsed.params;
    params.max_mem_bytes = max_mem_bytes;
    try params.validate();

    const derived = try hashRaw(alloc, password, parsed.salt, params, parsed.hash.len);
    defer alloc.free(derived);

    return constantTimeEql(derived, parsed.hash);
}

pub fn verifyPhc(
    alloc: std.mem.Allocator,
    password: []const u8,
    phc: []const u8,
): !bool {
    return verifyPhcWithLimit(alloc, password, phc, DEFAULT_MAX_MEM_BYTES);
}
