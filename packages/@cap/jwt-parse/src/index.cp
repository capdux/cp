// @cap/jwt-parse — Zero-allocation RFC 7519 JWT Splitter, Base64URL Decoder & Claim Guard
// Clean-room MIT implementation in pure CP.

pub struct JwtParts {
    pub header_b64: string,
    pub payload_b64: string,
    pub signature_b64: string,
    pub signing_input: string,
    pub valid: bool,
}

fn invalidParts() -> JwtParts {
    return JwtParts {
        header_b64: "",
        payload_b64: "",
        signature_b64: "",
        signing_input: "",
        valid: false,
    };
}

fn b64UrlVal(b: u8) -> i32 {
    if (b >= 65 && b <= 90) { return (b - 65) as i32; }       // A-Z -> 0..25
    if (b >= 97 && b <= 122) { return (b - 97 + 26) as i32; } // a-z -> 26..51
    if (b >= 48 && b <= 57) { return (b - 48 + 52) as i32; }  // 0-9 -> 52..61
    if (b == 45) { return 62; }                               // '-'
    if (b == 95) { return 63; }                               // '_'
    return -1;
}

/// Splits an RFC 7519 compact JWS/JWT into its 3 dot-separated Base64URL segments
/// plus the zero-copy `signing_input` (`header.payload`).
/// Requires non-empty header and payload and strictly valid Base64URL characters (`A-Za-z0-9-_`).
pub fn splitToken(token: string) -> JwtParts {
    const n = token.len;
    if (n < 3) {
        return invalidParts();
    }

    let dot1: usize = n;
    let dot2: usize = n;
    let i: usize = 0;
    while (i < n) {
        const ch = token[i];
        if (ch == 46) {
            if (dot1 == n) {
                dot1 = i;
            } else if (dot2 == n) {
                dot2 = i;
            } else {
                // More than 2 dots -> invalid compact JWS
                return invalidParts();
            }
        } else if (b64UrlVal(ch) < 0) {
            return invalidParts();
        }
        i += 1;
    }

    if (dot1 == n || dot2 == n || dot1 == 0 || dot2 <= dot1 + 1) {
        return invalidParts();
    }

    return JwtParts {
        header_b64: token[0..dot1],
        payload_b64: token[dot1 + 1..dot2],
        signature_b64: token[dot2 + 1..n],
        signing_input: token[0..dot2],
        valid: true,
    };
}

/// Decodes an unpadded RFC 4648 Base64URL string into `out`.
/// Returns bytes written, or `-1` on invalid length, illegal character, or insufficient buffer.
pub fn decodeBase64Url(input: string, out: []u8) -> i64 {
    const n = input.len;
    if (n == 0) {
        return 0;
    }
    const rem = n % 4;
    if (rem == 1) {
        return -1;
    }

    let extra: usize = 0;
    if (rem == 2) {
        extra = 1;
    } else if (rem == 3) {
        extra = 2;
    }
    const needed = (n / 4) * 3 + extra;
    if (needed > out.len) {
        return -1;
    }

    let i: usize = 0;
    let w: usize = 0;
    const full_end = n - rem;

    while (i < full_end) {
        const v0 = b64UrlVal(input[i]);
        const v1 = b64UrlVal(input[i + 1]);
        const v2 = b64UrlVal(input[i + 2]);
        const v3 = b64UrlVal(input[i + 3]);
        if (v0 < 0 || v1 < 0 || v2 < 0 || v3 < 0) {
            return -1;
        }
        const triple: u32 = ((v0 as u32) << 18) | ((v1 as u32) << 12) | ((v2 as u32) << 6) | (v3 as u32);
        out[w] = ((triple >> 16) & 0xFF) as u8;
        out[w + 1] = ((triple >> 8) & 0xFF) as u8;
        out[w + 2] = (triple & 0xFF) as u8;
        w += 3;
        i += 4;
    }

    if (rem == 2) {
        const v0 = b64UrlVal(input[i]);
        const v1 = b64UrlVal(input[i + 1]);
        if (v0 < 0 || v1 < 0) {
            return -1;
        }
        out[w] = (((v0 as u32) << 2) | ((v1 as u32) >> 4)) as u8;
        w += 1;
    } else if (rem == 3) {
        const v0 = b64UrlVal(input[i]);
        const v1 = b64UrlVal(input[i + 1]);
        const v2 = b64UrlVal(input[i + 2]);
        if (v0 < 0 || v1 < 0 || v2 < 0) {
            return -1;
        }
        out[w] = (((v0 as u32) << 2) | ((v1 as u32) >> 4)) as u8;
        out[w + 1] = ((((v1 as u32) & 0x0F) << 4) | ((v2 as u32) >> 2)) as u8;
        w += 2;
    }

    return w as i64;
}

/// Decodes the JWT Header JSON segment into `out`. Returns bytes written or `-1` on error.
pub fn decodeHeader(token: string, out: []u8) -> i64 {
    const parts = splitToken(token);
    if (!parts.valid) {
        return -1;
    }
    return decodeBase64Url(parts.header_b64, out);
}

/// Decodes the JWT Payload JSON segment into `out`. Returns bytes written or `-1` on error.
pub fn decodePayload(token: string, out: []u8) -> i64 {
    const parts = splitToken(token);
    if (!parts.valid) {
        return -1;
    }
    return decodeBase64Url(parts.payload_b64, out);
}

/// Returns true if the token has expired (`now_unix >= exp_unix + leeway_secs`).
pub fn isExpired(exp_unix: i64, now_unix: i64, leeway_secs: i64) -> bool {
    let lw = leeway_secs;
    if (lw < 0) {
        lw = 0;
    }
    return now_unix >= exp_unix + lw;
}

/// Returns true if the token is not yet valid (`now_unix + leeway_secs < nbf_unix`).
pub fn isNotYetValid(nbf_unix: i64, now_unix: i64, leeway_secs: i64) -> bool {
    let lw = leeway_secs;
    if (lw < 0) {
        lw = 0;
    }
    return now_unix + lw < nbf_unix;
}

fn isJsonSpace(b: u8) -> bool {
    return b == 32 || b == 9 || b == 10 || b == 13;
}

fn toLowerAscii(b: u8) -> u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

/// Detects unsecured `"alg":"none"` (case-insensitive) in a decoded JWT header JSON object.
pub fn isAlgNone(header_json: string) -> bool {
    const n = header_json.len;
    let i: usize = 0;
    while (i + 5 < n) {
        // Look for literal key `"alg"`
        if (header_json[i] == 34
            && header_json[i + 1] == 97
            && header_json[i + 2] == 108
            && header_json[i + 3] == 103
            && header_json[i + 4] == 34)
        {
            let p = i + 5;
            while (p < n && isJsonSpace(header_json[p])) {
                p += 1;
            }
            if (p < n && header_json[p] == 58) {
                // ':'
                p += 1;
                while (p < n && isJsonSpace(header_json[p])) {
                    p += 1;
                }
                if (p + 6 <= n && header_json[p] == 34 && header_json[p + 5] == 34) {
                    if (toLowerAscii(header_json[p + 1]) == 110
                        && toLowerAscii(header_json[p + 2]) == 111
                        && toLowerAscii(header_json[p + 3]) == 110
                        && toLowerAscii(header_json[p + 4]) == 101)
                    {
                        return true;
                    }
                }
            }
        }
        i += 1;
    }
    return false;
}
