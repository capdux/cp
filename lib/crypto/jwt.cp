// Source: Custom implementation by Antigravity AI (RFC 7519 JWT & RFC 7468 PEM)
// lib/crypto/jwt.cp
// RFC 7519 JSON Web Token (HS256/HS384/HS512/EdDSA) & RFC 7468 PEM Codec

import { Ed25519KeyPair } from "./asymmetric.cp";

pub fn constantTimeEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    for (i of 0..a.len) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

pub fn base64UrlEncode(data: []const u8): string {
    const encoder = std.base64.url_safe_no_pad.Encoder;
    const enc_len = encoder.calcSize(data.len);
    const out = std.heap.page_allocator.alloc(u8, enc_len) catch unreachable;
    * = encoder.encode(out, data);
    return out;
}

pub fn base64UrlDecode(encoded: string): ([]u8)? {
    const decoder = std.base64.url_safe_no_pad.Decoder;
    const dec_len = decoder.calcSizeForSlice(encoded) catch return null;
    const out = std.heap.page_allocator.alloc(u8, dec_len) catch return null;
    decoder.decode(out, encoded) catch {
        std.heap.page_allocator.free(out);
        return null;
    };
    return out;
}

pub struct JwtParts {
    header_json: string;
    payload_json: string;
    alg: string;
}

fn concatThreeWithDots(a: string, b: string, c: string): string {
    const total = a.len + 1 + b.len + 1 + c.len;
    let out = std.heap.page_allocator.alloc(u8, total) catch unreachable;
    @memcpy(out[0..a.len], a);
    out[a.len] = '.';
    @memcpy(out[a.len + 1 .. a.len + 1 + b.len], b);
    out[a.len + 1 + b.len] = '.';
    @memcpy(out[a.len + 1 + b.len + 1 .. total], c);
    return out;
}

fn buildSigningInput(header_json: string, payload_json: string): string {
    const h_b64 = base64UrlEncode(header_json);
    const p_b64 = base64UrlEncode(payload_json);
    const total = h_b64.len + 1 + p_b64.len;
    let out = std.heap.page_allocator.alloc(u8, total) catch unreachable;
    @memcpy(out[0..h_b64.len], h_b64);
    out[h_b64.len] = '.';
    @memcpy(out[h_b64.len + 1 .. total], p_b64);
    return out;
}

struct SplitJwtParts {
    header: string;
    payload: string;
    sig: string;
}

fn splitJwtToken(token: string): SplitJwtParts? {
    const first_dot = std.mem.indexOfScalar(u8, token, '.') ?? return null;
    if (first_dot == 0) return null;
    const rest = token[first_dot + 1 ..];
    const second_rel = std.mem.indexOfScalar(u8, rest, '.') ?? return null;
    if (second_rel == 0) return null;
    const sig_part = rest[second_rel + 1 ..];
    if (sig_part.len == 0) return null;
    // Reject tokens with more than two '.' separators
    if (std.mem.indexOfScalar(u8, sig_part, '.') != null) return null;
    return SplitJwtParts{
        header: token[0..first_dot],
        payload: rest[0..second_rel],
        sig: sig_part,
    };
}

fn extractHeaderAlg(header_json: string): string? {
    const parsed = JSON.parse(std.heap.page_allocator, header_json) catch return null;
    if (!parsed.isObject()) return null;
    const alg_val = parsed.get("alg") ?? return null;
    const alg_str = alg_val.asString() ?? return null;
    if (alg_str.len == 0) return null;
    return alg_str;
}

pub struct Jwt {
    static fn signHs256(payload_json: string, secret: string): string {
        const signing_input = buildSigningInput("{\"alg\":\"HS256\",\"typ\":\"JWT\"}", payload_json);
        let mac: [32]u8 = undefined;
        std.crypto.auth.hmac.sha2.HmacSha256.create(&mac, signing_input, secret);
        const sig_b64 = base64UrlEncode(&mac);
        const total = signing_input.len + 1 + sig_b64.len;
        let out = std.heap.page_allocator.alloc(u8, total) catch unreachable;
        @memcpy(out[0..signing_input.len], signing_input);
        out[signing_input.len] = '.';
        @memcpy(out[signing_input.len + 1 .. total], sig_b64);
        return out;
    }

    static fn signHs384(payload_json: string, secret: string): string {
        const signing_input = buildSigningInput("{\"alg\":\"HS384\",\"typ\":\"JWT\"}", payload_json);
        let mac: [48]u8 = undefined;
        std.crypto.auth.hmac.sha2.HmacSha384.create(&mac, signing_input, secret);
        const sig_b64 = base64UrlEncode(&mac);
        const total = signing_input.len + 1 + sig_b64.len;
        let out = std.heap.page_allocator.alloc(u8, total) catch unreachable;
        @memcpy(out[0..signing_input.len], signing_input);
        out[signing_input.len] = '.';
        @memcpy(out[signing_input.len + 1 .. total], sig_b64);
        return out;
    }

    static fn signHs512(payload_json: string, secret: string): string {
        const signing_input = buildSigningInput("{\"alg\":\"HS512\",\"typ\":\"JWT\"}", payload_json);
        let mac: [64]u8 = undefined;
        std.crypto.auth.hmac.sha2.HmacSha512.create(&mac, signing_input, secret);
        const sig_b64 = base64UrlEncode(&mac);
        const total = signing_input.len + 1 + sig_b64.len;
        let out = std.heap.page_allocator.alloc(u8, total) catch unreachable;
        @memcpy(out[0..signing_input.len], signing_input);
        out[signing_input.len] = '.';
        @memcpy(out[signing_input.len + 1 .. total], sig_b64);
        return out;
    }

    static fn signEdDsa(payload_json: string, key_pair: Ed25519KeyPair): string {
        const signing_input = buildSigningInput("{\"alg\":\"EdDSA\",\"typ\":\"JWT\"}", payload_json);
        const sig = key_pair.sign(signing_input);
        const sig_b64 = base64UrlEncode(&sig);
        const total = signing_input.len + 1 + sig_b64.len;
        let out = std.heap.page_allocator.alloc(u8, total) catch unreachable;
        @memcpy(out[0..signing_input.len], signing_input);
        out[signing_input.len] = '.';
        @memcpy(out[signing_input.len + 1 .. total], sig_b64);
        return out;
    }

    static fn verifyHs256(token: string, secret: string): JwtParts? {
        const parts = splitJwtToken(token) ?? return null;
        const h_b64 = parts.header;
        const p_b64 = parts.payload;
        const s_b64 = parts.sig;
        const header_bytes = base64UrlDecode(h_b64) ?? return null;
        const alg = extractHeaderAlg(header_bytes) ?? return null;
        if (!std.mem.eql(u8, alg, "HS256")) return null;

        const sig_bytes = base64UrlDecode(s_b64) ?? return null;
        if (sig_bytes.len != 32) return null;

        const signing_input = token[0 .. h_b64.len + 1 + p_b64.len];
        let expected: [32]u8 = undefined;
        std.crypto.auth.hmac.sha2.HmacSha256.create(&expected, signing_input, secret);
        let received: [32]u8 = undefined;
        @memcpy(&received, sig_bytes[0..32]);
        if (!std.crypto.timing_safe.eql([32]u8, expected, received)) return null;

        const payload_bytes = base64UrlDecode(p_b64) ?? return null;
        return JwtParts{
            header_json: header_bytes,
            payload_json: payload_bytes,
            alg: alg,
        };
    }

    static fn verifyHs384(token: string, secret: string): JwtParts? {
        const parts = splitJwtToken(token) ?? return null;
        const h_b64 = parts.header;
        const p_b64 = parts.payload;
        const s_b64 = parts.sig;
        const header_bytes = base64UrlDecode(h_b64) ?? return null;
        const alg = extractHeaderAlg(header_bytes) ?? return null;
        if (!std.mem.eql(u8, alg, "HS384")) return null;

        const sig_bytes = base64UrlDecode(s_b64) ?? return null;
        if (sig_bytes.len != 48) return null;

        const signing_input = token[0 .. h_b64.len + 1 + p_b64.len];
        let expected: [48]u8 = undefined;
        std.crypto.auth.hmac.sha2.HmacSha384.create(&expected, signing_input, secret);
        let received: [48]u8 = undefined;
        @memcpy(&received, sig_bytes[0..48]);
        if (!std.crypto.timing_safe.eql([48]u8, expected, received)) return null;

        const payload_bytes = base64UrlDecode(p_b64) ?? return null;
        return JwtParts{
            header_json: header_bytes,
            payload_json: payload_bytes,
            alg: alg,
        };
    }

    static fn verifyHs512(token: string, secret: string): JwtParts? {
        const parts = splitJwtToken(token) ?? return null;
        const h_b64 = parts.header;
        const p_b64 = parts.payload;
        const s_b64 = parts.sig;
        const header_bytes = base64UrlDecode(h_b64) ?? return null;
        const alg = extractHeaderAlg(header_bytes) ?? return null;
        if (!std.mem.eql(u8, alg, "HS512")) return null;

        const sig_bytes = base64UrlDecode(s_b64) ?? return null;
        if (sig_bytes.len != 64) return null;

        const signing_input = token[0 .. h_b64.len + 1 + p_b64.len];
        let expected: [64]u8 = undefined;
        std.crypto.auth.hmac.sha2.HmacSha512.create(&expected, signing_input, secret);
        let received: [64]u8 = undefined;
        @memcpy(&received, sig_bytes[0..64]);
        if (!std.crypto.timing_safe.eql([64]u8, expected, received)) return null;

        const payload_bytes = base64UrlDecode(p_b64) ?? return null;
        return JwtParts{
            header_json: header_bytes,
            payload_json: payload_bytes,
            alg: alg,
        };
    }

    static fn verifyEdDsa(token: string, public_key: []const u8): JwtParts? {
        const parts = splitJwtToken(token) ?? return null;
        const h_b64 = parts.header;
        const p_b64 = parts.payload;
        const s_b64 = parts.sig;
        const header_bytes = base64UrlDecode(h_b64) ?? return null;
        const alg = extractHeaderAlg(header_bytes) ?? return null;
        if (!std.mem.eql(u8, alg, "EdDSA")) return null;

        const sig_bytes = base64UrlDecode(s_b64) ?? return null;
        const signing_input = token[0 .. h_b64.len + 1 + p_b64.len];
        if (!Ed25519KeyPair.verify(sig_bytes, signing_input, public_key)) return null;

        const payload_bytes = base64UrlDecode(p_b64) ?? return null;
        return JwtParts{
            header_json: header_bytes,
            payload_json: payload_bytes,
            alg: alg,
        };
    }

    static fn decodeUnverified(token: string): JwtParts? {
        const parts = splitJwtToken(token) ?? return null;
        const header_bytes = base64UrlDecode(parts.header) ?? return null;
        const payload_bytes = base64UrlDecode(parts.payload) ?? return null;
        const alg = extractHeaderAlg(header_bytes) ?? "";
        return JwtParts{
            header_json: header_bytes,
            payload_json: payload_bytes,
            alg: alg,
        };
    }
}

// =============================================================================
// RFC 7468 PEM Encoder & Decoder (64-column Base64 line wrapping)
// =============================================================================

pub struct PemBlock {
    label: string;
    data: []u8;
}

pub struct Pem {
    static fn encode(label: string, der_bytes: []const u8): string {
        const encoder = std.base64.standard.Encoder;
        const b64_len = encoder.calcSize(der_bytes.len);
        const b64_buf = std.heap.page_allocator.alloc(u8, b64_len) catch unreachable;
        defer std.heap.page_allocator.free(b64_buf);
        * = encoder.encode(b64_buf, der_bytes);

        const line_count: usize = if (b64_len == 0) 0 else (b64_len + 63) / 64;
        const header_prefix = "-----BEGIN ";
        const header_suffix = "-----\n";
        const footer_prefix = "-----END ";
        const footer_suffix = "-----\n";
        const total_len = header_prefix.len + label.len + header_suffix.len + b64_len + line_count + footer_prefix.len + label.len + footer_suffix.len;

        let out = std.heap.page_allocator.alloc(u8, total_len) catch unreachable;
        let pos: usize = 0;

        @memcpy(out[pos .. pos + header_prefix.len], header_prefix);
        pos += header_prefix.len;
        @memcpy(out[pos .. pos + label.len], label);
        pos += label.len;
        @memcpy(out[pos .. pos + header_suffix.len], header_suffix);
        pos += header_suffix.len;

        let offset: usize = 0;
        while (offset < b64_len) {
            const rem = b64_len - offset;
            const chunk_len: usize = if (rem > 64) 64 else rem;
            @memcpy(out[pos .. pos + chunk_len], b64_buf[offset .. offset + chunk_len]);
            pos += chunk_len;
            out[pos] = '\n';
            pos += 1;
            offset += chunk_len;
        }

        @memcpy(out[pos .. pos + footer_prefix.len], footer_prefix);
        pos += footer_prefix.len;
        @memcpy(out[pos .. pos + label.len], label);
        pos += label.len;
        @memcpy(out[pos .. pos + footer_suffix.len], footer_suffix);
        pos += footer_suffix.len;

        return out[0..pos];
    }

    static fn decode(pem_text: string): PemBlock? {
        const begin_prefix = "-----BEGIN ";
        const dashes = "-----";
        const end_prefix = "-----END ";

        const begin_idx = std.mem.indexOf(u8, pem_text, begin_prefix) ?? return null;
        const label_start = begin_idx + begin_prefix.len;
        const label_end_rel = std.mem.indexOf(u8, pem_text[label_start..], dashes) ?? return null;
        if (label_end_rel == 0) return null;
        const label = pem_text[label_start .. label_start + label_end_rel];
        if (std.mem.indexOfAny(u8, label, "\r\n") != null) return null;

        let body_start = label_start + label_end_rel + dashes.len;
        // Skip optional trailing spaces/CRLF on BEGIN line
        while (body_start < pem_text.len && (pem_text[body_start] == ' ' || pem_text[body_start] == '\t' || pem_text[body_start] == '\r' || pem_text[body_start] == '\n')) {
            body_start += 1;
        }

        const end_marker_search = pem_text[body_start..];
        const end_rel = std.mem.indexOf(u8, end_marker_search, end_prefix) ?? return null;
        const body_slice = end_marker_search[0..end_rel];

        const end_label_start = body_start + end_rel + end_prefix.len;
        if (end_label_start + label.len + dashes.len > pem_text.len) return null;
        const end_label = pem_text[end_label_start .. end_label_start + label.len];
        if (!std.mem.eql(u8, label, end_label)) return null;
        if (!std.mem.eql(u8, pem_text[end_label_start + label.len .. end_label_start + label.len + dashes.len], dashes)) return null;

        const decoder = std.base64.standard.decoderWithIgnore("\r\n \t");
        const max_len = decoder.calcSizeUpperBound(body_slice.len);
        const out_buf = std.heap.page_allocator.alloc(u8, max_len) catch return null;
        const actual_len = decoder.decode(out_buf, body_slice) catch {
            std.heap.page_allocator.free(out_buf);
            return null;
        };

        const label_copy = std.heap.page_allocator.alloc(u8, label.len) catch {
            std.heap.page_allocator.free(out_buf);
            return null;
        };
        @memcpy(label_copy, label);

        return PemBlock{
            label: label_copy,
            data: out_buf[0..actual_len],
        };
    }
}
