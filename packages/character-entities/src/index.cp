// @cap/character-entities — HTML5 named & numeric character entity codec in pure CP
// Clean-room MIT implementation of npm `character-entities` (#576, 99.1M/wk),
// `decode-named-character-reference` (#642), `parse-entities` (#679), `stringify-entities` (#684).

import { strEq, hexVal } from "cap:ascii";
import { writeStr } from "cap:fmt";
import { encodeCodepointLossy } from "cap:utf8";

/// Maps Windows-1252 C1 invalid numeric character references (0x80..0x9F) per WHATWG HTML5 spec.
pub fn mapInvalidCharRef(cp: u32): u32 {
    if (cp == 0x80) { return 0x20AC; } // €
    if (cp == 0x82) { return 0x201A; } // ‚
    if (cp == 0x83) { return 0x0192; } // ƒ
    if (cp == 0x84) { return 0x201E; } // „
    if (cp == 0x85) { return 0x2026; } // …
    if (cp == 0x86) { return 0x2020; } // †
    if (cp == 0x87) { return 0x2021; } // ‡
    if (cp == 0x88) { return 0x02C6; } // ˆ
    if (cp == 0x89) { return 0x2030; } // ‰
    if (cp == 0x8A) { return 0x0160; } // Š
    if (cp == 0x8B) { return 0x2039; } // ‹
    if (cp == 0x8C) { return 0x0152; } // Œ
    if (cp == 0x8E) { return 0x017D; } // Ž
    if (cp == 0x91) { return 0x2018; } // ‘
    if (cp == 0x92) { return 0x2019; } // ’
    if (cp == 0x93) { return 0x201C; } // “
    if (cp == 0x94) { return 0x201D; } // ”
    if (cp == 0x95) { return 0x2022; } // •
    if (cp == 0x96) { return 0x2013; } // –
    if (cp == 0x97) { return 0x2014; } // —
    if (cp == 0x98) { return 0x02DC; } // ˜
    if (cp == 0x99) { return 0x2122; } // ™
    if (cp == 0x9A) { return 0x0161; } // š
    if (cp == 0x9B) { return 0x203A; } // ›
    if (cp == 0x9C) { return 0x0153; } // œ
    if (cp == 0x9E) { return 0x017E; } // ž
    if (cp == 0x9F) { return 0x0178; } // Ÿ
    return cp;
}

/// Decodes a named HTML5 character reference (without `&` and `;`) to its Unicode codepoint.
pub fn decodeNamedEntity(name: string): u32? {
    if (strEq(name, "amp") || strEq(name, "AMP")) { return 0x0026; }
    if (strEq(name, "lt") || strEq(name, "LT")) { return 0x003C; }
    if (strEq(name, "gt") || strEq(name, "GT")) { return 0x003E; }
    if (strEq(name, "quot") || strEq(name, "QUOT")) { return 0x0022; }
    if (strEq(name, "apos")) { return 0x0027; }
    if (strEq(name, "nbsp") || strEq(name, "NonBreakingSpace")) { return 0x00A0; }
    if (strEq(name, "copy") || strEq(name, "COPY")) { return 0x00A9; }
    if (strEq(name, "reg") || strEq(name, "REG")) { return 0x00AE; }
    if (strEq(name, "trade") || strEq(name, "TRADE")) { return 0x2122; }
    if (strEq(name, "euro")) { return 0x20AC; }
    if (strEq(name, "pound")) { return 0x00A3; }
    if (strEq(name, "yen")) { return 0x00A5; }
    if (strEq(name, "cent")) { return 0x00A2; }
    if (strEq(name, "sect")) { return 0x00A7; }
    if (strEq(name, "deg")) { return 0x00B0; }
    if (strEq(name, "plusmn")) { return 0x00B1; }
    if (strEq(name, "micro")) { return 0x00B5; }
    if (strEq(name, "middot")) { return 0x00B7; }
    if (strEq(name, "times")) { return 0x00D7; }
    if (strEq(name, "divide")) { return 0x00F7; }
    if (strEq(name, "ndash")) { return 0x2013; }
    if (strEq(name, "mdash")) { return 0x2014; }
    if (strEq(name, "lsquo")) { return 0x2018; }
    if (strEq(name, "rsquo")) { return 0x2019; }
    if (strEq(name, "ldquo")) { return 0x201C; }
    if (strEq(name, "rdquo")) { return 0x201D; }
    if (strEq(name, "bull")) { return 0x2022; }
    if (strEq(name, "hellip")) { return 0x2026; }
    if (strEq(name, "larr")) { return 0x2190; }
    if (strEq(name, "uarr")) { return 0x2191; }
    if (strEq(name, "rarr")) { return 0x2192; }
    if (strEq(name, "darr")) { return 0x2193; }
    if (strEq(name, "harr")) { return 0x2194; }
    if (strEq(name, "laquo")) { return 0x00AB; }
    if (strEq(name, "raquo")) { return 0x00BB; }
    if (strEq(name, "alpha")) { return 0x03B1; }
    if (strEq(name, "beta")) { return 0x03B2; }
    if (strEq(name, "gamma")) { return 0x03B3; }
    if (strEq(name, "delta")) { return 0x03B4; }
    if (strEq(name, "pi")) { return 0x03C0; }
    if (strEq(name, "omega")) { return 0x03C9; }
    if (strEq(name, "infin")) { return 0x221E; }
    if (strEq(name, "ne")) { return 0x2260; }
    if (strEq(name, "le")) { return 0x2264; }
    if (strEq(name, "ge")) { return 0x2265; }
    if (strEq(name, "tab") || strEq(name, "Tab")) { return 0x0009; }
    if (strEq(name, "newline") || strEq(name, "NewLine")) { return 0x000A; }
    return null;
}

/// Decodes a numeric character reference body (after `&#`, e.g. `"65"` or `"x1F600"`) with WHATWG mapping.
pub fn decodeNumericEntity(body: string): u32? {
    if (body.len == 0) {
        return null;
    }
    let is_hex = false;
    let idx: usize = 0;
    if (body[0] == 120 || body[0] == 88) { // 'x' or 'X'
        is_hex = true;
        idx = 1;
    }
    if (idx >= body.len) {
        return null;
    }
    let val: u32 = 0;
    while (idx < body.len) {
        const b = body[idx];
        let d: u32 = 0;
        if (is_hex) {
            if (hv := hexVal(b)) {
                d = hv as u32;
            } else {
                return null;
            }
        } else if (b >= 48 && b <= 57) {
            d = (b - 48) as u32;
        } else {
            return null;
        }
        if (val <= 0x10FFFF) {
            if (is_hex) {
                val = val * 16 + d;
            } else {
                val = val * 10 + d;
            }
        }
        idx += 1;
    }
    const mapped = mapInvalidCharRef(val);
    if (mapped == 0 || (mapped >= 0xD800 && mapped <= 0xDFFF) || mapped > 0x10FFFF) {
        return 0xFFFD;
    }
    return mapped;
}

/// Decodes all named and numeric HTML character entities in `input` into `out`; returns bytes written.
pub fn decodeEntities(input: string, out: []u8): usize {
    let i: usize = 0;
    let p: usize = 0;
    while (i < input.len) {
        if (input[i] == 38) { // '&'
            // Find ';' within next 32 bytes
            let semi: usize = 0;
            let found = false;
            let k = i + 1;
            let limit = i + 33;
            if (limit > input.len) { limit = input.len; }
            while (k < limit) {
                if (input[k] == 59) { // ';'
                    semi = k;
                    found = true;
                    break;
                }
                if (input[k] == 38 || input[k] == 32) {
                    break;
                }
                k += 1;
            }
            if (found && semi > i + 1) {
                const body = input[(i + 1)..semi];
                let cp_opt: u32? = null;
                if (body[0] == 35) { // '#'
                    cp_opt = decodeNumericEntity(body[1..body.len]);
                } else {
                    cp_opt = decodeNamedEntity(body);
                }
                if (cp_opt != null) {
                    const cp = cp_opt ?? 0xFFFD;
                    const safe_cp: u32 = if (cp == 0) 0xFFFD else cp;
                    p += encodeCodepointLossy(safe_cp, out[p..out.len]);
                    i = semi + 1;
                    continue;
                }
            }
        }
        if (p < out.len) {
            out[p] = input[i];
            p += 1;
        }
        i += 1;
    }
    return p;
}

/// Encodes HTML-special characters (`&`, `<`, `>`, `"`, `'`) into named entities in `out`.
pub fn encodeEntities(input: string, out: []u8): usize {
    let i: usize = 0;
    let p: usize = 0;
    while (i < input.len) {
        const b = input[i];
        if (b == 38) {
            p = writeStr("&amp;", out, p);
        } else if (b == 60) {
            p = writeStr("&lt;", out, p);
        } else if (b == 62) {
            p = writeStr("&gt;", out, p);
        } else if (b == 34) {
            p = writeStr("&quot;", out, p);
        } else if (b == 39) {
            p = writeStr("&#x27;", out, p);
        } else {
            if (p < out.len) {
                out[p] = b;
                p += 1;
            }
        }
        i += 1;
    }
    return p;
}

export {
    mapInvalidCharRef,
    decodeNamedEntity,
    decodeNumericEntity,
    decodeEntities,
    encodeEntities,
};
