// packages/@cap/user-agent-hints/src/index.cp
// Zero-allocation W3C User-Agent Client Hints (Sec-CH-UA*) parser and builder.
// MIT clean-room pure CP implementation.

/// Zero-copy brand and version pair parsed from `Sec-CH-UA` / `Sec-CH-UA-Full-Version-List`.
pub struct BrandVersion {
    pub brand: string,
    pub version: string,
}

fn uahToLower(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn uahIsOws(c: u8): bool {
    return c == (' ' as u8) || c == ('\t' as u8);
}

fn uahTrimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && uahIsOws(s[start])) {
        start += 1;
    }
    while (end > start && uahIsOws(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn uahEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (uahToLower(a[i]) != uahToLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Find first case-insensitive occurrence of `needle` in `haystack` starting at `from`.
/// Returns index or `haystack.len` if not found.
fn uahFindIgnoreCaseFrom(haystack: []const u8, needle: []const u8, start_idx: usize): usize {
    if (needle.len == 0 || start_idx + needle.len > haystack.len) {
        return haystack.len;
    }
    let i = start_idx;
    while (i + needle.len <= haystack.len) {
        let ok = true;
        let j: usize = 0;
        while (j < needle.len) {
            if (uahToLower(haystack[i + j]) != uahToLower(needle[j])) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) {
            return i;
        }
        i += 1;
    }
    return haystack.len;
}

/// Scan a quoted RFC 8941 string starting at `header[pos] == '"'`.
/// Writes inner slice bounds into `innerStart`/`innerEnd`, advances `pos` past closing '"',
/// and returns true on success.
fn uahScanQuoted(header: []const u8, pos: *usize, innerStart: *usize, innerEnd: *usize): bool {
    if (pos.* >= header.len || header[pos.*] != ('"' as u8)) {
        return false;
    }
    pos.* += 1;
    innerStart.* = pos.*;
    while (pos.* < header.len) {
        const c = header[pos.*];
        if (c == ('\\' as u8) && pos.* + 1 < header.len) {
            pos.* += 2;
        } else if (c == ('"' as u8)) {
            innerEnd.* = pos.*;
            pos.* += 1;
            return true;
        } else {
            pos.* += 1;
        }
    }
    return false;
}

/// Advance `pos` to the next unquoted ',' (or end of `header`).
fn uahSkipToNextItem(header: []const u8, pos: *usize) {
    let inQuotes = false;
    while (pos.* < header.len) {
        const c = header[pos.*];
        if (inQuotes) {
            if (c == ('\\' as u8) && pos.* + 1 < header.len) {
                pos.* += 2;
            } else if (c == ('"' as u8)) {
                inQuotes = false;
                pos.* += 1;
            } else {
                pos.* += 1;
            }
        } else {
            if (c == ('"' as u8)) {
                inQuotes = true;
                pos.* += 1;
            } else if (c == (',' as u8)) {
                pos.* += 1;
                return;
            } else {
                pos.* += 1;
            }
        }
    }
}

/// Parse one brand-version list item at `pos`, advancing `pos` past trailing ','.
/// Returns true if a valid quoted brand item was parsed.
fn uahParseNextBrand(header: []const u8, pos: *usize, item: *BrandVersion): bool {
    while (pos.* < header.len) {
        while (pos.* < header.len && (uahIsOws(header[pos.*]) || header[pos.*] == (',' as u8))) {
            pos.* += 1;
        }
        if (pos.* >= header.len) {
            return false;
        }

        let bStart: usize = 0;
        let bEnd: usize = 0;
        if (!uahScanQuoted(header, pos, &bStart, &bEnd)) {
            uahSkipToNextItem(header, pos);
            continue;
        }

        let verSlice = header[bEnd..bEnd];

        // Parse ';' parameters until ',' or end of header.
        while (pos.* < header.len) {
            while (pos.* < header.len && uahIsOws(header[pos.*])) {
                pos.* += 1;
            }
            if (pos.* >= header.len) {
                break;
            }
            if (header[pos.*] == (',' as u8)) {
                pos.* += 1;
                break;
            }
            if (header[pos.*] != (';' as u8)) {
                uahSkipToNextItem(header, pos);
                break;
            }
            // Skip ';'.
            pos.* += 1;
            while (pos.* < header.len && uahIsOws(header[pos.*])) {
                pos.* += 1;
            }

            const keyStart = pos.*;
            while (pos.* < header.len && header[pos.*] != ('=' as u8) && header[pos.*] != (';' as u8) && header[pos.*] != (',' as u8)) {
                pos.* += 1;
            }
            const key = uahTrimOws(header[keyStart..pos.*]);

            if (pos.* < header.len && header[pos.*] == ('=' as u8)) {
                pos.* += 1;
                while (pos.* < header.len && uahIsOws(header[pos.*])) {
                    pos.* += 1;
                }
                if (pos.* < header.len && header[pos.*] == ('"' as u8)) {
                    let vStart: usize = 0;
                    let vEnd: usize = 0;
                    if (uahScanQuoted(header, pos, &vStart, &vEnd)) {
                        if (uahEqIgnoreCase(key, "v")) {
                            verSlice = header[vStart..vEnd];
                        }
                    } else {
                        uahSkipToNextItem(header, pos);
                        break;
                    }
                } else {
                    const vStart = pos.*;
                    while (pos.* < header.len && header[pos.*] != (';' as u8) && header[pos.*] != (',' as u8)) {
                        pos.* += 1;
                    }
                    if (uahEqIgnoreCase(key, "v")) {
                        verSlice = uahTrimOws(header[vStart..pos.*]);
                    }
                }
            }
        }

        item.* = BrandVersion{
            brand: header[bStart..bEnd],
            version: verSlice,
        };
        return true;
    }
    return false;
}

/// Parse an RFC 8941 `Sec-CH-UA` or `Sec-CH-UA-Full-Version-List` structured list into `out`.
/// Handles escaped quotes and commas/semicolons inside quoted strings.
/// Returns the number of brand entries written to `out`.
pub fn parseBrands(header: string, out: []BrandVersion): usize {
    let pos: usize = 0;
    let count: usize = 0;
    let item = BrandVersion{ brand: "", version: "" };

    while (count < out.len && uahParseNextBrand(header, &pos, &item)) {
        out[count] = item;
        count += 1;
    }
    return count;
}

/// Detect Chromium/W3C GREASE placeholder brands (e.g. `"Not-A.Brand"`, `"Not/A)Brand"`, `"Not:A-Brand"`).
pub fn isGreaseBrand(brand: string): bool {
    const notPos = uahFindIgnoreCaseFrom(brand, "not", 0);
    if (notPos >= brand.len) {
        return false;
    }
    const brandPos = uahFindIgnoreCaseFrom(brand, "brand", notPos + 3);
    return brandPos < brand.len;
}

/// Case-insensitive lookup of a specific brand's version in `Sec-CH-UA*`.
/// Returns the version slice if found, or `""` if absent.
pub fn findBrandVersion(header: string, brand: string): string {
    if (brand.len == 0) {
        return "";
    }
    let pos: usize = 0;
    let item = BrandVersion{ brand: "", version: "" };
    while (uahParseNextBrand(header, &pos, &item)) {
        if (uahEqIgnoreCase(item.brand, brand)) {
            return item.version;
        }
    }
    return "";
}

/// Parse `Sec-CH-UA-Mobile` RFC 8941 boolean (`"?1"` -> `1`, `"?0"` -> `0`, invalid -> `-1`).
pub fn parseMobile(header: string): i32 {
    const t = uahTrimOws(header);
    if (t.len == 2 && t[0] == ('?' as u8)) {
        if (t[1] == ('1' as u8)) {
            return 1 as i32;
        }
        if (t[1] == ('0' as u8)) {
            return 0 as i32;
        }
    }
    return -1 as i32;
}

/// Parse `Sec-CH-UA-Platform` quoted RFC 8941 string (e.g. `"\"macOS\""` -> `"macOS"`).
/// Returns empty slice `""` if not a valid quoted string.
pub fn parsePlatform(header: string): string {
    const t = uahTrimOws(header);
    if (t.len < 2 || t[0] != ('"' as u8) || t[t.len - 1] != ('"' as u8)) {
        return "";
    }
    let pos: usize = 0;
    let sStart: usize = 0;
    let sEnd: usize = 0;
    if (!uahScanQuoted(t, &pos, &sStart, &sEnd)) {
        return "";
    }
    if (pos != t.len) {
        return "";
    }
    return t[sStart..sEnd];
}

fn uahQuotedEncodedLen(s: []const u8): usize {
    let n: usize = 2; // surrounding quotes
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ('"' as u8) || s[i] == ('\\' as u8)) {
            n += 2;
        } else {
            n += 1;
        }
        i += 1;
    }
    return n;
}

fn uahWriteQuoted(out: []u8, pos: *usize, s: []const u8) {
    out[pos.*] = '"' as u8;
    pos.* += 1;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == ('"' as u8) || c == ('\\' as u8)) {
            out[pos.*] = '\\' as u8;
            out[pos.* + 1] = c;
            pos.* += 2;
        } else {
            out[pos.*] = c;
            pos.* += 1;
        }
        i += 1;
    }
    out[pos.*] = '"' as u8;
    pos.* += 1;
}

/// Format `brands` into a standard `Sec-CH-UA` RFC 8941 structured list (`"Brand";v="Ver", ...`).
/// Stops before overflowing `out` so no partial entry is ever written.
/// Returns total bytes written into `out`.
pub fn formatBrands(brands: []const BrandVersion, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < brands.len) {
        const b = brands[i];
        let need: usize = uahQuotedEncodedLen(b.brand) + 3 + uahQuotedEncodedLen(b.version); // ";v=" = 3
        if (i > 0) {
            need += 2; // ", "
        }
        if (pos + need > out.len) {
            break;
        }
        if (i > 0) {
            out[pos] = ',' as u8;
            out[pos + 1] = ' ' as u8;
            pos += 2;
        }
        uahWriteQuoted(out, &pos, b.brand);
        out[pos] = ';' as u8;
        out[pos + 1] = 'v' as u8;
        out[pos + 2] = '=' as u8;
        pos += 3;
        uahWriteQuoted(out, &pos, b.version);
        i += 1;
    }
    return pos;
}

export {
    BrandVersion,
    parseBrands,
    isGreaseBrand,
    findBrandVersion,
    parseMobile,
    parsePlatform,
    formatBrands,
};
