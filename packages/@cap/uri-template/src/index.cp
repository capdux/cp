// packages/@cap/uri-template/src/index.cp
// Zero-allocation RFC 6570 URI Template (Levels 1-4) expander and parser.
// MIT clean-room pure CP implementation.

/// Key-value variable binding for RFC 6570 URI Template expansion.
pub struct TemplateVar {
    pub name: string,
    pub value: string,
}

fn utIsAlpha(c: u8): bool {
    return (c >= ('a' as u8) && c <= ('z' as u8)) || (c >= ('A' as u8) && c <= ('Z' as u8));
}

fn utIsDigit(c: u8): bool {
    return c >= ('0' as u8) && c <= ('9' as u8);
}

fn utIsHex(c: u8): bool {
    return utIsDigit(c) || (c >= ('a' as u8) && c <= ('f' as u8)) || (c >= ('A' as u8) && c <= ('F' as u8));
}

fn utHexUpper(n: u8): u8 {
    if (n < (10 as u8)) {
        return ('0' as u8) + n;
    }
    return ('A' as u8) + (n - (10 as u8));
}

fn utIsUnreserved(c: u8): bool {
    return utIsAlpha(c) || utIsDigit(c) || c == ('-' as u8) || c == ('.' as u8) || c == ('_' as u8) || c == ('~' as u8);
}

fn utIsReserved(c: u8): bool {
    if (c == (':' as u8) || c == ('/' as u8) || c == ('?' as u8) || c == ('#' as u8)) {
        return true;
    }
    if (c == ('[' as u8) || c == (']' as u8) || c == ('@' as u8) || c == ('!' as u8)) {
        return true;
    }
    if (c == ('$' as u8) || c == ('&' as u8) || c == ('\'' as u8) || c == ('(' as u8)) {
        return true;
    }
    if (c == (')' as u8) || c == ('*' as u8) || c == ('+' as u8) || c == (',' as u8)) {
        return true;
    }
    if (c == (';' as u8) || c == ('=' as u8)) {
        return true;
    }
    return false;
}

fn utIsOperator(c: u8): bool {
    return c == ('+' as u8) || c == ('#' as u8) || c == ('.' as u8) || c == ('/' as u8) || c == (';' as u8) || c == ('?' as u8) || c == ('&' as u8);
}

fn utIsReservedOp(c: u8): bool {
    return c == ('=' as u8) || c == (',' as u8) || c == ('!' as u8) || c == ('@' as u8) || c == ('|' as u8);
}

fn utEqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn utIsValidVarSpec(spec: []const u8): bool {
    if (spec.len == 0) {
        return false;
    }

    // Determine where modifier starts (':' or trailing '*').
    let nameEnd = spec.len;
    let hasPrefix = false;
    let prefixStart: usize = 0;

    let i: usize = 0;
    while (i < spec.len) {
        if (spec[i] == (':' as u8)) {
            nameEnd = i;
            hasPrefix = true;
            prefixStart = i + 1;
            break;
        }
        i += 1;
    }

    if (!hasPrefix && spec[spec.len - 1] == ('*' as u8)) {
        nameEnd = spec.len - 1;
    }

    if (nameEnd == 0) {
        return false;
    }

    // Validate varname: ALPHA / DIGIT / "_" / "." / pct-encoded.
    let vi: usize = 0;
    while (vi < nameEnd) {
        const c = spec[vi];
        if (utIsAlpha(c) || utIsDigit(c) || c == ('_' as u8) || c == ('.' as u8)) {
            vi += 1;
        } else if (c == ('%' as u8)) {
            if (vi + 2 >= nameEnd || !utIsHex(spec[vi + 1]) || !utIsHex(spec[vi + 2])) {
                return false;
            }
            vi += 3;
        } else {
            return false;
        }
    }

    // Validate optional ':max-length' (1..9999, 1-4 digits, first digit 1..9).
    if (hasPrefix) {
        const plen = spec.len - prefixStart;
        if (plen == 0 || plen > 4) {
            return false;
        }
        if (spec[prefixStart] < ('1' as u8) || spec[prefixStart] > ('9' as u8)) {
            return false;
        }
        let pi = prefixStart;
        while (pi < spec.len) {
            if (!utIsDigit(spec[pi])) {
                return false;
            }
            pi += 1;
        }
    }

    return true;
}

/// Validate an RFC 6570 URI Template string:
/// checks balanced `{...}`, valid operators (`+ # . / ; ? &`), valid variable names,
/// and valid `:max-length` / `*` modifiers.
pub fn isValid(tmpl: string): bool {
    let i: usize = 0;
    while (i < tmpl.len) {
        const c = tmpl[i];
        if (c == ('}' as u8)) {
            return false;
        }
        if (c == ('{' as u8)) {
            let close = i + 1;
            while (close < tmpl.len && tmpl[close] != ('}' as u8)) {
                if (tmpl[close] == ('{' as u8)) {
                    return false;
                }
                close += 1;
            }
            if (close >= tmpl.len) {
                return false;
            }
            let start = i + 1;
            if (start >= close) {
                return false;
            }
            const first = tmpl[start];
            if (utIsReservedOp(first)) {
                return false;
            }
            if (utIsOperator(first)) {
                start += 1;
            }
            if (start >= close) {
                return false;
            }

            // Validate comma-separated var-list.
            let p = start;
            while (p <= close) {
                let segEnd = p;
                while (segEnd < close && tmpl[segEnd] != (',' as u8)) {
                    segEnd += 1;
                }
                if (!utIsValidVarSpec(tmpl[p..segEnd])) {
                    return false;
                }
                if (segEnd >= close) {
                    break;
                }
                p = segEnd + 1;
                if (p == close) {
                    // Trailing comma is invalid.
                    return false;
                }
            }

            i = close + 1;
        } else {
            i += 1;
        }
    }
    return true;
}

/// Count the total number of template variables across all `{...}` expressions.
/// Returns `0` if `tmpl` is invalid.
pub fn countVars(tmpl: string): usize {
    if (!isValid(tmpl)) {
        return 0;
    }
    let total: usize = 0;
    let i: usize = 0;
    while (i < tmpl.len) {
        if (tmpl[i] == ('{' as u8)) {
            let close = i + 1;
            while (close < tmpl.len && tmpl[close] != ('}' as u8)) {
                close += 1;
            }
            let p = i + 1;
            if (p < close && utIsOperator(tmpl[p])) {
                p += 1;
            }
            while (p < close) {
                total += 1;
                while (p < close && tmpl[p] != (',' as u8)) {
                    p += 1;
                }
                if (p < close) {
                    p += 1;
                }
            }
            i = close + 1;
        } else {
            i += 1;
        }
    }
    return total;
}

/// Extract the `index`-th (0-based) variable name from `tmpl` into `out` (stripping operator and modifiers).
/// Returns bytes written, or `0` if `index` is out of bounds or `tmpl` is invalid.
pub fn extractVarName(tmpl: string, index: usize, out: []u8): usize {
    if (!isValid(tmpl)) {
        return 0;
    }
    let cur: usize = 0;
    let i: usize = 0;
    while (i < tmpl.len) {
        if (tmpl[i] == ('{' as u8)) {
            let close = i + 1;
            while (close < tmpl.len && tmpl[close] != ('}' as u8)) {
                close += 1;
            }
            let p = i + 1;
            if (p < close && utIsOperator(tmpl[p])) {
                p += 1;
            }
            while (p < close) {
                let segEnd = p;
                while (segEnd < close && tmpl[segEnd] != (',' as u8)) {
                    segEnd += 1;
                }
                if (cur == index) {
                    // Strip ':maxlen' or '*' modifier.
                    let nameEnd = segEnd;
                    let si = p;
                    while (si < segEnd) {
                        if (tmpl[si] == (':' as u8)) {
                            nameEnd = si;
                            break;
                        }
                        si += 1;
                    }
                    if (nameEnd == segEnd && nameEnd > p && tmpl[nameEnd - 1] == ('*' as u8)) {
                        nameEnd -= 1;
                    }
                    const nlen = nameEnd - p;
                    let copyLen = nlen;
                    if (copyLen > out.len) {
                        copyLen = out.len;
                    }
                    let ci: usize = 0;
                    while (ci < copyLen) {
                        out[ci] = tmpl[p + ci];
                        ci += 1;
                    }
                    return copyLen;
                }
                cur += 1;
                if (segEnd < close) {
                    p = segEnd + 1;
                } else {
                    p = close;
                }
            }
            i = close + 1;
        } else {
            i += 1;
        }
    }
    return 0;
}

/// Percent-encode `input` into `out` per RFC 6570.
/// When `allow_reserved` is true (`+` / `#` operators), unreserved, reserved, and existing `%XX` triples pass through.
/// Returns total bytes written into `out`.
pub fn pctEncode(input: string, allow_reserved: bool, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const c = input[i];
        if (utIsUnreserved(c)) {
            if (pos >= out.len) {
                break;
            }
            out[pos] = c;
            pos += 1;
            i += 1;
        } else if (allow_reserved && utIsReserved(c)) {
            if (pos >= out.len) {
                break;
            }
            out[pos] = c;
            pos += 1;
            i += 1;
        } else if (allow_reserved && c == ('%' as u8) && i + 2 < input.len && utIsHex(input[i + 1]) && utIsHex(input[i + 2])) {
            if (pos + 3 > out.len) {
                break;
            }
            out[pos] = '%' as u8;
            out[pos + 1] = input[i + 1];
            out[pos + 2] = input[i + 2];
            pos += 3;
            i += 3;
        } else {
            if (pos + 3 > out.len) {
                break;
            }
            out[pos] = '%' as u8;
            out[pos + 1] = utHexUpper((c >> (4 as u3)) & (0x0F as u8));
            out[pos + 2] = utHexUpper(c & (0x0F as u8));
            pos += 3;
            i += 1;
        }
    }
    return pos;
}

/// Slice `val` to at most `maxChars` UTF-8 code points (RFC 6570 prefix modifier).
fn utTruncateUtf8(val: []const u8, maxChars: usize): []const u8 {
    if (maxChars == 0) {
        return val[0..0];
    }
    let chars: usize = 0;
    let i: usize = 0;
    while (i < val.len) {
        if ((val[i] & (0xC0 as u8)) != (0x80 as u8)) {
            if (chars == maxChars) {
                return val[0..i];
            }
            chars += 1;
        }
        i += 1;
    }
    return val;
}

fn utWriteSlice(out: []u8, pos: *usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len && pos.* < out.len) {
        out[pos.*] = src[i];
        pos.* += 1;
        i += 1;
    }
}

fn utWriteByte(out: []u8, pos: *usize, b: u8) {
    if (pos.* < out.len) {
        out[pos.*] = b;
        pos.* += 1;
    }
}

/// Expand an RFC 6570 URI Template (Levels 1-4) into caller-provided `out` buffer.
/// Supports simple `{var}`, reserved `{+path}`, fragment `{#frag}`, label `{.ext}`,
/// path `{/list}`, path params `{;x,y}`, query `{?x,y}`, continuation `{&x,y}`,
/// multi-variable expressions, and prefix truncation `{var:3}`.
/// Returns bytes written into `out`.
pub fn expand(tmpl: string, vars: []const TemplateVar, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;

    while (i < tmpl.len && pos < out.len) {
        if (tmpl[i] != ('{' as u8)) {
            utWriteByte(out, &pos, tmpl[i]);
            i += 1;
            continue;
        }

        let close = i + 1;
        while (close < tmpl.len && tmpl[close] != ('}' as u8)) {
            close += 1;
        }
        if (close >= tmpl.len) {
            break;
        }

        let p = i + 1;
        let op: u8 = 0 as u8;
        if (p < close && utIsOperator(tmpl[p])) {
            op = tmpl[p];
            p += 1;
        }

        // RFC 6570 operator table.
        let firstChar: u8 = 0 as u8;
        let sepChar: u8 = ',' as u8;
        let named = false;
        let ifempEq = false;
        let allowReserved = false;

        if (op == ('+' as u8)) {
            allowReserved = true;
        } else if (op == ('#' as u8)) {
            firstChar = '#' as u8;
            allowReserved = true;
        } else if (op == ('.' as u8)) {
            firstChar = '.' as u8;
            sepChar = '.' as u8;
        } else if (op == ('/' as u8)) {
            firstChar = '/' as u8;
            sepChar = '/' as u8;
        } else if (op == (';' as u8)) {
            firstChar = ';' as u8;
            sepChar = ';' as u8;
            named = true;
        } else if (op == ('?' as u8)) {
            firstChar = '?' as u8;
            sepChar = '&' as u8;
            named = true;
            ifempEq = true;
        } else if (op == ('&' as u8)) {
            firstChar = '&' as u8;
            sepChar = '&' as u8;
            named = true;
            ifempEq = true;
        }

        let emittedAny = false;
        while (p < close) {
            let segEnd = p;
            while (segEnd < close && tmpl[segEnd] != (',' as u8)) {
                segEnd += 1;
            }

            // Parse varname and optional ':maxLen' prefix modifier.
            let nameEnd = segEnd;
            let hasMax = false;
            let maxLen: usize = 0;

            let si = p;
            while (si < segEnd) {
                if (tmpl[si] == (':' as u8)) {
                    nameEnd = si;
                    hasMax = true;
                    let mi = si + 1;
                    while (mi < segEnd && utIsDigit(tmpl[mi])) {
                        maxLen = maxLen * 10 + ((tmpl[mi] - ('0' as u8)) as usize);
                        mi += 1;
                    }
                    break;
                }
                si += 1;
            }
            if (!hasMax && nameEnd > p && tmpl[nameEnd - 1] == ('*' as u8)) {
                nameEnd -= 1;
            }

            const varName = tmpl[p..nameEnd];

            // Look up variable in `vars`.
            let found = false;
            let val: []const u8 = "";
            let vi: usize = 0;
            while (vi < vars.len) {
                if (utEqBytes(vars[vi].name, varName)) {
                    found = true;
                    val = vars[vi].value;
                    break;
                }
                vi += 1;
            }

            if (found) {
                if (!emittedAny) {
                    if (firstChar != (0 as u8)) {
                        utWriteByte(out, &pos, firstChar);
                    }
                    emittedAny = true;
                } else {
                    utWriteByte(out, &pos, sepChar);
                }

                if (named) {
                    utWriteSlice(out, &pos, varName);
                    if (val.len == 0) {
                        if (ifempEq) {
                            utWriteByte(out, &pos, '=' as u8);
                        }
                    } else {
                        utWriteByte(out, &pos, '=' as u8);
                    }
                }

                if (val.len > 0) {
                    let effectiveVal = val;
                    if (hasMax) {
                        effectiveVal = utTruncateUtf8(val, maxLen);
                    }
                    const written = pctEncode(effectiveVal, allowReserved, out[pos..out.len]);
                    pos += written;
                }
            }

            if (segEnd < close) {
                p = segEnd + 1;
            } else {
                p = close;
            }
        }

        i = close + 1;
    }

    return pos;
}

export {
    TemplateVar,
    isValid,
    countVars,
    expand,
    pctEncode,
    extractVarName,
};
