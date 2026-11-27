// @cap/base64-url — Zero-heap RFC 4648 §5 Base64URL encoder, decoder, converter & validator.
// Clean-room MIT port of npm `base64url` v3; reuses `@cap/data-encoding` and `cap:ascii`.

import {
    base64Encode, base64Decode, base64EncodeLen, DecodeResult
} from "../../data-encoding/src/index.cp";
import { isAlnum } from "cap:ascii";

/// Encodes `src` bytes into unpadded Base64URL (`A-Za-z0-9-_`) into `out`.
pub fn encodeBase64Url(src: []const u8, out: []u8): usize {
    return base64Encode(src, true, false, out);
}

/// Upstream alias: encodes `src` bytes into unpadded Base64URL (`A-Za-z0-9-_`) into `out`.
pub fn encode(src: []const u8, out: []u8): usize {
    return base64Encode(src, true, false, out);
}

/// Encodes `src` bytes into padded Base64URL (`A-Za-z0-9-_` + `=`) into `out`.
pub fn encodePadded(src: []const u8, out: []u8): usize {
    return base64Encode(src, true, true, out);
}

/// Decodes unpadded (or padded) Base64URL into `out`, returning `DecodeResult`.
pub fn decodeBase64Url(src: []const u8, out: []u8): DecodeResult {
    return base64Decode(src, true, out);
}

/// Upstream alias: decodes unpadded (or padded) Base64URL into `out`.
pub fn decode(src: []const u8, out: []u8): DecodeResult {
    return base64Decode(src, true, out);
}

/// Returns the exact output byte length for unpadded Base64URL encoding of `input_len` bytes.
pub fn encodedBase64UrlLen(input_len: usize): usize {
    return base64EncodeLen(input_len, false);
}

/// Upstream alias: exact unpadded Base64URL encoded length for `input_len` raw bytes.
pub fn encodedLen(input_len: usize): usize {
    return base64EncodeLen(input_len, false);
}

/// Returns the exact output byte length for padded Base64URL encoding of `input_len` bytes.
pub fn encodedPaddedLen(input_len: usize): usize {
    return base64EncodeLen(input_len, true);
}

/// Computes the exact decoded byte length for a Base64URL string (unpadded or padded).
/// Returns `null` if the length or padding is malformed.
pub fn decodedLen(src: string): usize? {
    let n = src.len;
    if (n == 0) { return 0; }
    let pad: usize = 0;
    if (src[n - 1] == 61) {
        pad = 1;
        if (n >= 2 && src[n - 2] == 61) {
            pad = 2;
        }
        // Padded Base64URL must be a multiple of 4
        if ((n & 3) != 0) { return null; }
        // Reject 3+ trailing '='
        if (n >= 3 && src[n - 3] == 61) { return null; }
        let unpadded = n - pad;
        let tail: usize = if (pad == 2) { 1 } else { 2 };
        return (unpadded / 4) * 3 + tail;
    }
    let rem = n & 3;
    if (rem == 1) { return null; }
    let extra: usize = if (rem == 0) { 0 } else { rem - 1 };
    return (n / 4) * 3 + extra;
}

/// Converts standard Base64 (`+`, `/`, `=`) to unpadded Base64URL (`-`, `_`).
pub fn fromStandardBase64(b64: string, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    while (i < b64.len && p < out.len) {
        let c = b64[i];
        if (c == 61) { // '=' padding stripped
            i = i + 1;
            continue;
        }
        if (c == 43) { out[p] = 45; }      // '+' -> '-'
        else if (c == 47) { out[p] = 95; } // '/' -> '_'
        else { out[p] = c; }
        p = p + 1;
        i = i + 1;
    }
    return p;
}

/// Upstream alias: converts standard Base64 (`+`, `/`, `=`) to unpadded Base64URL (`-`, `_`).
pub fn fromBase64(b64: string, out: []u8): usize {
    return fromStandardBase64(b64, out);
}

/// Converts unpadded Base64URL (`-`, `_`) to padded standard Base64 (`+`, `/`, `=`).
pub fn toStandardBase64(b64url: string, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    while (i < b64url.len && p < out.len) {
        let c = b64url[i];
        if (c == 45) { out[p] = 43; }      // '-' -> '+'
        else if (c == 95) { out[p] = 47; } // '_' -> '/'
        else { out[p] = c; }
        p = p + 1;
        i = i + 1;
    }
    let rem = p & 3;
    if (rem != 0) {
        let pad = 4 - rem;
        let k: usize = 0;
        while (k < pad && p < out.len) {
            out[p] = 61; // '='
            p = p + 1;
            k = k + 1;
        }
    }
    return p;
}

/// Upstream alias: converts Base64URL (`-`, `_`) to padded standard Base64 (`+`, `/`, `=`).
pub fn toBase64(b64url: string, out: []u8): usize {
    return toStandardBase64(b64url, out);
}

/// Pads a Base64URL string with `=` to a multiple of 4 (matching npm `base64url.padString`).
/// Returns written length, or `0` if `s.len % 4 == 1` or `out` is too small.
pub fn padString(s: string, out: []u8): usize {
    let rem = s.len & 3;
    if (rem == 1) { return 0; }
    let pad: usize = if (rem == 0) { 0 } else { 4 - rem };
    let total = s.len + pad;
    if (total > out.len) { return 0; }
    if (s.len > 0) {
        @memcpy(out[0..s.len], s);
    }
    let k: usize = 0;
    while (k < pad) {
        out[s.len + k] = 61; // '='
        k = k + 1;
    }
    return total;
}

/// Zero-copy strip of trailing `=` padding characters.
pub fn unpadString(s: string): string {
    let end = s.len;
    while (end > 0 && s[end - 1] == 61) {
        end = end - 1;
    }
    return s[0..end];
}

/// Validates whether `s` consists strictly of unpadded Base64URL alphabet characters (`A-Za-z0-9-_`)
/// with legal length (`len % 4 != 1`).
pub fn isValidBase64Url(s: string): bool {
    if ((s.len & 3) == 1) { return false; }
    let i: usize = 0;
    while (i < s.len) {
        let c = s[i];
        if (!isAlnum(c) && c != 45 && c != 95) { return false; }
        i = i + 1;
    }
    return true;
}

/// Upstream alias: validates unpadded Base64URL string.
pub fn validate(s: string): bool {
    return isValidBase64Url(s);
}

/// Validates a Base64URL string allowing optional canonical `=`/`==` padding.
pub fn validatePadded(s: string): bool {
    if (s.len == 0) { return true; }
    if (s[s.len - 1] != 61) {
        return isValidBase64Url(s);
    }
    if ((s.len & 3) != 0) { return false; }
    let pad: usize = 1;
    if (s.len >= 2 && s[s.len - 2] == 61) { pad = 2; }
    if (s.len >= 3 && s[s.len - 3] == 61) { return false; }
    let body = s[0..s.len - pad];
    let i: usize = 0;
    while (i < body.len) {
        let c = body[i];
        if (!isAlnum(c) && c != 45 && c != 95) { return false; }
        i = i + 1;
    }
    return true;
}
