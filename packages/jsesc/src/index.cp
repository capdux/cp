// @cap/jsesc — Zero-allocation JavaScript & JSON string escaper
// Clean-room MIT implementation modeled on npm `jsesc` v3 (Mathias Bynens / Babel).
// Accelerated with 16-lane @Vector(16, u8) SIMD safe-run scanning, comptime 256-entry LUT, and @memcpy.

type JescVec16 = @Vector(16, u8);

const JESC_HEX_LOWER = "0123456789abcdef";
const JESC_HEX_UPPER = "0123456789ABCDEF";

fn jescBuildBaseSafeLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0x20;
    while (c <= 0x7E) {
        if (c != 92) { // '\\'
            lut[c] = 1;
        }
        c += 1;
    }
    return lut;
}

const JESC_BASE_SAFE_LUT: [256]u8 = jescBuildBaseSafeLut();

fn jescFindUnsafe(input: string, start: usize, quote: u8, check_lt: bool, check_dollar: bool): usize {
    let i: usize = start;
    const len = input.len;
    const v_20: JescVec16 = @splat(0x20 as u8);
    const v_7f: JescVec16 = @splat(0x7F as u8);
    const v_bs: JescVec16 = @splat('\\' as u8);
    const v_qt: JescVec16 = @splat(quote);
    const v_lt: JescVec16 = @splat('<' as u8);
    const v_dl: JescVec16 = @splat('$' as u8);
    while (i + 16 <= len) {
        const v: JescVec16 = input[i .. i + 16][0..16].*;
        if (@reduce(.Or, v < v_20) ||
            @reduce(.Or, v >= v_7f) ||
            @reduce(.Or, v == v_bs) ||
            @reduce(.Or, v == v_qt) ||
            (check_lt && @reduce(.Or, v == v_lt)) ||
            (check_dollar && @reduce(.Or, v == v_dl)))
        {
            let k: usize = 0;
            while (k < 16) {
                const c = input[i + k];
                if (JESC_BASE_SAFE_LUT[c as usize] == 0 || c == quote || (check_lt && c == '<') || (check_dollar && c == '$')) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c = input[i];
        if (JESC_BASE_SAFE_LUT[c as usize] == 0 || c == quote || (check_lt && c == '<') || (check_dollar && c == '$')) {
            return i;
        }
        i += 1;
    }
    return len;
}

pub struct JsescOptions {
    quote: u8,
    wrap: bool,
    escape_everything: bool,
    minimal: bool,
    is_script_context: bool,
    json: bool,
    es6: bool,
    lowercase_hex: bool,

    static fn default(): JsescOptions {
        return JsescOptions{
            quote: '\'',
            wrap: false,
            escape_everything: false,
            minimal: false,
            is_script_context: false,
            json: false,
            es6: false,
            lowercase_hex: false,
        };
    }

    static fn jsonDefault(): JsescOptions {
        return JsescOptions{
            quote: '"',
            wrap: true,
            escape_everything: false,
            minimal: false,
            is_script_context: false,
            json: true,
            es6: false,
            lowercase_hex: false,
        };
    }
}

struct JescDecoded {
    cp: u32,
    len: usize,
}

fn jescHexChar(nibble: u32, lowercase: bool): u8 {
    const idx = (nibble & 0xF) as usize;
    if (lowercase) { return JESC_HEX_LOWER[idx]; }
    return JESC_HEX_UPPER[idx];
}

fn jescPush(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn jescPush2(out: []u8, pos: usize, b0: u8, b1: u8): usize {
    let p = jescPush(out, pos, b0);
    p = jescPush(out, p, b1);
    return p;
}

fn jescPushHex2(out: []u8, pos: usize, val: u32, lowercase: bool): usize {
    let p = jescPush2(out, pos, '\\', 'x');
    p = jescPush(out, p, jescHexChar((val >> 4) & 0xF, lowercase));
    p = jescPush(out, p, jescHexChar(val & 0xF, lowercase));
    return p;
}

fn jescPushUnicode4(out: []u8, pos: usize, val: u32, lowercase: bool): usize {
    let p = jescPush2(out, pos, '\\', 'u');
    p = jescPush(out, p, jescHexChar((val >> 12) & 0xF, lowercase));
    p = jescPush(out, p, jescHexChar((val >> 8) & 0xF, lowercase));
    p = jescPush(out, p, jescHexChar((val >> 4) & 0xF, lowercase));
    p = jescPush(out, p, jescHexChar(val & 0xF, lowercase));
    return p;
}

fn jescPushEs6CodePoint(out: []u8, pos: usize, cp: u32, lowercase: bool): usize {
    let p = jescPush2(out, pos, '\\', 'u');
    p = jescPush(out, p, '{');
    let digits: [8]u8 = undefined;
    let count: usize = 0;
    let v = cp;
    if (v == 0) {
        digits[0] = '0';
        count = 1;
    } else {
        while (v > 0 && count < 8) {
            digits[count] = jescHexChar(v & 0xF, lowercase);
            count += 1;
            v = v >> 4;
        }
    }
    let k = count;
    while (k > 0) {
        k -= 1;
        p = jescPush(out, p, digits[k]);
    }
    p = jescPush(out, p, '}');
    return p;
}

fn jescDecodeUtf8(input: string, idx: usize): JescDecoded {
    const b0 = input[idx];
    if (b0 < 0x80) {
        return JescDecoded{ cp: u32(b0), len: 1 };
    }
    if ((b0 & 0xE0) == 0xC0 && idx + 1 < input.len) {
        const b1 = input[idx + 1];
        if ((b1 & 0xC0) == 0x80) {
            const cp = ((u32(b0 & 0x1F)) << 6) | u32(b1 & 0x3F);
            if (cp >= 0x80) {
                return JescDecoded{ cp: cp, len: 2 };
            }
        }
    } else if ((b0 & 0xF0) == 0xE0 && idx + 2 < input.len) {
        const b1 = input[idx + 1];
        const b2 = input[idx + 2];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            const cp = ((u32(b0 & 0x0F)) << 12) | ((u32(b1 & 0x3F)) << 6) | u32(b2 & 0x3F);
            if (cp >= 0x800) {
                return JescDecoded{ cp: cp, len: 3 };
            }
        }
    } else if ((b0 & 0xF8) == 0xF0 && idx + 3 < input.len) {
        const b1 = input[idx + 1];
        const b2 = input[idx + 2];
        const b3 = input[idx + 3];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            const cp = ((u32(b0 & 0x07)) << 18) | ((u32(b1 & 0x3F)) << 12) | ((u32(b2 & 0x3F)) << 6) | u32(b3 & 0x3F);
            if (cp >= 0x10000 && cp <= 0x10FFFF) {
                return JescDecoded{ cp: cp, len: 4 };
            }
        }
    }
    return JescDecoded{ cp: u32(b0), len: 1 };
}

fn jescAsciiLower(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') return b + 32;
    return b;
}

fn jescMatchesHtmlComment(input: string, idx: usize): bool {
    if (idx + 3 >= input.len) return false;
    return input[idx] == '<' && input[idx + 1] == '!' && input[idx + 2] == '-' && input[idx + 3] == '-';
}

fn jescMatchesCloseScript(input: string, idx: usize): bool {
    if (idx + 7 >= input.len) return false;
    return input[idx] == '<' &&
        input[idx + 1] == '/' &&
        jescAsciiLower(input[idx + 2]) == 's' &&
        jescAsciiLower(input[idx + 3]) == 'c' &&
        jescAsciiLower(input[idx + 4]) == 'r' &&
        jescAsciiLower(input[idx + 5]) == 'i' &&
        jescAsciiLower(input[idx + 6]) == 'p' &&
        jescAsciiLower(input[idx + 7]) == 't';
}

fn jescEscapeCodePoint(out: []u8, pos: usize, cp: u32, is_json: bool, use_es6: bool, lowercase: bool): usize {
    if (cp <= 0xFF) {
        if (is_json) {
            return jescPushUnicode4(out, pos, cp, lowercase);
        }
        if (use_es6 && cp > 0x7F) {
            return jescPushHex2(out, pos, cp, lowercase);
        }
        return jescPushHex2(out, pos, cp, lowercase);
    }
    if (cp <= 0xFFFF) {
        return jescPushUnicode4(out, pos, cp, lowercase);
    }
    // Supplementary plane (U+10000 .. U+10FFFF)
    if (use_es6 && !is_json) {
        return jescPushEs6CodePoint(out, pos, cp, lowercase);
    }
    const adj = cp - 0x10000;
    const high = 0xD800 + (adj >> 10);
    const low = 0xDC00 + (adj & 0x3FF);
    let p = jescPushUnicode4(out, pos, high, lowercase);
    p = jescPushUnicode4(out, p, low, lowercase);
    return p;
}

/// Escape `input` using full `JsescOptions` into caller-provided `out` buffer.
/// Returns number of bytes written.
pub fn jsescWithOptions(input: string, opts: JsescOptions, out: []u8): usize {
    const is_json = opts.json;
    let quote = opts.quote;
    if (is_json) {
        quote = '"';
    } else if (quote != '\'' && quote != '"' && quote != '`') {
        quote = '\'';
    }
    const do_wrap = opts.wrap || is_json;
    const use_es6 = opts.es6 && !is_json;
    const lower = opts.lowercase_hex;
    const check_lt = opts.is_script_context;
    const check_dollar = quote == '`';

    let pos: usize = 0;
    if (do_wrap) {
        pos = jescPush(out, pos, quote);
    }

    let i: usize = 0;
    while (i < input.len) {
        if (!opts.escape_everything) {
            const safe_end = jescFindUnsafe(input, i, quote, check_lt, check_dollar);
            if (safe_end > i) {
                const seg_len = safe_end - i;
                if (pos < out.len) {
                    const avail = out.len - pos;
                    const n = if (seg_len <= avail) { seg_len } else { avail };
                    @memcpy(out[pos .. pos + n], input[i .. i + n]);
                    pos += n;
                }
                i = safe_end;
                if (i >= input.len) { break; }
            }
        }

        const dec_cp = jescDecodeUtf8(input, i);
        const cp = dec_cp.cp;
        const step = dec_cp.len;

        // Script context protection: escape '<' in '<!--' and '</script' (case-insensitive)
        if (opts.is_script_context && cp == '<') {
            if (jescMatchesHtmlComment(input, i) || jescMatchesCloseScript(input, i)) {
                if (is_json) {
                    pos = jescPushUnicode4(out, pos, 0x3C, lower);
                } else {
                    pos = jescPushHex2(out, pos, 0x3C, lower);
                }
                i += step;
                continue;
            }
        }

        // Standard single-character escapes
        if (cp == '\\') {
            pos = jescPush2(out, pos, '\\', '\\');
            i += step;
            continue;
        }
        if (cp == 0x08) {
            pos = jescPush2(out, pos, '\\', 'b');
            i += step;
            continue;
        }
        if (cp == 0x0C) {
            pos = jescPush2(out, pos, '\\', 'f');
            i += step;
            continue;
        }
        if (cp == 0x0A) {
            pos = jescPush2(out, pos, '\\', 'n');
            i += step;
            continue;
        }
        if (cp == 0x0D) {
            pos = jescPush2(out, pos, '\\', 'r');
            i += step;
            continue;
        }
        if (cp == 0x09) {
            pos = jescPush2(out, pos, '\\', 't');
            i += step;
            continue;
        }
        if (cp == 0x00) {
            const next_is_digit = (i + step < input.len) && input[i + step] >= '0' && input[i + step] <= '9';
            if (is_json) {
                pos = jescPushUnicode4(out, pos, 0, lower);
            } else if (next_is_digit) {
                pos = jescPushHex2(out, pos, 0, lower);
            } else {
                pos = jescPush2(out, pos, '\\', '0');
            }
            i += step;
            continue;
        }

        // Active quote character
        if (cp == u32(quote)) {
            pos = jescPush2(out, pos, '\\', quote);
            i += step;
            continue;
        }

        // Template literal interpolation guard: escape '$' before '{' when quote is '`'
        if (quote == '`' && cp == '$' && i + 1 < input.len && input[i + 1] == '{') {
            pos = jescPush2(out, pos, '\\', '$');
            i += step;
            continue;
        }

        // Line separator (U+2028) & Paragraph separator (U+2029) are always escaped
        if (cp == 0x2028 || cp == 0x2029) {
            pos = jescPushUnicode4(out, pos, cp, lower);
            i += step;
            continue;
        }

        // Printable ASCII (0x20 .. 0x7E)
        if (cp >= 0x20 && cp <= 0x7E) {
            if (opts.escape_everything) {
                pos = jescEscapeCodePoint(out, pos, cp, is_json, use_es6, lower);
            } else {
                pos = jescPush(out, pos, u8(cp));
            }
            i += step;
            continue;
        }

        // Remaining ASCII control characters (0x01..0x1F, 0x7F)
        if (cp < 0x80) {
            pos = jescEscapeCodePoint(out, pos, cp, is_json, use_es6, lower);
            i += step;
            continue;
        }

        // Non-ASCII characters (cp >= 0x80)
        if (opts.minimal && !opts.escape_everything) {
            let k: usize = 0;
            while (k < step) : (k += 1) {
                pos = jescPush(out, pos, input[i + k]);
            }
        } else {
            pos = jescEscapeCodePoint(out, pos, cp, is_json, use_es6, lower);
        }
        i += step;
    }

    if (do_wrap) {
        pos = jescPush(out, pos, quote);
    }
    return pos;
}

/// Escape `input` with default `jsesc` options (single-quote, minimal=false, wrap=false).
pub fn jsesc(input: string, out: []u8): usize {
    return jsescWithOptions(input, JsescOptions.default(), out);
}
