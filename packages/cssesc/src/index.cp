// @cap/cssesc — Zero-allocation CSSOM-compliant CSS identifier/string escaper in pure CP
// Reference: npm:cssesc (https://github.com/mathiasbynens/cssesc, MIT)

pub enum CssQuoteStyle {
    Single,
    Double,
}

pub struct CssEscOptions {
    is_identifier: bool,
    quotes: CssQuoteStyle,
    wrap: bool,
    escape_everything: bool,
}

struct CescWriter {
    buf: []u8,
    pos: usize,
    overflow: bool,

    fn writeByte(*_, b: u8) {
        if (_.overflow) return;
        if (_.pos >= _.buf.len) {
            _.overflow = true;
            return;
        }
        _.buf[_.pos] = b;
        _.pos += 1;
    }

    fn writeHexEscape(*_, cp: u32, add_space: bool) {
        _.writeByte('\\');
        let tmp: [8]u8 = undefined;
        let len: usize = 0;
        let v = cp;
        if (v == 0) {
            _.writeByte('0');
        } else {
            while (v > 0) {
                const nib = u8(v & 0xF);
                if (nib < 10) {
                    tmp[len] = u8('0') + nib;
                } else {
                    tmp[len] = u8('a') + (nib - 10);
                }
                len += 1;
                v >>= 4;
            }
            while (len > 0) {
                len -= 1;
                _.writeByte(tmp[len]);
            }
        }
        if (add_space) {
            _.writeByte(' ');
        }
    }
}

struct CescUtf8Res {
    cp: u32,
    next: usize,
}

@tls let cesc_tls_buf: [16384]u8 = undefined;

pub fn defaultCssEscOptions(): CssEscOptions {
    return CssEscOptions{
        is_identifier: false,
        quotes: .Single,
        wrap: false,
        escape_everything: false,
    };
}

fn cescDecodeUtf8(s: string, start: usize): CescUtf8Res {
    const b0 = s[start];
    if (b0 < 0x80) {
        return CescUtf8Res{ cp: b0 as u32, next: start + 1 };
    }
    if ((b0 & 0xE0) == 0xC0 && start + 1 < s.len) {
        const b1 = s[start + 1];
        if ((b1 & 0xC0) == 0x80) {
            const cp = (((b0 & 0x1F) as u32) << 6) | ((b1 & 0x3F) as u32);
            return CescUtf8Res{ cp: cp, next: start + 2 };
        }
    }
    if ((b0 & 0xF0) == 0xE0 && start + 2 < s.len) {
        const b1 = s[start + 1];
        const b2 = s[start + 2];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            const cp = (((b0 & 0x0F) as u32) << 12) | (((b1 & 0x3F) as u32) << 6) | ((b2 & 0x3F) as u32);
            return CescUtf8Res{ cp: cp, next: start + 3 };
        }
    }
    if ((b0 & 0xF8) == 0xF0 && start + 3 < s.len) {
        const b1 = s[start + 1];
        const b2 = s[start + 2];
        const b3 = s[start + 3];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            const cp = (((b0 & 0x07) as u32) << 18) | (((b1 & 0x3F) as u32) << 12) | (((b2 & 0x3F) as u32) << 6) | ((b3 & 0x3F) as u32);
            return CescUtf8Res{ cp: cp, next: start + 4 };
        }
    }
    return CescUtf8Res{ cp: b0 as u32, next: start + 1 };
}

fn cescIsAsciiAlnumOrDash(cp: u32): bool {
    if ((cp >= 0x30 && cp <= 0x39) || (cp >= 0x41 && cp <= 0x5A) || (cp >= 0x61 && cp <= 0x7A) || cp == 0x2D || cp == 0x5F) {
        return true;
    }
    return false;
}

pub fn cssescBuf(input: string, opts: CssEscOptions, out: []u8): usize {
    let w = CescWriter{ buf: out, pos: 0, overflow: false };

    let quote_char: u8 = '\'';
    if (opts.quotes == .Double) {
        quote_char = '"';
    }

    if (opts.wrap) {
        w.writeByte(quote_char);
    }

    let i: usize = 0;
    let cp_idx: usize = 0;
    let first_is_dash = false;

    while (i < input.len) {
        const r = cescDecodeUtf8(input, i);
        const cp = r.cp;
        const is_last = r.next >= input.len;
        const is_first = cp_idx == 0;
        const is_second = cp_idx == 1;
        if (is_first && cp == 0x2D) {
            first_is_dash = true;
        }
        i = r.next;
        cp_idx += 1;

        // NUL -> U+FFFD (EF BF BD)
        if (cp == 0) {
            w.writeByte(0xEF);
            w.writeByte(0xBF);
            w.writeByte(0xBD);
            continue;
        }

        // Control chars (U+0001..U+001F, U+007F)
        if ((cp >= 0x01 && cp <= 0x1F) || cp == 0x7F) {
            w.writeHexEscape(cp, !is_last);
            continue;
        }

        // escape_everything mode
        if (opts.escape_everything) {
            w.writeHexEscape(cp, !is_last);
            continue;
        }

        // Non-ASCII characters (>= 0x80) -> hex escape per cssesc default
        if (cp >= 0x80) {
            w.writeHexEscape(cp, !is_last);
            continue;
        }

        // Identifier-specific leading digit / lone dash rules
        if (opts.is_identifier) {
            if (is_first && cp >= 0x30 && cp <= 0x39) {
                w.writeHexEscape(cp, !is_last);
                continue;
            }
            if (is_second && first_is_dash && cp >= 0x30 && cp <= 0x39) {
                w.writeHexEscape(cp, !is_last);
                continue;
            }
            if (is_first && is_last && cp == 0x2D) {
                w.writeByte('\\');
                w.writeByte('-');
                continue;
            }
        }

        // Backslash always escaped
        if (cp == 0x5C) {
            w.writeByte('\\');
            w.writeByte('\\');
            continue;
        }

        if (opts.is_identifier) {
            if (cescIsAsciiAlnumOrDash(cp)) {
                w.writeByte(u8(cp));
            } else {
                w.writeByte('\\');
                w.writeByte(u8(cp));
            }
        } else {
            if ((opts.quotes == .Single && cp == 0x27) || (opts.quotes == .Double && cp == 0x22)) {
                w.writeByte('\\');
                w.writeByte(u8(cp));
            } else {
                w.writeByte(u8(cp));
            }
        }
    }

    if (opts.wrap) {
        w.writeByte(quote_char);
    }

    if (w.overflow) return 0;
    return w.pos;
}

pub fn cssesc(input: string, opts: CssEscOptions): string {
    const n = cssescBuf(input, opts, cesc_tls_buf[0..16384]);
    return cesc_tls_buf[0..n];
}
