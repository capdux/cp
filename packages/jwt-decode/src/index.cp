// @cap/jwt-parse — Zero-allocation RFC 7519 JWT Splitter, Base64URL Decoder & Claim Guard
// Clean-room MIT implementation in pure CP.
// Accelerated with comptime 256-entry Base64URL LUT and 16-lane @Vector(16, u8) SIMD token scanning.

import { eqIgnoreCase } from "cap:ascii";
import { indexOfBytes } from "cap:bin";

type JwtVec16 = @Vector(16, u8);

fn jwtBuildB64UrlLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let tbl: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        tbl[i] = 0xFF as u8;
        i += 1;
    }
    let k: usize = 0;
    while (k < 26) {
        tbl[65 + k] = k as u8;        // A-Z -> 0..25
        tbl[97 + k] = (26 + k) as u8; // a-z -> 26..51
        k += 1;
    }
    let d: usize = 0;
    while (d < 10) {
        tbl[48 + d] = (52 + d) as u8; // 0-9 -> 52..61
        d += 1;
    }
    tbl[45] = 62 as u8; // '-'
    tbl[95] = 63 as u8; // '_'
    return tbl;
}

const JWT_B64U_LUT: [256]u8 = jwtBuildB64UrlLut();

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

fn b64UrlVal(b: u8) -> u8 {
    return JWT_B64U_LUT[b as usize];
}

/// Splits an RFC 7519 compact JWS/JWT into its 3 dot-separated Base64URL segments
/// plus the zero-copy `signing_input` (`header.payload`).
/// Requires non-empty header and payload and strictly valid Base64URL characters (`A-Za-z0-9-_`).
pub fn splitToken(token: string) -> JwtParts {
    const n = token.len;
    if (n < 3) {
        return invalidParts();
    }

    const v_dot: JwtVec16 = @splat(46 as u8);
    const v_A: JwtVec16 = @splat(65 as u8);
    const v_Z: JwtVec16 = @splat(90 as u8);
    const v_a: JwtVec16 = @splat(97 as u8);
    const v_z: JwtVec16 = @splat(122 as u8);
    const v_0: JwtVec16 = @splat(48 as u8);
    const v_9: JwtVec16 = @splat(57 as u8);
    const v_dash: JwtVec16 = @splat(45 as u8);
    const v_under: JwtVec16 = @splat(95 as u8);
    const v_one: JwtVec16 = @splat(1 as u8);
    const v_zero: JwtVec16 = @splat(0 as u8);

    let dot1: usize = n;
    let dot2: usize = n;
    let i: usize = 0;

    while (i + 16 <= n) {
        const v: JwtVec16 = token[i .. i + 16][0..16].*;
        const has_dot = @reduce(.Or, v == v_dot);
        if (!has_dot) {
            const is_up = @select(u8, v >= v_A, v_one, v_zero) & @select(u8, v <= v_Z, v_one, v_zero);
            const is_lo = @select(u8, v >= v_a, v_one, v_zero) & @select(u8, v <= v_z, v_one, v_zero);
            const is_dg = @select(u8, v >= v_0, v_one, v_zero) & @select(u8, v <= v_9, v_one, v_zero);
            const is_sp = @select(u8, v == v_dash, v_one, v_zero) | @select(u8, v == v_under, v_one, v_zero);
            const valid_v = is_up | is_lo | is_dg | is_sp;
            if (@reduce(.Min, valid_v) == (0 as u8)) {
                return invalidParts();
            }
            i += 16;
            continue;
        }
        const chunk_end = i + 16;
        while (i < chunk_end) {
            const ch = token[i];
            if (ch == 46) {
                if (dot1 == n) {
                    dot1 = i;
                } else if (dot2 == n) {
                    dot2 = i;
                } else {
                    return invalidParts();
                }
            } else if (b64UrlVal(ch) == (0xFF as u8)) {
                return invalidParts();
            }
            i += 1;
        }
    }

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
        } else if (b64UrlVal(ch) == (0xFF as u8)) {
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

    // 16-byte -> 12-byte unrolled LUT fast path with single combined validity check
    while (i + 16 <= full_end) {
        const a0 = JWT_B64U_LUT[input[i] as usize];
        const a1 = JWT_B64U_LUT[input[i + 1] as usize];
        const a2 = JWT_B64U_LUT[input[i + 2] as usize];
        const a3 = JWT_B64U_LUT[input[i + 3] as usize];
        const b0 = JWT_B64U_LUT[input[i + 4] as usize];
        const b1 = JWT_B64U_LUT[input[i + 5] as usize];
        const b2 = JWT_B64U_LUT[input[i + 6] as usize];
        const b3 = JWT_B64U_LUT[input[i + 7] as usize];
        const c0 = JWT_B64U_LUT[input[i + 8] as usize];
        const c1 = JWT_B64U_LUT[input[i + 9] as usize];
        const c2 = JWT_B64U_LUT[input[i + 10] as usize];
        const c3 = JWT_B64U_LUT[input[i + 11] as usize];
        const d0 = JWT_B64U_LUT[input[i + 12] as usize];
        const d1 = JWT_B64U_LUT[input[i + 13] as usize];
        const d2 = JWT_B64U_LUT[input[i + 14] as usize];
        const d3 = JWT_B64U_LUT[input[i + 15] as usize];

        if (((a0 | a1 | a2 | a3 | b0 | b1 | b2 | b3 | c0 | c1 | c2 | c3 | d0 | d1 | d2 | d3) & (0x80 as u8)) != (0 as u8)) {
            return -1;
        }

        const t0: u32 = ((a0 as u32) << 18) | ((a1 as u32) << 12) | ((a2 as u32) << 6) | (a3 as u32);
        const t1: u32 = ((b0 as u32) << 18) | ((b1 as u32) << 12) | ((b2 as u32) << 6) | (b3 as u32);
        const t2: u32 = ((c0 as u32) << 18) | ((c1 as u32) << 12) | ((c2 as u32) << 6) | (c3 as u32);
        const t3: u32 = ((d0 as u32) << 18) | ((d1 as u32) << 12) | ((d2 as u32) << 6) | (d3 as u32);

        let block: [12]u8 = undefined;
        block[0] = ((t0 >> 16) & 0xFF) as u8;
        block[1] = ((t0 >> 8) & 0xFF) as u8;
        block[2] = (t0 & 0xFF) as u8;
        block[3] = ((t1 >> 16) & 0xFF) as u8;
        block[4] = ((t1 >> 8) & 0xFF) as u8;
        block[5] = (t1 & 0xFF) as u8;
        block[6] = ((t2 >> 16) & 0xFF) as u8;
        block[7] = ((t2 >> 8) & 0xFF) as u8;
        block[8] = (t2 & 0xFF) as u8;
        block[9] = ((t3 >> 16) & 0xFF) as u8;
        block[10] = ((t3 >> 8) & 0xFF) as u8;
        block[11] = (t3 & 0xFF) as u8;
        @memcpy(out[w .. w + 12], block[0..12]);
        w += 12;
        i += 16;
    }

    while (i < full_end) {
        const v0 = JWT_B64U_LUT[input[i] as usize];
        const v1 = JWT_B64U_LUT[input[i + 1] as usize];
        const v2 = JWT_B64U_LUT[input[i + 2] as usize];
        const v3 = JWT_B64U_LUT[input[i + 3] as usize];
        if (((v0 | v1 | v2 | v3) & (0x80 as u8)) != (0 as u8)) {
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
        const v0 = JWT_B64U_LUT[input[i] as usize];
        const v1 = JWT_B64U_LUT[input[i + 1] as usize];
        if (((v0 | v1) & (0x80 as u8)) != (0 as u8)) {
            return -1;
        }
        out[w] = (((v0 as u32) << 2) | ((v1 as u32) >> 4)) as u8;
        w += 1;
    } else if (rem == 3) {
        const v0 = JWT_B64U_LUT[input[i] as usize];
        const v1 = JWT_B64U_LUT[input[i + 1] as usize];
        const v2 = JWT_B64U_LUT[input[i + 2] as usize];
        if (((v0 | v1 | v2) & (0x80 as u8)) != (0 as u8)) {
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

/// Detects unsecured `"alg":"none"` (case-insensitive) in a decoded JWT header JSON object.
pub fn isAlgNone(header_json: string) -> bool {
    const n = header_json.len;
    let i: usize = 0;
    while (i + 5 < n) {
        const found = indexOfBytes(header_json, "\"alg\"", i);
        if (found == null) {
            return false;
        }
        const pos = found.?;
        let p = pos + 5;
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
                if (eqIgnoreCase(header_json[(p + 1)..(p + 5)], "none")) {
                    return true;
                }
            }
        }
        i = pos + 1;
    }
    return false;
}
