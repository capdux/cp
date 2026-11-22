// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T13:15:07Z (RFC 7519 JWT Structure & Claims Inspector)
// Pure-CP zero-allocation JWT segment splitter, Base64URL decoder, and RFC 7519 claims validator.

fn b64UrlVal(c: u8): u8? {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return null;
}

/// Decode unpadded (or padded) Base64URL string into `out`. Returns decoded slice or null on error.
export fn decodeBase64Url(input: []const u8, out: []u8): ([]const u8)? {
    // Strip optional trailing '=' padding
    let len = input.len;
    while (len > 0 && input[len - 1] == '=') {
        len -= 1;
    }
    const rem = len % 4;
    if (rem == 1) return null;

    const full_groups = len / 4;
    let out_len = full_groups * 3;
    if (rem == 2) out_len += 1;
    if (rem == 3) out_len += 2;
    if (out_len > out.len) return null;

    let in_i: usize = 0;
    let out_i: usize = 0;
    while (in_i + 4 <= len) {
        const a = b64UrlVal(input[in_i]) ?? return null;
        const b = b64UrlVal(input[in_i + 1]) ?? return null;
        const c = b64UrlVal(input[in_i + 2]) ?? return null;
        const d = b64UrlVal(input[in_i + 3]) ?? return null;
        out[out_i] = (a << 2) | (b >> 4);
        out[out_i + 1] = ((b & 0x0F) << 4) | (c >> 2);
        out[out_i + 2] = ((c & 0x03) << 6) | d;
        in_i += 4;
        out_i += 3;
    }

    if (rem == 2) {
        const a = b64UrlVal(input[in_i]) ?? return null;
        const b = b64UrlVal(input[in_i + 1]) ?? return null;
        out[out_i] = (a << 2) | (b >> 4);
        out_i += 1;
    } else if (rem == 3) {
        const a = b64UrlVal(input[in_i]) ?? return null;
        const b = b64UrlVal(input[in_i + 1]) ?? return null;
        const c = b64UrlVal(input[in_i + 2]) ?? return null;
        out[out_i] = (a << 2) | (b >> 4);
        out[out_i + 1] = ((b & 0x0F) << 4) | (c >> 2);
        out_i += 2;
    }

    return out[0..out_i];
}

export struct JwtParts {
    header_b64: []const u8,
    payload_b64: []const u8,
    signature_b64: []const u8,
    signing_input: []const u8,
}

/// Split a compact JWS/JWT `header.payload.signature` into its 3 Base64URL segments.
export fn splitJwt(token: []const u8): JwtParts? {
    let dot1: usize? = null;
    let dot2: usize? = null;
    let i: usize = 0;
    while (i < token.len) {
        if (token[i] == '.') {
            if (dot1 == null) {
                dot1 = i;
            } else if (dot2 == null) {
                dot2 = i;
            } else {
                return null; // >2 dots invalid for compact JWS
            }
        }
        i += 1;
    }
    const d1 = dot1 ?? return null;
    const d2 = dot2 ?? return null;
    if (d1 == 0 || d2 == d1 + 1) return null;
    return JwtParts {
        header_b64: token[0..d1],
        payload_b64: token[d1 + 1..d2],
        signature_b64: token[d2 + 1..],
        signing_input: token[0..d2],
    };
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// Skip a JSON value at `pos` (handles nested objects, arrays, strings, numbers, literals).
fn skipJsonValue(json: []const u8, pos: *usize): bool {
    while (pos.* < json.len && isWs(json[pos.*])) {
        pos.* += 1;
    }
    if (pos.* >= json.len) return false;
    const c = json[pos.*];
    if (c == '"') {
        pos.* += 1;
        while (pos.* < json.len) {
            const ch = json[pos.*];
            pos.* += 1;
            if (ch == '\\') {
                if (pos.* >= json.len) return false;
                pos.* += 1;
            } else if (ch == '"') {
                return true;
            }
        }
        return false;
    }
    if (c == '{' || c == '[') {
        let depth: usize = 0;
        let in_str = false;
        while (pos.* < json.len) {
            const ch = json[pos.*];
            pos.* += 1;
            if (in_str) {
                if (ch == '\\') {
                    if (pos.* >= json.len) return false;
                    pos.* += 1;
                } else if (ch == '"') {
                    in_str = false;
                }
            } else {
                if (ch == '"') {
                    in_str = true;
                } else if (ch == '{' || ch == '[') {
                    depth += 1;
                } else if (ch == '}' || ch == ']') {
                    if (depth == 0) return false;
                    depth -= 1;
                    if (depth == 0) return true;
                }
            }
        }
        return false;
    }
    while (pos.* < json.len && json[pos.*] != ',' && json[pos.*] != '}' && json[pos.*] != ']' && !isWs(json[pos.*])) {
        pos.* += 1;
    }
    return true;
}

/// Extract a top-level string claim value slice (raw JSON string contents) from a decoded JSON object.
export fn extractStringClaim(json: []const u8, key: []const u8): ([]const u8)? {
    let pos: usize = 0;
    while (pos < json.len && isWs(json[pos])) pos += 1;
    if (pos >= json.len || json[pos] != '{') return null;
    pos += 1;

    while (pos < json.len) {
        while (pos < json.len && (isWs(json[pos]) || json[pos] == ',')) pos += 1;
        if (pos >= json.len || json[pos] == '}') return null;
        if (json[pos] != '"') return null;
        pos += 1;
        const kstart = pos;
        while (pos < json.len && json[pos] != '"') {
            if (json[pos] == '\\') pos += 1;
            pos += 1;
        }
        if (pos >= json.len) return null;
        const k = json[kstart..pos];
        pos += 1;

        while (pos < json.len && isWs(json[pos])) pos += 1;
        if (pos >= json.len || json[pos] != ':') return null;
        pos += 1;
        while (pos < json.len && isWs(json[pos])) pos += 1;

        if (strEq(k, key)) {
            if (pos >= json.len || json[pos] != '"') return null;
            pos += 1;
            const vstart = pos;
            while (pos < json.len && json[pos] != '"') {
                if (json[pos] == '\\') pos += 1;
                pos += 1;
            }
            if (pos >= json.len) return null;
            return json[vstart..pos];
        }
        if (!skipJsonValue(json, &pos)) return null;
    }
    return null;
}

/// Extract a top-level integer claim (e.g. `exp`, `nbf`, `iat`) as `i64` from a decoded JSON object.
export fn extractIntClaim(json: []const u8, key: []const u8): i64? {
    let pos: usize = 0;
    while (pos < json.len && isWs(json[pos])) pos += 1;
    if (pos >= json.len || json[pos] != '{') return null;
    pos += 1;

    while (pos < json.len) {
        while (pos < json.len && (isWs(json[pos]) || json[pos] == ',')) pos += 1;
        if (pos >= json.len || json[pos] == '}') return null;
        if (json[pos] != '"') return null;
        pos += 1;
        const kstart = pos;
        while (pos < json.len && json[pos] != '"') {
            if (json[pos] == '\\') pos += 1;
            pos += 1;
        }
        if (pos >= json.len) return null;
        const k = json[kstart..pos];
        pos += 1;

        while (pos < json.len && isWs(json[pos])) pos += 1;
        if (pos >= json.len || json[pos] != ':') return null;
        pos += 1;
        while (pos < json.len && isWs(json[pos])) pos += 1;

        if (strEq(k, key)) {
            if (pos >= json.len) return null;
            let neg = false;
            if (json[pos] == '-') {
                neg = true;
                pos += 1;
            }
            const dstart = pos;
            let val: i64 = 0;
            while (pos < json.len && json[pos] >= '0' && json[pos] <= '9') {
                const digit: i64 = @as(i64, json[pos] - '0');
                if (val > 922337203685477580) return null;
                val = val * 10 + digit;
                pos += 1;
            }
            if (pos == dstart) return null;
            if (neg) return 0 - val;
            return val;
        }
        if (!skipJsonValue(json, &pos)) return null;
    }
    return null;
}

export struct JwtHeader {
    alg: ([]const u8)?,
    typ: ([]const u8)?,
    kid: ([]const u8)?,

    pub fn parseJson(header_json: []const u8): JwtHeader {
        return JwtHeader {
            alg: extractStringClaim(header_json, "alg"),
            typ: extractStringClaim(header_json, "typ"),
            kid: extractStringClaim(header_json, "kid"),
        };
    }

    pub fn isNoneAlg(): bool {
        if (a := _.alg) {
            if (a.len == 4) {
                const c0 = if (a[0] >= 'A' && a[0] <= 'Z') a[0] + 32 else a[0];
                const c1 = if (a[1] >= 'A' && a[1] <= 'Z') a[1] + 32 else a[1];
                const c2 = if (a[2] >= 'A' && a[2] <= 'Z') a[2] + 32 else a[2];
                const c3 = if (a[3] >= 'A' && a[3] <= 'Z') a[3] + 32 else a[3];
                return c0 == 'n' && c1 == 'o' && c2 == 'n' && c3 == 'e';
            }
        }
        return false;
    }
}

export enum ClaimStatus {
    valid,
    expired,
    not_yet_valid,
    issuer_mismatch,
    subject_mismatch,
    audience_mismatch,
}

export struct JwtClaims {
    iss: ([]const u8)?,
    sub: ([]const u8)?,
    aud: ([]const u8)?,
    jti: ([]const u8)?,
    exp: i64?,
    nbf: i64?,
    iat: i64?,

    pub fn parseJson(payload_json: []const u8): JwtClaims {
        return JwtClaims {
            iss: extractStringClaim(payload_json, "iss"),
            sub: extractStringClaim(payload_json, "sub"),
            aud: extractStringClaim(payload_json, "aud"),
            jti: extractStringClaim(payload_json, "jti"),
            exp: extractIntClaim(payload_json, "exp"),
            nbf: extractIntClaim(payload_json, "nbf"),
            iat: extractIntClaim(payload_json, "iat"),
        };
    }

    /// Validate `exp` and `nbf` against `now_unix_sec` with `leeway_sec` clock skew tolerance.
    pub fn validateTime(now_unix_sec: i64, leeway_sec: i64): ClaimStatus {
        if (e := _.exp) {
            if (now_unix_sec >= e + leeway_sec) return ClaimStatus.expired;
        }
        if (n := _.nbf) {
            if (now_unix_sec + leeway_sec < n) return ClaimStatus.not_yet_valid;
        }
        return ClaimStatus.valid;
    }

    /// Validate time window plus optional expected `iss`, `sub`, `aud`.
    pub fn validate(
        now_unix_sec: i64,
        leeway_sec: i64,
        expected_iss: ([]const u8)?,
        expected_sub: ([]const u8)?,
        expected_aud: ([]const u8)?,
    ): ClaimStatus {
        const ts = _.validateTime(now_unix_sec, leeway_sec);
        if (ts != ClaimStatus.valid) return ts;
        if (want_iss := expected_iss) {
            const got = _.iss ?? return ClaimStatus.issuer_mismatch;
            if (!strEq(got, want_iss)) return ClaimStatus.issuer_mismatch;
        }
        if (want_sub := expected_sub) {
            const got = _.sub ?? return ClaimStatus.subject_mismatch;
            if (!strEq(got, want_sub)) return ClaimStatus.subject_mismatch;
        }
        if (want_aud := expected_aud) {
            const got = _.aud ?? return ClaimStatus.audience_mismatch;
            if (!strEq(got, want_aud)) return ClaimStatus.audience_mismatch;
        }
        return ClaimStatus.valid;
    }
}
