// Source: cp/lib/shamir/shamir.cp (Pure CP GF(2^8) Shamir's Secret Sharing)
const std = @import("std");

pub const ShamirError = error{
    EmptySecret,
    InvalidThreshold,
    InsufficientShares,
    DuplicateShareIndex,
    InvalidShareFormat,
    ShareLengthMismatch,
    InvalidShareIndex,
    OutOfMemory,
};

const GF_POLY: u16 = 0x11d;

fn gfMul(a_in: u8, b_in: u8): u8 {
    let a: u16 = a_in;
    let b: u16 = b_in;
    let res: u16 = 0;
    let i: u8 = 0;
    while (i < 8) : (i += 1) {
        if ((b & 1) != 0) { res ^= a; }
        const hi = (a & 0x80) != 0;
        a <<= 1;
        if (hi) { a ^= GF_POLY; }
        b >>= 1;
    }
    return @truncate(res & 0xFF);
}

fn gfPow(base_in: u8, exp_in: u8): u8 {
    let res: u8 = 1;
    let base = base_in;
    let exp = exp_in;
    while (exp > 0) {
        if ((exp & 1) != 0) { res = gfMul(res, base); }
        base = gfMul(base, base);
        exp >>= 1;
    }
    return res;
}

fn gfInv(a: u8): u8 {
    if (a == 0) { return 0; }
    return gfPow(a, 254);
}

fn evalPoly(coeffs: []const u8, x: u8): u8 {
    // Horner's method: coeffs[0] is constant term (secret byte)
    let acc: u8 = 0;
    let idx: usize = coeffs.len;
    while (idx > 0) {
        idx -= 1;
        acc = gfMul(acc, x) ^ coeffs[idx];
    }
    return acc;
}

pub const Share = struct {
    index: u8, // 1..255 (never 0)
    threshold: u8,
    data: []u8,

    pub fn deinit(self: *const Share, alloc: std.mem.Allocator) {
        alloc.free(self.data);
    }

    pub fn encode(self: Share, alloc: std.mem.Allocator): ShamirError![]u8 {
        if (self.index == 0) { return ShamirError.InvalidShareIndex; }
        // Format: "CPSS1" (5) + threshold (1) + index (1) + data (N)
        let out = try alloc.alloc(u8, 7 + self.data.len);
        @memcpy(out[0..5], "CPSS1");
        out[5] = self.threshold;
        out[6] = self.index;
        @memcpy(out[7..], self.data);
        return out;
    }

    pub fn decode(alloc: std.mem.Allocator, raw: []const u8): ShamirError!Share {
        if (raw.len < 8 or !std.mem.eql(u8, raw[0..5], "CPSS1")) {
            return ShamirError.InvalidShareFormat;
        }
        const threshold = raw[5];
        const index = raw[6];
        if (threshold < 2) { return ShamirError.InvalidThreshold; }
        if (index == 0) { return ShamirError.InvalidShareIndex; }
        const data = try alloc.dupe(u8, raw[7..]);
        return Share{
            .index = index,
            .threshold = threshold,
            .data = data,
        };
    }
};

pub fn freeShares(alloc: std.mem.Allocator, shares: []Share) {
    for (s of shares) {
        s.deinit(alloc);
    }
    alloc.free(shares);
}

pub fn splitSecret(alloc: std.mem.Allocator, secret: []const u8, threshold: u8, total_shares: u8): ShamirError![]Share {
    if (secret.len == 0) { return ShamirError.EmptySecret; }
    if (threshold < 2 or threshold > total_shares) { return ShamirError.InvalidThreshold; }

    let shares = try alloc.alloc(Share, total_shares);
    let allocated_count: usize = 0;
    errdefer {
        let j: usize = 0;
        while (j < allocated_count) : (j += 1) {
            alloc.free(shares[j].data);
        }
        alloc.free(shares);
    }

    let s_idx: usize = 0;
    while (s_idx < total_shares) : (s_idx += 1) {
        const buf = try alloc.alloc(u8, secret.len);
        shares[s_idx] = Share{
            .index = @intCast(s_idx + 1),
            .threshold = threshold,
            .data = buf,
        };
        allocated_count += 1;
    }

    let coeffs = try alloc.alloc(u8, threshold);
    defer alloc.free(coeffs);

    let byte_i: usize = 0;
    while (byte_i < secret.len) : (byte_i += 1) {
        coeffs[0] = secret[byte_i];
        std.crypto.random.bytes(coeffs[1..threshold]);
        s_idx = 0;
        while (s_idx < total_shares) : (s_idx += 1) {
            shares[s_idx].data[byte_i] = evalPoly(coeffs, shares[s_idx].index);
        }
    }

    return shares;
}

pub fn combineShares(alloc: std.mem.Allocator, shares: []const Share): ShamirError![]u8 {
    if (shares.len < 2) { return ShamirError.InsufficientShares; }
    const secret_len = shares[0].data.len;
    if (secret_len == 0) { return ShamirError.EmptySecret; }
    const k = shares[0].threshold;
    if (shares.len < k) { return ShamirError.InsufficientShares; }

    // Validate indices & uniqueness among first k (or all) shares
    let seen: [256]bool = undefined;
    @memset(&seen, false);
    for (s of shares) {
        if (s.index == 0) { return ShamirError.InvalidShareIndex; }
        if (s.data.len != secret_len) { return ShamirError.ShareLengthMismatch; }
        if (seen[s.index]) { return ShamirError.DuplicateShareIndex; }
        seen[s.index] = true;
    }

    // Precompute Lagrange basis coefficients at x = 0:
    // L_i(0) = prod_{j != i} (x_j / (x_j ^ x_i))
    const used = shares[0..k];
    let lagrange = try alloc.alloc(u8, k);
    defer alloc.free(lagrange);

    let i: usize = 0;
    while (i < k) : (i += 1) {
        let num: u8 = 1;
        let den: u8 = 1;
        let j: usize = 0;
        while (j < k) : (j += 1) {
            if (i == j) { continue; }
            num = gfMul(num, used[j].index);
            den = gfMul(den, used[j].index ^ used[i].index);
        }
        lagrange[i] = gfMul(num, gfInv(den));
    }

    let secret = try alloc.alloc(u8, secret_len);
    let byte_i: usize = 0;
    while (byte_i < secret_len) : (byte_i += 1) {
        let acc: u8 = 0;
        i = 0;
        while (i < k) : (i += 1) {
            acc ^= gfMul(used[i].data[byte_i], lagrange[i]);
        }
        secret[byte_i] = acc;
    }
    return secret;
}
