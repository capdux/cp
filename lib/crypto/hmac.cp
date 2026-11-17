// Source: UNVERIFIED (origin to be confirmed: Bun / Zig std / custom)
// lib/crypto/hmac.cp
// HMAC message authentication code (SHA256, SHA512)


pub enum HmacAlgorithm {
    Sha256,
    Sha512,

    static fn fromString(str: []const u8): HmacAlgorithm? {
        if (std.mem.eql(u8, str, "sha256") or std.mem.eql(u8, str, "SHA256")) return HmacAlgorithm.Sha256;
        if (std.mem.eql(u8, str, "sha512") or std.mem.eql(u8, str, "SHA512")) return HmacAlgorithm.Sha512;
        return null;
    }
}

pub fn hmacSha256(key: string, data: string): string {
    let out: [32]u8 = undefined;
    std.crypto.auth.hmac.sha2.HmacSha256.create(&out, data, key);
    const hex = std.fmt.bytesToHex(out, .lower);
    const res = std.heap.page_allocator.alloc(u8, 64) catch unreachable;
    @memcpy(res, hex[0..64]);
    return res;
}

pub fn hmacSha256Raw(out: *[32]u8, key: string, data: string) {
    std.crypto.auth.hmac.sha2.HmacSha256.create(out, data, key);
}

pub class Hmac {
    algo: HmacAlgorithm = HmacAlgorithm.Sha256;
    h256: std.crypto.auth.hmac.sha2.HmacSha256 = undefined;
    h512: std.crypto.auth.hmac.sha2.HmacSha512 = undefined;

    pub static fn init(algo: HmacAlgorithm, key: string): *Hmac {
        let h = std.heap.page_allocator.create(Hmac) catch unreachable;
        h.* = Hmac{};
        h.algo = algo;
        switch (algo) {
            .Sha256 => { h.h256 = std.crypto.auth.hmac.sha2.HmacSha256.init(key); },
            .Sha512 => { h.h512 = std.crypto.auth.hmac.sha2.HmacSha512.init(key); },
        }
        return h;
    }

    pub fn update(*_, data: string): *Hmac {
        switch (_.algo) {
            .Sha256 => { _.h256.update(data); },
            .Sha512 => { _.h512.update(data); },
        }
        return _;
    }

    pub fn digest(*_): string {
        switch (_.algo) {
            .Sha256 => {
                let out: [32]u8 = undefined;
                _.h256.final(&out);
                const hex = std.fmt.bytesToHex(out, .lower);
                const res = std.heap.page_allocator.alloc(u8, 64) catch unreachable;
                @memcpy(res, hex[0..64]);
                return res;
            },
            .Sha512 => {
                let out: [64]u8 = undefined;
                _.h512.final(&out);
                const hex = std.fmt.bytesToHex(out, .lower);
                const res = std.heap.page_allocator.alloc(u8, 128) catch unreachable;
                @memcpy(res, hex[0..128]);
                return res;
            },
        }
    }
}

pub fn createHmac(algo: string, key: string): (*Hmac)? {
    const parsed = HmacAlgorithm.fromString(algo);
    if (parsed == null) {
        return null;
    }
    return Hmac.init(parsed.?, key);
}
