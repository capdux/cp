// @cap/data-uri — Zero-allocation RFC 2397 Data URI parser, decoder & builder
// Clean-room MIT implementation inspired by npm `data-uri-to-buffer`, `datauri`,
// and Rust `data-url`.

import { isSpace, trim, eqIgnoreCase, hexCharUpper, hexVal, isAlnum, findByte } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub struct DataUri {
    pub ok: bool,
    pub media_type: string,
    pub charset: string,
    pub is_base64: bool,
    pub raw_payload: string,

    pub static fn invalid() -> DataUri {
        return DataUri{
            ok: false,
            media_type: "",
            charset: "",
            is_base64: false,
            raw_payload: "",
        };
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    return writeStr(s, out, pos);
}

fn isUnreservedUriByte(c: u8) -> bool {
    return isAlnum(c) || c == '-' || c == '_' || c == '.' || c == '~';
}

// ---------------------------------------------------------------------------
// 1. RFC 2397 Parser
// ---------------------------------------------------------------------------

/// Parses an RFC 2397 `data:[<mediatype>][;base64],<data>` URI with zero heap allocation.
/// Defaults `media_type` to `"text/plain"` and `charset` to `"US-ASCII"` when omitted.
pub fn parseDataUri(uri: string) -> DataUri {
    if (uri.len < 5) {
        return DataUri.invalid();
    }
    if (!eqIgnoreCase(uri[0..5], "data:")) {
        return DataUri.invalid();
    }

    // Locate the first ',' separating metadata from payload
    const comma_opt = findByte(uri, ',', 5);
    if (comma_opt == null) {
        return DataUri.invalid();
    }
    const comma = comma_opt.?;

    const meta = uri[5..comma];
    const raw_payload = uri[(comma + 1)..uri.len];

    let media_type: string = "text/plain";
    let charset: string = "US-ASCII";
    let is_base64 = false;

    if (meta.len > 0) {
        let seg_idx: usize = 0;
        let seg_start: usize = 0;
        let p: usize = 0;
        while (p <= meta.len) {
            if (p == meta.len || meta[p] == ';') {
                const is_last = (p == meta.len);
                const seg = trim(meta[seg_start..p]);

                if (seg.len > 0) {
                    if (is_last && eqIgnoreCase(seg, "base64")) {
                        is_base64 = true;
                    } else {
                        // Check if this segment is a `key=value` parameter
                        const eq_opt = findByte(seg, '=', 0);
                        if (eq_opt != null) {
                            const eq_pos = eq_opt.?;
                            const param_k = trim(seg[0..eq_pos]);
                            let param_v = trim(seg[(eq_pos + 1)..seg.len]);
                            if (param_v.len >= 2 && param_v[0] == '"' && param_v[param_v.len - 1] == '"') {
                                param_v = param_v[1..(param_v.len - 1)];
                            }
                            if (eqIgnoreCase(param_k, "charset") && param_v.len > 0) {
                                charset = param_v;
                            }
                        } else if (seg_idx == 0) {
                            // First segment is `<type>/<subtype>`
                            const slash_opt = findByte(seg, '/', 0);
                            if (slash_opt == null || slash_opt.? == 0 || slash_opt.? + 1 >= seg.len) {
                                return DataUri.invalid();
                            }
                            media_type = seg;
                        }
                    }
                }

                seg_idx += 1;
                seg_start = p + 1;
            }
            p += 1;
        }
    }

    return DataUri{
        ok: true,
        media_type: media_type,
        charset: charset,
        is_base64: is_base64,
        raw_payload: raw_payload,
    };
}

/// Returns `true` if `s` is a structurally valid RFC 2397 Data URI.
pub fn isDataUri(s: string) -> bool {
    return parseDataUri(s).ok;
}

// ---------------------------------------------------------------------------
// 2. Decoding (Base64 & Percent-Decoding)
// ---------------------------------------------------------------------------

fn b64Val(c: u8) -> u8? {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return (c - 'a') + 26;
    if (c >= '0' && c <= '9') return (c - '0') + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return null;
}

fn decodeBase64Payload(payload: string, out: []u8) -> ([]u8)? {
    let pos: usize = 0;
    let quad: [4]u8 = undefined;
    let qlen: usize = 0;
    let pad_count: usize = 0;
    let saw_pad = false;

    let i: usize = 0;
    while (i < payload.len) {
        const c = payload[i];
        if (isSpace(c)) {
            i += 1;
            continue;
        }
        if (c == '=') {
            if (qlen < 2) {
                return null;
            }
            saw_pad = true;
            pad_count += 1;
            quad[qlen] = 0;
            qlen += 1;
        } else {
            if (saw_pad) {
                return null;
            }
            const v = b64Val(c);
            if (v == null) {
                return null;
            }
            quad[qlen] = v.?;
            qlen += 1;
        }

        if (qlen == 4) {
            if (pad_count > 2) {
                return null;
            }
            const b0: u8 = (quad[0] << 2) | (quad[1] >> 4);
            const b1: u8 = ((quad[1] & 0x0F) << 4) | (quad[2] >> 2);
            const b2: u8 = ((quad[2] & 0x03) << 6) | quad[3];

            if (pos >= out.len) return null;
            out[pos] = b0;
            pos += 1;

            if (pad_count < 2) {
                if (pos >= out.len) return null;
                out[pos] = b1;
                pos += 1;
            }
            if (pad_count == 0) {
                if (pos >= out.len) return null;
                out[pos] = b2;
                pos += 1;
            }

            qlen = 0;
            pad_count = 0;
        }
        i += 1;
    }

    if (qlen == 1) {
        return null;
    }
    if (qlen == 2) {
        if (saw_pad) return null;
        if (pos >= out.len) return null;
        out[pos] = (quad[0] << 2) | (quad[1] >> 4);
        pos += 1;
    } else if (qlen == 3) {
        if (saw_pad) return null;
        if (pos + 2 > out.len) return null;
        out[pos] = (quad[0] << 2) | (quad[1] >> 4);
        out[pos + 1] = ((quad[1] & 0x0F) << 4) | (quad[2] >> 2);
        pos += 2;
    }

    return out[0..pos];
}

fn decodePercentPayload(payload: string, out: []u8) -> ([]u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < payload.len) {
        if (payload[i] == '%') {
            if (i + 2 >= payload.len) {
                return null;
            }
            const hi = hexVal(payload[i + 1]);
            const lo = hexVal(payload[i + 2]);
            if (hi == null || lo == null) {
                return null;
            }
            if (pos >= out.len) {
                return null;
            }
            out[pos] = (hi.? << 4) | lo.?;
            pos += 1;
            i += 3;
        } else {
            if (pos >= out.len) {
                return null;
            }
            out[pos] = payload[i];
            pos += 1;
            i += 1;
        }
    }
    return out[0..pos];
}

/// Decodes an RFC 2397 Data URI payload into `out`, automatically applying Base64 or
/// `%XX` percent-decoding. Returns `null` on invalid URI, invalid encoding, or buffer overflow.
pub fn decodeDataUri(uri: string, out: []u8) -> ([]u8)? {
    const parsed = parseDataUri(uri);
    if (!parsed.ok) {
        return null;
    }
    if (parsed.is_base64) {
        return decodeBase64Payload(parsed.raw_payload, out);
    }
    return decodePercentPayload(parsed.raw_payload, out);
}

// ---------------------------------------------------------------------------
// 3. Encoding / Building
// ---------------------------------------------------------------------------

const B64_ALPHABET: string = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/// Encodes binary `payload` into a `data:<media_type>;base64,<b64>` URI written to `out`.
pub fn encodeDataUriBase64(media_type: string, payload: []u8, out: []u8) -> string {
    let pos: usize = 0;
    pos = appendStr(out, pos, "data:");
    if (media_type.len > 0) {
        pos = appendStr(out, pos, media_type);
    }
    pos = appendStr(out, pos, ";base64,");

    let i: usize = 0;
    while (i + 3 <= payload.len) {
        const b0 = payload[i];
        const b1 = payload[i + 1];
        const b2 = payload[i + 2];

        pos = appendByte(out, pos, B64_ALPHABET[(b0 >> 2) as usize]);
        pos = appendByte(out, pos, B64_ALPHABET[(((b0 & 0x03) << 4) | (b1 >> 4)) as usize]);
        pos = appendByte(out, pos, B64_ALPHABET[(((b1 & 0x0F) << 2) | (b2 >> 6)) as usize]);
        pos = appendByte(out, pos, B64_ALPHABET[(b2 & 0x3F) as usize]);
        i += 3;
    }

    const rem = payload.len - i;
    if (rem == 1) {
        const b0 = payload[i];
        pos = appendByte(out, pos, B64_ALPHABET[(b0 >> 2) as usize]);
        pos = appendByte(out, pos, B64_ALPHABET[((b0 & 0x03) << 4) as usize]);
        pos = appendByte(out, pos, '=');
        pos = appendByte(out, pos, '=');
    } else if (rem == 2) {
        const b0 = payload[i];
        const b1 = payload[i + 1];
        pos = appendByte(out, pos, B64_ALPHABET[(b0 >> 2) as usize]);
        pos = appendByte(out, pos, B64_ALPHABET[(((b0 & 0x03) << 4) | (b1 >> 4)) as usize]);
        pos = appendByte(out, pos, B64_ALPHABET[((b1 & 0x0F) << 2) as usize]);
        pos = appendByte(out, pos, '=');
    }

    return out[0..pos];
}

/// Encodes `text` into a percent-encoded `data:<media_type>,<encoded>` URI written to `out`.
pub fn encodeDataUriText(media_type: string, text: string, out: []u8) -> string {
    let pos: usize = 0;
    pos = appendStr(out, pos, "data:");
    if (media_type.len > 0) {
        pos = appendStr(out, pos, media_type);
    }
    pos = appendByte(out, pos, ',');

    let i: usize = 0;
    while (i < text.len) {
        const c = text[i];
        if (isUnreservedUriByte(c)) {
            pos = appendByte(out, pos, c);
        } else {
            pos = appendByte(out, pos, '%');
            pos = appendByte(out, pos, hexCharUpper((c >> 4) & 0x0F));
            pos = appendByte(out, pos, hexCharUpper(c & 0x0F));
        }
        i += 1;
    }
    return out[0..pos];
}
