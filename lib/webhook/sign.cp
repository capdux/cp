// Source: cp/lib/webhook/sign.cp (Standard Webhooks, GitHub & Stripe HMAC-SHA256 Verification)
// Pure CP webhook cryptographic signing and constant-time verification for
// Standard Webhooks (v1), GitHub (sha256), and Stripe (t=...,v1=...) schemes.

const std = @import("std");

/// Error set returned by webhook signature generation, parsing, and replay-window verification.
pub err WebhookError {
    InvalidSignature,
    TimestampExpired,
    TimestampFuture,
    MalformedHeader,
    OutOfMemory,
}

const HEX_DIGITS: []const u8 = "0123456789abcdef";

fn hexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn encodeHex32(mac: []const u8, out: []u8) {
    for (i of 0..32) {
        const b = mac[i];
        out[i * 2] = HEX_DIGITS[usize((b >> 4) & 0x0F)];
        out[i * 2 + 1] = HEX_DIGITS[usize(b & 0x0F)];
    }
}

fn decodeHex32(hex: []const u8, out: *[32]u8): bool {
    if (hex.len != 64) return false;
    for (i of 0..32) {
        const hi = hexNibble(hex[i * 2]) ?? return false;
        const lo = hexNibble(hex[i * 2 + 1]) ?? return false;
        out.*[i] = (hi << 4) | lo;
    }
    return true;
}

fn trimAsciiSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t')) {
        start += 1;
    }
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) {
        end -= 1;
    }
    return s[start..end];
}

fn parseStrictI64(raw: []const u8): i64? {
    if (raw.len == 0) return null;
    let i: usize = 0;
    if (raw[0] == '-') {
        if (raw.len == 1) return null;
        i = 1;
    }
    while (i < raw.len) : (i += 1) {
        if (raw[i] < '0' || raw[i] > '9') return null;
    }
    return std.fmt.parseInt(i64, raw, 10) catch return null;
}

fn checkTimestampWindow(ts_sec: i64, now_sec: i64, tolerance_sec: i64): void ! WebhookError {
    if (tolerance_sec >= 0) {
        const ts_128: i128 = ts_sec;
        const now_128: i128 = now_sec;
        const tol_128: i128 = tolerance_sec;
        if (ts_128 - now_128 > tol_128) {
            return WebhookError.TimestampFuture;
        }
        if (now_128 - ts_128 > tol_128) {
            return WebhookError.TimestampExpired;
        }
    }
}

/// Standard Webhooks (`webhook-signature`: `v1,<base64(HMAC-SHA256(secret, "{msg_id}.{ts}.{body}"))>`).
pub struct StandardWebhook {
    /// Generates a `v1,<base64>` signature header value. Caller owns the returned slice.
    pub static fn sign(
        alloc: std.mem.Allocator,
        secret: []const u8,
        msg_id: []const u8,
        ts_sec: i64,
        payload: []const u8,
    ): ([]u8) ! WebhookError {
        let ts_buf: [32]u8 = undefined;
        const ts_str = std.fmt.bufPrint(&ts_buf, "{d}", .{ts_sec}) catch return WebhookError.MalformedHeader;

        let h = std.crypto.auth.hmac.sha2.HmacSha256.init(secret);
        h.update(msg_id);
        h.update(".");
        h.update(ts_str);
        h.update(".");
        h.update(payload);
        let mac: [32]u8 = undefined;
        h.final(&mac);

        const encoder = std.base64.standard.Encoder;
        const b64_len = encoder.calcSize(32);
        let out = alloc.alloc(u8, 3 + b64_len) catch return WebhookError.OutOfMemory;
        out[0] = 'v';
        out[1] = '1';
        out[2] = ',';
        * = encoder.encode(out[3..], &mac);
        return out;
    }

    /// Verifies a Standard Webhooks signature header (supporting space-delimited key-rotation lists),
    /// constant-time HMAC-SHA256 validation, and replay-window enforcement.
    pub static fn verify(
        secret: []const u8,
        msg_id: []const u8,
        ts_sec: i64,
        payload: []const u8,
        sig_header: []const u8,
        now_sec: i64,
        tolerance_sec: i64,
    ): void ! WebhookError {
        let v1_count: usize = 0;
        let scan_pos: usize = 0;
        while (scan_pos < sig_header.len) {
            while (scan_pos < sig_header.len && (sig_header[scan_pos] == ' ' || sig_header[scan_pos] == '\t')) {
                scan_pos += 1;
            }
            if (scan_pos >= sig_header.len) break;
            const tok_start = scan_pos;
            while (scan_pos < sig_header.len && sig_header[scan_pos] != ' ' && sig_header[scan_pos] != '\t') {
                scan_pos += 1;
            }
            const tok = sig_header[tok_start..scan_pos];
            const comma_opt = std.mem.indexOfScalar(u8, tok, ',');
            if (comma_opt == null) {
                return WebhookError.MalformedHeader;
            }
            const comma_idx = comma_opt.?;
            if (comma_idx == 0 || comma_idx + 1 >= tok.len) {
                return WebhookError.MalformedHeader;
            }
            const version = tok[0..comma_idx];
            if (std.mem.eql(u8, version, "v1")) {
                v1_count += 1;
            }
        }
        if (v1_count == 0) {
            return WebhookError.MalformedHeader;
        }

        checkTimestampWindow(ts_sec, now_sec, tolerance_sec)?;

        let ts_buf: [32]u8 = undefined;
        const ts_str = std.fmt.bufPrint(&ts_buf, "{d}", .{ts_sec}) catch return WebhookError.MalformedHeader;

        let h = std.crypto.auth.hmac.sha2.HmacSha256.init(secret);
        h.update(msg_id);
        h.update(".");
        h.update(ts_str);
        h.update(".");
        h.update(payload);
        let expected_mac: [32]u8 = undefined;
        h.final(&expected_mac);

        const decoder = std.base64.standard.Decoder;
        let matched = false;
        let pos: usize = 0;
        while (pos < sig_header.len) {
            while (pos < sig_header.len && (sig_header[pos] == ' ' || sig_header[pos] == '\t')) {
                pos += 1;
            }
            if (pos >= sig_header.len) break;
            const tok_start = pos;
            while (pos < sig_header.len && sig_header[pos] != ' ' && sig_header[pos] != '\t') {
                pos += 1;
            }
            const tok = sig_header[tok_start..pos];
            const comma_idx = std.mem.indexOfScalar(u8, tok, ',') ?? continue;
            const version = tok[0..comma_idx];
            if (!std.mem.eql(u8, version, "v1")) continue;

            const sig_b64 = tok[comma_idx + 1 ..];
            const dec_len = decoder.calcSizeForSlice(sig_b64) catch continue;
            if (dec_len != 32) continue;

            let decoded: [32]u8 = undefined;
            decoder.decode(&decoded, sig_b64) catch continue;
            if (std.crypto.timing_safe.eql([32]u8, expected_mac, decoded)) {
                matched = true;
            }
        }

        if (!matched) {
            return WebhookError.InvalidSignature;
        }
    }
}

/// GitHub Webhooks (`X-Hub-Signature-256`: `sha256=<hex>`).
pub struct GitHubWebhook {
    /// Generates a `"sha256=<lowercase_hex>"` signature header. Caller owns the returned slice.
    pub static fn signSha256(
        alloc: std.mem.Allocator,
        secret: []const u8,
        payload: []const u8,
    ): ([]u8) ! WebhookError {
        let mac: [32]u8 = undefined;
        std.crypto.auth.hmac.sha2.HmacSha256.create(&mac, payload, secret);

        let out = alloc.alloc(u8, 7 + 64) catch return WebhookError.OutOfMemory;
        @memcpy(out[0..7], "sha256=");
        encodeHex32(&mac, out[7..71]);
        return out;
    }

    /// Verifies a `"sha256=<hex>"` header in constant time against `HMAC-SHA256(secret, payload)`.
    pub static fn verifySha256(
        secret: []const u8,
        payload: []const u8,
        header: []const u8,
    ): bool {
        if (header.len != 71) return false;
        if (!std.mem.eql(u8, header[0..7], "sha256=")) return false;

        let received_mac: [32]u8 = undefined;
        if (!decodeHex32(header[7..71], &received_mac)) return false;

        let expected_mac: [32]u8 = undefined;
        std.crypto.auth.hmac.sha2.HmacSha256.create(&expected_mac, payload, secret);
        return std.crypto.timing_safe.eql([32]u8, expected_mac, received_mac);
    }
}

/// Stripe Webhooks (`Stripe-Signature`: `t=<unix_sec>,v1=<hex>[,v1=<hex>...]`).
pub struct StripeWebhook {
    /// Generates a `"t=<ts_sec>,v1=<lowercase_hex>"` header. Caller owns the returned slice.
    pub static fn signHeader(
        alloc: std.mem.Allocator,
        secret: []const u8,
        ts_sec: i64,
        payload: []const u8,
    ): ([]u8) ! WebhookError {
        let ts_buf: [32]u8 = undefined;
        const ts_str = std.fmt.bufPrint(&ts_buf, "{d}", .{ts_sec}) catch return WebhookError.MalformedHeader;

        let h = std.crypto.auth.hmac.sha2.HmacSha256.init(secret);
        h.update(ts_str);
        h.update(".");
        h.update(payload);
        let mac: [32]u8 = undefined;
        h.final(&mac);

        const total_len = 2 + ts_str.len + 4 + 64;
        let out = alloc.alloc(u8, total_len) catch return WebhookError.OutOfMemory;
        out[0] = 't';
        out[1] = '=';
        @memcpy(out[2 .. 2 + ts_str.len], ts_str);
        const v1_off = 2 + ts_str.len;
        @memcpy(out[v1_off .. v1_off + 4], ",v1=");
        encodeHex32(&mac, out[v1_off + 4 .. total_len]);
        return out;
    }

    /// Verifies a Stripe signature header (`t=...,v1=...`), supporting multiple rotated `v1=` keys,
    /// constant-time HMAC-SHA256 verification, and replay-window enforcement.
    pub static fn verifyHeader(
        secret: []const u8,
        payload: []const u8,
        header: []const u8,
        now_sec: i64,
        tolerance_sec: i64,
    ): void ! WebhookError {
        let ts_opt: i64? = null;
        let v1_count: usize = 0;

        let scan_pos: usize = 0;
        while (scan_pos <= header.len) {
            const rel_comma = std.mem.indexOfScalarPos(u8, header, scan_pos, ',');
            const part_end = rel_comma ?? header.len;
            const part = trimAsciiSpaces(header[scan_pos..part_end]);
            if (part.len == 0) {
                return WebhookError.MalformedHeader;
            }
            const eq_opt = std.mem.indexOfScalar(u8, part, '=');
            if (eq_opt == null) {
                return WebhookError.MalformedHeader;
            }
            const eq_idx = eq_opt.?;
            const key = trimAsciiSpaces(part[0..eq_idx]);
            const val = trimAsciiSpaces(part[eq_idx + 1 ..]);
            if (key.len == 0 || val.len == 0) {
                return WebhookError.MalformedHeader;
            }

            if (std.mem.eql(u8, key, "t")) {
                if (ts_opt != null) return WebhookError.MalformedHeader;
                if (parsed_ts := parseStrictI64(val)) {
                    ts_opt = parsed_ts;
                } else {
                    return WebhookError.MalformedHeader;
                }
            } else if (std.mem.eql(u8, key, "v1")) {
                v1_count += 1;
            }

            if (rel_comma == null) break;
            scan_pos = part_end + 1;
        }

        if (ts_opt == null) {
            return WebhookError.MalformedHeader;
        }
        const ts_sec = ts_opt.?;
        if (v1_count == 0) {
            return WebhookError.MalformedHeader;
        }

        checkTimestampWindow(ts_sec, now_sec, tolerance_sec)?;

        let ts_buf: [32]u8 = undefined;
        const ts_str = std.fmt.bufPrint(&ts_buf, "{d}", .{ts_sec}) catch return WebhookError.MalformedHeader;

        let h = std.crypto.auth.hmac.sha2.HmacSha256.init(secret);
        h.update(ts_str);
        h.update(".");
        h.update(payload);
        let expected_mac: [32]u8 = undefined;
        h.final(&expected_mac);

        let matched = false;
        let pos: usize = 0;
        while (pos <= header.len) {
            const rel_comma = std.mem.indexOfScalarPos(u8, header, pos, ',');
            const part_end = rel_comma ?? header.len;
            const part = trimAsciiSpaces(header[pos..part_end]);
            if (eq_idx := std.mem.indexOfScalar(u8, part, '=')) {
                const key = trimAsciiSpaces(part[0..eq_idx]);
                const val = trimAsciiSpaces(part[eq_idx + 1 ..]);

                if (std.mem.eql(u8, key, "v1")) {
                    let received_mac: [32]u8 = undefined;
                    if (decodeHex32(val, &received_mac)) {
                        if (std.crypto.timing_safe.eql([32]u8, expected_mac, received_mac)) {
                            matched = true;
                        }
                    }
                }
            }

            if (rel_comma == null) break;
            pos = part_end + 1;
        }

        if (!matched) {
            return WebhookError.InvalidSignature;
        }
    }
}
