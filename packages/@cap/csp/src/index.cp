// packages/@cap/csp/src/index.cp
// Zero-allocation W3C Content-Security-Policy 3 header builder and parser.
// MIT clean-room pure CP implementation.

fn toLowerAscii(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn isAsciiSpace(c: u8): bool {
    return c == (' ' as u8) || c == ('\t' as u8) || c == ('\r' as u8) || c == ('\n' as u8);
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isValidDirectiveName(name: []const u8): bool {
    if (name.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < name.len) {
        const c = name[i];
        const isLower = c >= ('a' as u8) && c <= ('z' as u8);
        const isUpper = c >= ('A' as u8) && c <= ('Z' as u8);
        const isDigit = c >= ('0' as u8) && c <= ('9' as u8);
        const isHyphen = c == ('-' as u8);
        if (!isLower && !isUpper && !isDigit && !isHyphen) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isValidSourceValue(src: []const u8): bool {
    if (src.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) {
        const c = src[i];
        // Reject CTLs, whitespace, ';' and ',' to prevent header / directive injection.
        if (c <= (0x20 as u8) || c >= (0x7F as u8) || c == (';' as u8) || c == (',' as u8)) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Zero-allocation W3C CSP header builder writing into a caller-provided buffer.
pub struct CspBuilder {
    pub buf: []u8,
    pub pos: usize,
    pub count: usize,
    pub overflow: bool,

    pub static fn init(buf: []u8): CspBuilder {
        return CspBuilder{
            buf: buf,
            pos: 0,
            count: 0,
            overflow: false,
        };
    }

    /// Append a directive and its source expressions.
    /// Rejects invalid directive names or source values containing ';', ',', whitespace, or CRLF.
    /// Returns false on validation failure or buffer overflow.
    pub fn addDirective(*_, name: []const u8, sources: []const []const u8): bool {
        if (_.overflow) {
            return false;
        }
        if (!isValidDirectiveName(name)) {
            return false;
        }

        let need: usize = name.len;
        if (_.count > 0) {
            need += 2; // "; "
        }

        let si: usize = 0;
        while (si < sources.len) {
            const src = sources[si];
            if (!isValidSourceValue(src)) {
                return false;
            }
            need += 1 + src.len; // " " + src
            si += 1;
        }

        if (_.pos + need > _.buf.len) {
            _.overflow = true;
            return false;
        }

        if (_.count > 0) {
            _.buf[_.pos] = ';' as u8;
            _.buf[_.pos + 1] = ' ' as u8;
            _.pos += 2;
        }

        let ni: usize = 0;
        while (ni < name.len) {
            _.buf[_.pos] = name[ni];
            _.pos += 1;
            ni += 1;
        }

        let wi: usize = 0;
        while (wi < sources.len) {
            const src = sources[wi];
            _.buf[_.pos] = ' ' as u8;
            _.pos += 1;
            let ci: usize = 0;
            while (ci < src.len) {
                _.buf[_.pos] = src[ci];
                _.pos += 1;
                ci += 1;
            }
            wi += 1;
        }

        _.count += 1;
        return true;
    }

    pub fn written(): []const u8 {
        return _.buf[0.._.pos];
    }

    pub fn len(): usize {
        return _.pos;
    }

    pub fn isOverflow(): bool {
        return _.overflow;
    }
}

/// Byte span of a parsed CSP directive within the original header slice.
pub struct CspDirectiveSpan {
    pub name_start: usize,
    pub name_len: usize,
    pub val_start: usize,
    pub val_len: usize,
}

/// Zero-allocation parsed W3C CSP policy (up to 32 unique directives).
pub struct CspPolicy {
    pub directives: [32]CspDirectiveSpan,
    pub count: usize,

    pub static fn parse(header: []const u8): CspPolicy {
        let spans: [32]CspDirectiveSpan = undefined;
        let idx: usize = 0;
        while (idx < 32) {
            spans[idx] = CspDirectiveSpan{
                name_start: 0,
                name_len: 0,
                val_start: 0,
                val_len: 0,
            };
            idx += 1;
        }

        let count: usize = 0;
        let pos: usize = 0;
        while (pos < header.len) {
            // Find end of current ';'-delimited segment.
            let segEnd = pos;
            while (segEnd < header.len && header[segEnd] != (';' as u8)) {
                segEnd += 1;
            }

            // Trim leading and trailing ASCII whitespace.
            let start = pos;
            while (start < segEnd && isAsciiSpace(header[start])) {
                start += 1;
            }
            let end = segEnd;
            while (end > start && isAsciiSpace(header[end - 1])) {
                end -= 1;
            }

            if (start < end) {
                // Extract directive name (up to first ASCII whitespace).
                let nameEnd = start;
                while (nameEnd < end && !isAsciiSpace(header[nameEnd])) {
                    nameEnd += 1;
                }
                const nameSlice = header[start..nameEnd];

                // W3C CSP3: ignore duplicate directives (first occurrence wins).
                let isDup = false;
                let di: usize = 0;
                while (di < count) {
                    const existing = spans[di];
                    const existName = header[existing.name_start..(existing.name_start + existing.name_len)];
                    if (eqIgnoreCase(existName, nameSlice)) {
                        isDup = true;
                        break;
                    }
                    di += 1;
                }

                if (!isDup && count < 32) {
                    let valStart = nameEnd;
                    while (valStart < end && isAsciiSpace(header[valStart])) {
                        valStart += 1;
                    }
                    const valLen = end - valStart;
                    spans[count] = CspDirectiveSpan{
                        name_start: start,
                        name_len: nameEnd - start,
                        val_start: valStart,
                        val_len: valLen,
                    };
                    count += 1;
                }
            }

            if (segEnd < header.len) {
                pos = segEnd + 1;
            } else {
                pos = header.len;
            }
        }

        return CspPolicy{
            directives: spans,
            count: count,
        };
    }

    pub fn len(): usize {
        return _.count;
    }

    /// Case-insensitive lookup of a directive's source list slice (first occurrence per W3C CSP3).
    /// Returns empty slice if directive is absent or valueless.
    pub fn getDirective(header: []const u8, name: []const u8): []const u8 {
        let i: usize = 0;
        while (i < _.count) {
            const sp = _.directives[i];
            const dname = header[sp.name_start..(sp.name_start + sp.name_len)];
            if (eqIgnoreCase(dname, name)) {
                return header[sp.val_start..(sp.val_start + sp.val_len)];
            }
            i += 1;
        }
        return "";
    }

    /// Case-insensitive check whether a directive is present in the policy.
    pub fn hasDirective(header: []const u8, name: []const u8): bool {
        let i: usize = 0;
        while (i < _.count) {
            const sp = _.directives[i];
            const dname = header[sp.name_start..(sp.name_start + sp.name_len)];
            if (eqIgnoreCase(dname, name)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// Returns true if the given directive explicitly includes the `'unsafe-inline'` token
    /// (case-insensitive whole-token match).
    pub fn allowsUnsafeInline(header: []const u8, directive: []const u8): bool {
        const val = _.getDirective(header, directive);
        if (val.len == 0) {
            return false;
        }
        let i: usize = 0;
        while (i < val.len) {
            while (i < val.len && isAsciiSpace(val[i])) {
                i += 1;
            }
            const tokStart = i;
            while (i < val.len && !isAsciiSpace(val[i])) {
                i += 1;
            }
            if (i > tokStart) {
                const tok = val[tokStart..i];
                if (eqIgnoreCase(tok, "'unsafe-inline'")) {
                    return true;
                }
            }
        }
        return false;
    }
}

export {
    CspBuilder,
    CspDirectiveSpan,
    CspPolicy,
};
