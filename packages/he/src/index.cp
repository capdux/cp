// @cap/html-entities — Zero-allocation HTML5 entity & numeric reference UTF-8 codec for CP
// Clean-room MIT implementation inspired by npm `he`, `escape-html`, `html-entities`, and Rust `html-escape`.
//
// Features:
// - XSS-safe HTML escaping (`escapeHtml`, `needsHtmlEscape`) for &, <, >, ", '.
// - Non-ASCII UTF-8 codepoint encoding (`encodeNonAscii`) into `&#xHEX;` references.
// - Full HTML5 entity decoding (`decodeHtmlEntities`):
//   * Decimal (`&#128640;`) & hexadecimal (`&#x1F680;` / `&#X1f680;`) references to UTF-8 (1-4 bytes),
//     replacing invalid/surrogate/zero codepoints with U+FFFD.
//   * 60+ built-in high-frequency HTML5 named entities.
//   * Safely preserves unknown entities and stray '&' verbatim.
// - Zero heap allocation: writes directly into caller-provided `out: []u8`.

import { strEq, hexVal, hexCharUpper, isDigit } from "cap:ascii";
import { writeStr } from "cap:fmt";
import { decodeCodepoint, encodeCodepointLossy } from "cap:utf8";

fn pushByte(out: []u8, pos: usize, b: u8) -> usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn writeUtf8(out: []u8, pos: usize, raw_cp: u32) -> usize {
    if (pos >= out.len) {
        return pos;
    }
    const cp = if (raw_cp == 0) 0xFFFD else raw_cp;
    return pos + encodeCodepointLossy(cp, out[pos..out.len]);
}

fn pushHexEntity(out: []u8, pos: usize, cp: u32) -> usize {
    let p = writeStr("&#x", out, pos);
    let digits: [8]u8 = undefined;
    let count: usize = 0;
    let v = cp;
    if (v == 0) {
        digits[0] = '0';
        count = 1;
    } else {
        while (v > 0 && count < 8) {
            digits[count] = hexCharUpper((v & 0xF) as u8);
            count += 1;
            v = v >> 4;
        }
    }
    let k = count;
    while (k > 0) {
        k -= 1;
        p = pushByte(out, p, digits[k]);
    }
    p = pushByte(out, p, ';');
    return p;
}

/// Returns `true` if `s` contains any of `&`, `<`, `>`, `"`, or `'`.
pub fn needsHtmlEscape(s: string) -> bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '&' || c == '<' || c == '>' || c == '"' || c == '\'') {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Escapes `&`, `<`, `>`, `"`, `'` into `&amp;`, `&lt;`, `&gt;`, `&quot;`, `&#39;`.
pub fn escapeHtml(input: string, out: []u8) -> string {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const c = input[i];
        if (c == '&') {
            pos = writeStr("&amp;", out, pos);
        } else if (c == '<') {
            pos = writeStr("&lt;", out, pos);
        } else if (c == '>') {
            pos = writeStr("&gt;", out, pos);
        } else if (c == '"') {
            pos = writeStr("&quot;", out, pos);
        } else if (c == '\'') {
            pos = writeStr("&#39;", out, pos);
        } else {
            pos = pushByte(out, pos, c);
        }
        i += 1;
    }
    return out[0..pos];
}

/// Escapes the 5 core HTML characters AND encodes all non-ASCII UTF-8 codepoints as `&#xHEX;`.
pub fn encodeNonAscii(input: string, out: []u8) -> string {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const c = input[i];
        if (c == '&') {
            pos = writeStr("&amp;", out, pos);
            i += 1;
        } else if (c == '<') {
            pos = writeStr("&lt;", out, pos);
            i += 1;
        } else if (c == '>') {
            pos = writeStr("&gt;", out, pos);
            i += 1;
        } else if (c == '"') {
            pos = writeStr("&quot;", out, pos);
            i += 1;
        } else if (c == '\'') {
            pos = writeStr("&#39;", out, pos);
            i += 1;
        } else if (c < 0x80) {
            pos = pushByte(out, pos, c);
            i += 1;
        } else {
            const step = decodeCodepoint(input, i);
            pos = pushHexEntity(out, pos, step.codepoint);
            i += if (step.len > 0) step.len else 1;
        }
    }
    return out[0..pos];
}

/// Looks up a built-in HTML5 named entity (without '&' and ';'). Returns 0 if unknown.
fn lookupNamedEntity(name: string) -> u32 {
    if (name.len == 0 || name.len > 10) {
        return 0;
    }
    // Core XML / HTML5 entities
    if (strEq(name, "amp")) { return 0x0026; }
    if (strEq(name, "lt")) { return 0x003C; }
    if (strEq(name, "gt")) { return 0x003E; }
    if (strEq(name, "quot")) { return 0x0022; }
    if (strEq(name, "apos")) { return 0x0027; }
    if (strEq(name, "nbsp")) { return 0x00A0; }

    // Legal, typography & punctuation
    if (strEq(name, "copy")) { return 0x00A9; }
    if (strEq(name, "reg")) { return 0x00AE; }
    if (strEq(name, "trade")) { return 0x2122; }
    if (strEq(name, "mdash")) { return 0x2014; }
    if (strEq(name, "ndash")) { return 0x2013; }
    if (strEq(name, "hellip")) { return 0x2026; }
    if (strEq(name, "lsquo")) { return 0x2018; }
    if (strEq(name, "rsquo")) { return 0x2019; }
    if (strEq(name, "ldquo")) { return 0x201C; }
    if (strEq(name, "rdquo")) { return 0x201D; }
    if (strEq(name, "laquo")) { return 0x00AB; }
    if (strEq(name, "raquo")) { return 0x00BB; }
    if (strEq(name, "bull")) { return 0x2022; }
    if (strEq(name, "middot")) { return 0x00B7; }
    if (strEq(name, "sect")) { return 0x00A7; }
    if (strEq(name, "para")) { return 0x00B6; }
    if (strEq(name, "dagger")) { return 0x2020; }
    if (strEq(name, "Dagger")) { return 0x2021; }
    if (strEq(name, "permil")) { return 0x2030; }
    if (strEq(name, "prime")) { return 0x2032; }
    if (strEq(name, "Prime")) { return 0x2033; }

    // Currencies
    if (strEq(name, "euro")) { return 0x20AC; }
    if (strEq(name, "pound")) { return 0x00A3; }
    if (strEq(name, "yen")) { return 0x00A5; }
    if (strEq(name, "cent")) { return 0x00A2; }
    if (strEq(name, "curren")) { return 0x00A4; }

    // Math & fractions
    if (strEq(name, "deg")) { return 0x00B0; }
    if (strEq(name, "plusmn")) { return 0x00B1; }
    if (strEq(name, "times")) { return 0x00D7; }
    if (strEq(name, "divide")) { return 0x00F7; }
    if (strEq(name, "frac12")) { return 0x00BD; }
    if (strEq(name, "frac14")) { return 0x00BC; }
    if (strEq(name, "frac34")) { return 0x00BE; }
    if (strEq(name, "infin")) { return 0x221E; }
    if (strEq(name, "ne")) { return 0x2260; }
    if (strEq(name, "le")) { return 0x2264; }
    if (strEq(name, "ge")) { return 0x2265; }
    if (strEq(name, "approx")) { return 0x2248; }
    if (strEq(name, "minus")) { return 0x2212; }
    if (strEq(name, "radic")) { return 0x221A; }
    if (strEq(name, "sum")) { return 0x2211; }
    if (strEq(name, "prod")) { return 0x220F; }
    if (strEq(name, "micro")) { return 0x00B5; }

    // Arrows
    if (strEq(name, "larr")) { return 0x2190; }
    if (strEq(name, "uarr")) { return 0x2191; }
    if (strEq(name, "rarr")) { return 0x2192; }
    if (strEq(name, "darr")) { return 0x2193; }
    if (strEq(name, "harr")) { return 0x2194; }

    // Checkmarks & card suits
    if (strEq(name, "check")) { return 0x2713; }
    if (strEq(name, "cross")) { return 0x2717; }
    if (strEq(name, "hearts")) { return 0x2665; }
    if (strEq(name, "spades")) { return 0x2660; }
    if (strEq(name, "clubs")) { return 0x2663; }
    if (strEq(name, "diams")) { return 0x2666; }
    if (strEq(name, "starf")) { return 0x2605; }

    return 0;
}

/// Decodes HTML5 named entities and decimal/hexadecimal numeric character references into UTF-8.
/// Unknown entities and stray `&` characters are preserved verbatim.
pub fn decodeHtmlEntities(input: string, out: []u8) -> string {
    let pos: usize = 0;
    let i: usize = 0;

    while (i < input.len) {
        if (input[i] == '&') {
            // Find ';' within a reasonable entity span (max 16 bytes)
            let semi: usize = i + 1;
            const max_scan = if (i + 18 < input.len) { i + 18 } else { input.len };
            let found_semi = false;
            while (semi < max_scan) {
                const sc = input[semi];
                if (sc == ';') {
                    found_semi = true;
                    break;
                }
                if (sc == '&' || sc == ' ' || sc == '\n' || sc == '\r' || sc == '\t' || sc == '<') {
                    break;
                }
                semi += 1;
            }

            if (found_semi && semi > i + 1) {
                const body = input[(i + 1)..semi];

                // Numeric character reference: &#...; or &#x...; / &#X...;
                if (body[0] == '#') {
                    if (body.len >= 3 && (body[1] == 'x' || body[1] == 'X')) {
                        let cp: u32 = 0;
                        let valid = true;
                        let overflow = false;
                        let k: usize = 2;
                        while (k < body.len) {
                            if (d := hexVal(body[k])) {
                                if (cp > 0x10FFFF) {
                                    overflow = true;
                                } else {
                                    cp = (cp * 16) + (d as u32);
                                }
                            } else {
                                valid = false;
                                break;
                            }
                            k += 1;
                        }
                        if (valid) {
                            pos = writeUtf8(out, pos, if (overflow) { 0xFFFD } else { cp });
                            i = semi + 1;
                            continue;
                        }
                    } else if (body.len >= 2) {
                        let cp: u32 = 0;
                        let valid = true;
                        let overflow = false;
                        let k: usize = 1;
                        while (k < body.len) {
                            const c = body[k];
                            if (!isDigit(c)) {
                                valid = false;
                                break;
                            }
                            if (cp > 0x10FFFF) {
                                overflow = true;
                            } else {
                                cp = (cp * 10) + ((c - '0') as u32);
                            }
                            k += 1;
                        }
                        if (valid) {
                            pos = writeUtf8(out, pos, if (overflow) { 0xFFFD } else { cp });
                            i = semi + 1;
                            continue;
                        }
                    }
                } else {
                    // Named HTML5 entity
                    const cp = lookupNamedEntity(body);
                    if (cp != 0) {
                        pos = writeUtf8(out, pos, cp);
                        i = semi + 1;
                        continue;
                    }
                }
            }
        }

        pos = pushByte(out, pos, input[i]);
        i += 1;
    }

    return out[0..pos];
}
