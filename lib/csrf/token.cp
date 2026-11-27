// Source: bun/src/csrf/csrf.cp -> cp/lib/csrf/token.cp (HMAC-SHA256 & BREACH masking)
// Pure CP CSRF signed token generation, constant-time verification, and BREACH masking.

const std = @import("std");
import {
    CsrfError,
    DEFAULT_EXPIRATION_MS,
    SECRET_LEN,
    NONCE_LEN,
    TIMESTAMP_LEN,
    MAC_LEN,
    RAW_TOKEN_LEN,
    MASK_PAD_LEN,
} from "./types.cp";

fn computeTokenMac(secret: []const u8, ts_bytes: *const [TIMESTAMP_LEN]u8, nonce: *const [NONCE_LEN]u8, session_id: []const u8): [MAC_LEN]u8 {
    let ctx = std.crypto.auth.hmac.sha2.HmacSha256.init(secret);
    ctx.update(ts_bytes);
    ctx.update(nonce);
    let sid_len_bytes: [4]u8 = [0; 4];
    std.mem.writeInt(u32, &sid_len_bytes, @intCast(session_id.len), .big);
    ctx.update(&sid_len_bytes);
    ctx.update(session_id);
    let mac: [MAC_LEN]u8 = [0; MAC_LEN];
    ctx.final(&mac);
    return mac;
}

fn currentTimestampMs(): u64 {
    const ms = std.time.milliTimestamp();
    if (ms < 0) {
        return 0;
    }
    return @intCast(ms);
}

pub fn generateSecret(out_buf: *[SECRET_LEN]u8): []u8 {
    std.crypto.random.bytes(out_buf);
    return out_buf[0..SECRET_LEN];
}

pub fn generateSecretAlloc(alloc: std.mem.Allocator): ![]u8 {
    const buf = alloc.alloc(u8, SECRET_LEN) catch return CsrfError.OutOfMemory;
    std.crypto.random.bytes(buf);
    return buf;
}

pub fn signTokenAt(alloc: std.mem.Allocator, secret: []const u8, session_id: []const u8, timestamp_ms: u64): ![]u8 {
    if (secret.len == 0) {
        return CsrfError.InvalidSecret;
    }

    let ts_bytes: [TIMESTAMP_LEN]u8 = [0; TIMESTAMP_LEN];
    std.mem.writeInt(u64, &ts_bytes, timestamp_ms, .big);

    let nonce: [NONCE_LEN]u8 = [0; NONCE_LEN];
    std.crypto.random.bytes(&nonce);

    const mac = computeTokenMac(secret, &ts_bytes, &nonce, session_id);

    let raw: [RAW_TOKEN_LEN]u8 = [0; RAW_TOKEN_LEN];
    @memcpy(raw[0..TIMESTAMP_LEN], &ts_bytes);
    @memcpy(raw[TIMESTAMP_LEN .. TIMESTAMP_LEN + NONCE_LEN], &nonce);
    @memcpy(raw[TIMESTAMP_LEN + NONCE_LEN .. RAW_TOKEN_LEN], &mac);

    const encoder = std.base64.url_safe_no_pad.Encoder;
    const enc_len = encoder.calcSize(RAW_TOKEN_LEN);
    let out = alloc.alloc(u8, enc_len) catch return CsrfError.OutOfMemory;
    let * = encoder.encode(out, &raw);
    return out;
}

pub fn signToken(alloc: std.mem.Allocator, secret: []const u8, session_id: []const u8): ![]u8 {
    return signTokenAt(alloc, secret, session_id, currentTimestampMs());
}

pub fn verifySignedTokenWithTtl(secret: []const u8, session_id: []const u8, token: []const u8, max_age_ms: u64): !void {
    if (secret.len == 0) {
        return CsrfError.InvalidSecret;
    }
    if (token.len == 0) {
        return CsrfError.InvalidToken;
    }

    const decoder = std.base64.url_safe_no_pad.Decoder;
    const expected_dec_len = decoder.calcSizeForSlice(token) catch return CsrfError.DecodingFailed;
    if (expected_dec_len != RAW_TOKEN_LEN) {
        return CsrfError.InvalidToken;
    }

    let raw: [RAW_TOKEN_LEN]u8 = [0; RAW_TOKEN_LEN];
    decoder.decode(&raw, token) catch return CsrfError.DecodingFailed;

    let ts_bytes: [TIMESTAMP_LEN]u8 = [0; TIMESTAMP_LEN];
    @memcpy(&ts_bytes, raw[0..TIMESTAMP_LEN]);
    let nonce: [NONCE_LEN]u8 = [0; NONCE_LEN];
    @memcpy(&nonce, raw[TIMESTAMP_LEN .. TIMESTAMP_LEN + NONCE_LEN]);
    let received_mac: [MAC_LEN]u8 = [0; MAC_LEN];
    @memcpy(&received_mac, raw[TIMESTAMP_LEN + NONCE_LEN .. RAW_TOKEN_LEN]);

    const expected_mac = computeTokenMac(secret, &ts_bytes, &nonce, session_id);
    if (!std.crypto.timing_safe.eql([MAC_LEN]u8, expected_mac, received_mac)) {
        return CsrfError.InvalidSignature;
    }

    if (max_age_ms > 0) {
        const ts = std.mem.readInt(u64, &ts_bytes, .big);
        const now = currentTimestampMs();
        if (ts > now) {
            return CsrfError.ExpiredToken;
        }
        if (now - ts > max_age_ms) {
            return CsrfError.ExpiredToken;
        }
    }
}

pub fn verifySignedToken(secret: []const u8, session_id: []const u8, token: []const u8): !void {
    return verifySignedTokenWithTtl(secret, session_id, token, DEFAULT_EXPIRATION_MS);
}

pub fn maskToken(alloc: std.mem.Allocator, raw_token: []const u8): ![]u8 {
    if (raw_token.len == 0) {
        return CsrfError.InvalidToken;
    }

    let pad: [MASK_PAD_LEN]u8 = [0; MASK_PAD_LEN];
    std.crypto.random.bytes(&pad);

    const combined_len = MASK_PAD_LEN + raw_token.len;
    let combined = alloc.alloc(u8, combined_len) catch return CsrfError.OutOfMemory;
    defer alloc.free(combined);

    @memcpy(combined[0..MASK_PAD_LEN], &pad);
    let i: usize = 0;
    while (i < raw_token.len) {
        combined[MASK_PAD_LEN + i] = raw_token[i] ^ pad[i % MASK_PAD_LEN];
        i += 1;
    }

    const encoder = std.base64.url_safe_no_pad.Encoder;
    const enc_len = encoder.calcSize(combined_len);
    let out = alloc.alloc(u8, enc_len) catch return CsrfError.OutOfMemory;
    let * = encoder.encode(out, combined);
    return out;
}

pub fn unmaskAndVerifyWithTtl(alloc: std.mem.Allocator, secret: []const u8, session_id: []const u8, masked_token: []const u8, max_age_ms: u64): !void {
    if (masked_token.len == 0) {
        return CsrfError.InvalidToken;
    }

    const decoder = std.base64.url_safe_no_pad.Decoder;
    const dec_len = decoder.calcSizeForSlice(masked_token) catch return CsrfError.DecodingFailed;
    if (dec_len <= MASK_PAD_LEN) {
        return CsrfError.InvalidToken;
    }

    const combined = alloc.alloc(u8, dec_len) catch return CsrfError.OutOfMemory;
    defer alloc.free(combined);
    decoder.decode(combined, masked_token) catch return CsrfError.DecodingFailed;

    const pad = combined[0..MASK_PAD_LEN];
    const xored = combined[MASK_PAD_LEN..];

    let raw_buf = alloc.alloc(u8, xored.len) catch return CsrfError.OutOfMemory;
    defer alloc.free(raw_buf);

    let i: usize = 0;
    while (i < xored.len) {
        raw_buf[i] = xored[i] ^ pad[i % MASK_PAD_LEN];
        i += 1;
    }

    return verifySignedTokenWithTtl(secret, session_id, raw_buf, max_age_ms);
}

pub fn unmaskAndVerify(alloc: std.mem.Allocator, secret: []const u8, session_id: []const u8, masked_token: []const u8): !void {
    return unmaskAndVerifyWithTtl(alloc, secret, session_id, masked_token, DEFAULT_EXPIRATION_MS);
}

pub struct CsrfProtector {
    secret: []const u8,
    max_age_ms: u64,

    pub static fn init(secret: []const u8): !CsrfProtector {
        return CsrfProtector.initWithMaxAge(secret, DEFAULT_EXPIRATION_MS);
    }

    pub static fn initWithMaxAge(secret: []const u8, max_age_ms: u64): !CsrfProtector {
        if (secret.len == 0) {
            return CsrfError.InvalidSecret;
        }
        return CsrfProtector{
            .secret = secret,
            .max_age_ms = max_age_ms,
        };
    }

    pub fn sign(alloc: std.mem.Allocator, session_id: []const u8): ![]u8 {
        return signToken(alloc, _.secret, session_id);
    }

    pub fn verify(session_id: []const u8, token: []const u8): !void {
        return verifySignedTokenWithTtl(_.secret, session_id, token, _.max_age_ms);
    }

    pub fn mask(alloc: std.mem.Allocator, raw_token: []const u8): ![]u8 {
        return maskToken(alloc, raw_token);
    }

    pub fn unmaskAndVerify(alloc: std.mem.Allocator, session_id: []const u8, masked_token: []const u8): !void {
        return unmaskAndVerifyWithTtl(alloc, _.secret, session_id, masked_token, _.max_age_ms);
    }
}
