// @cap/base64-url — Zero-heap RFC 4648 §5 Base64URL (unpadded) encoder, decoder & converter.
// Clean-room MIT port of npm `base64url` v3; reuses `@cap/data-encoding` with zero codec duplication.

import {
    base64Encode, base64Decode, base64EncodeLen, DecodeResult
} from "../../data-encoding/src/index.cp";
import { isAlnum } from "cap:ascii";

/// Encodes `src` bytes into unpadded Base64URL (`A-Za-z0-9-_`) into `out`.
pub fn encodeBase64Url(src: []const u8, out: []u8): usize {
    return base64Encode(src, true, false, out);
}

/// Decodes unpadded (or padded) Base64URL into `out`, returning `DecodeResult`.
pub fn decodeBase64Url(src: []const u8, out: []u8): DecodeResult {
    return base64Decode(src, true, out);
}

/// Returns the exact output byte length for unpadded Base64URL encoding of `input_len` bytes.
pub fn encodedBase64UrlLen(input_len: usize): usize {
    return base64EncodeLen(input_len, false);
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
