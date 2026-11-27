// @cap/link-header — Zero-allocation RFC 8288 HTTP Link header parser & builder
// Clean-room MIT implementation inspired by npm `parse-link-header`, `http-link-header`,
// and Rust `parse_link_header`.
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning, comptime 256-entry LUTs, and @memcpy.

import { eqIgnoreCase, findByte, trim } from "cap:ascii";
import { writeU64 } from "cap:fmt";

type LhVec16 = @Vector(16, u8);

fn lhBuildSpaceLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut[0x20] = 1;
    lut[0x09] = 1;
    lut[0x0A] = 1;
    lut[0x0B] = 1;
    lut[0x0C] = 1;
    lut[0x0D] = 1;
    return lut;
}

fn lhBuildKeyStopLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut[0x20] = 1;
    lut[0x09] = 1;
    lut[0x0A] = 1;
    lut[0x0B] = 1;
    lut[0x0C] = 1;
    lut[0x0D] = 1;
    lut['=' as usize] = 1;
    lut[';' as usize] = 1;
    lut[',' as usize] = 1;
    return lut;
}

const LH_SPACE_LUT: [256]u8 = lhBuildSpaceLut();
const LH_KEY_STOP_LUT: [256]u8 = lhBuildKeyStopLut();

fn lhSkipSpace(s: string, start: usize) -> usize {
    let i: usize = start;
    const len = s.len;
    const v_sp: LhVec16 = @splat(0x20 as u8);
    while (i + 16 <= len) {
        const v: LhVec16 = s[i .. i + 16][0..16].*;
        if (!@reduce(.And, v == v_sp)) {
            break;
        }
        i += 16;
    }
    while (i < len && LH_SPACE_LUT[s[i] as usize] != 0) {
        i += 1;
    }
    return i;
}

fn lhFindQuoteOrBackslash(s: string, start: usize) -> usize {
    let i: usize = start;
    const len = s.len;
    const v_q: LhVec16 = @splat('"' as u8);
    const v_bs: LhVec16 = @splat('\\' as u8);
    while (i + 16 <= len) {
        const v: LhVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_q) || @reduce(.Or, v == v_bs)) {
            let k: usize = 0;
            while (k < 16) {
                const c = s[i + k];
                if (c == '"' || c == '\\') {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c = s[i];
        if (c == '"' || c == '\\') {
            return i;
        }
        i += 1;
    }
    return len;
}

fn lhFindQuoteOrComma(s: string, start: usize) -> usize {
    let i: usize = start;
    const len = s.len;
    const v_q: LhVec16 = @splat('"' as u8);
    const v_c: LhVec16 = @splat(',' as u8);
    while (i + 16 <= len) {
        const v: LhVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_q) || @reduce(.Or, v == v_c)) {
            let k: usize = 0;
            while (k < 16) {
                const c = s[i + k];
                if (c == '"' || c == ',') {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c = s[i];
        if (c == '"' || c == ',') {
            return i;
        }
        i += 1;
    }
    return len;
}

fn lhWriteStr(src: string, out: []u8, pos: usize) -> usize {
    if (pos >= out.len || src.len == 0) {
        return pos;
    }
    const rem: usize = out.len - pos;
    const n: usize = if (src.len < rem) src.len else rem;
    @memcpy(out[pos .. pos + n], src[0 .. n]);
    return pos + n;
}

pub const MAX_LINK_ENTRIES: usize = 32;

pub struct LinkEntry {
    pub url: string,
    pub rel: string,
    pub title: string,
    pub type_val: string,
    pub hreflang: string,

    pub static fn empty() -> LinkEntry {
        return LinkEntry{
            url: "",
            rel: "",
            title: "",
            type_val: "",
            hreflang: "",
        };
    }
}

/// Checks if a whitespace-delimited `rel` attribute (e.g. `"next prefetch"`) contains `target`
/// (case-insensitive per RFC 8288).
fn relMatches(rel_attr: string, target: string) -> bool {
    if (rel_attr.len == 0 || target.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < rel_attr.len) {
        i = lhSkipSpace(rel_attr, i);
        const start = i;
        while (i < rel_attr.len && LH_SPACE_LUT[rel_attr[i] as usize] == 0) {
            i += 1;
        }
        if (i > start && eqIgnoreCase(rel_attr[start..i], target)) {
            return true;
        }
    }
    return false;
}

pub struct LinkHeader {
    pub entries: [32]LinkEntry,
    pub count: usize,

    pub static fn empty() -> LinkHeader {
        let entries: [32]LinkEntry = undefined;
        let i: usize = 0;
        while (i < MAX_LINK_ENTRIES) {
            entries[i] = LinkEntry.empty();
            i += 1;
        }
        return LinkHeader{
            entries: entries,
            count: 0,
        };
    }

    pub fn entryAt(i: usize) -> LinkEntry? {
        if (i < _.count) {
            return _.entries[i];
        }
        return null;
    }

    pub fn getByRel(rel: string) -> LinkEntry? {
        let i: usize = 0;
        while (i < _.count) {
            if (relMatches(_.entries[i].rel, rel)) {
                return _.entries[i];
            }
            i += 1;
        }
        return null;
    }

    pub fn nextUrl() -> string? {
        const entry = _.getByRel("next");
        if (entry != null) {
            return entry.?.url;
        }
        return null;
    }

    pub fn prevUrl() -> string? {
        const entry = _.getByRel("prev");
        if (entry != null) {
            return entry.?.url;
        }
        const alt = _.getByRel("previous");
        if (alt != null) {
            return alt.?.url;
        }
        return null;
    }

    pub fn firstUrl() -> string? {
        const entry = _.getByRel("first");
        if (entry != null) {
            return entry.?.url;
        }
        return null;
    }

    pub fn lastUrl() -> string? {
        const entry = _.getByRel("last");
        if (entry != null) {
            return entry.?.url;
        }
        return null;
    }
}

// ---------------------------------------------------------------------------
// RFC 8288 Parser (Zero Heap Allocation)
// ---------------------------------------------------------------------------

fn skipToNextEntry(s: string, start: usize) -> usize {
    let i = start;
    let in_quotes = false;
    while (i < s.len) {
        if (in_quotes) {
            i = lhFindQuoteOrBackslash(s, i);
            if (i >= s.len) {
                return s.len;
            }
            if (s[i] == '\\' && i + 1 < s.len) {
                i += 2;
                continue;
            }
            if (s[i] == '"') {
                in_quotes = false;
            }
            i += 1;
            continue;
        }
        i = lhFindQuoteOrComma(s, i);
        if (i >= s.len) {
            return s.len;
        }
        if (s[i] == '"') {
            in_quotes = true;
            i += 1;
            continue;
        }
        return i + 1;
    }
    return i;
}

/// Parses an RFC 8288 HTTP `Link` header value into a stack-allocated `LinkHeader`
/// (up to 32 entries). Malformed entries are safely skipped.
pub fn parseLinkHeader(header: string) -> LinkHeader {
    let res = LinkHeader.empty();
    let i: usize = 0;

    while (i < header.len && res.count < MAX_LINK_ENTRIES) {
        // Skip leading whitespace and stray commas
        while (i < header.len && (LH_SPACE_LUT[header[i] as usize] != 0 || header[i] == ',')) {
            i += 1;
        }
        if (i >= header.len) {
            break;
        }

        // Expect '<URL>'
        if (header[i] != '<') {
            i = skipToNextEntry(header, i);
            continue;
        }
        i += 1;
        const raw_url_start = i;
        if (gt_pos := findByte(header, '>', i)) {
            i = gt_pos;
        } else {
            // Unterminated '<'
            break;
        }
        const trimmed_url = trim(header[raw_url_start..i]);
        i += 1; // consume '>'

        let entry = LinkEntry.empty();
        entry.url = trimmed_url;

        // Parse '; key=val' parameters until ',' or end of header
        while (i < header.len) {
            i = lhSkipSpace(header, i);
            if (i >= header.len) {
                break;
            }
            if (header[i] == ',') {
                i += 1;
                break;
            }
            if (header[i] != ';') {
                // Unexpected token: skip to next entry safely
                i = skipToNextEntry(header, i);
                break;
            }
            i += 1; // consume ';'
            i = lhSkipSpace(header, i);
            const key_start = i;
            while (i < header.len && LH_KEY_STOP_LUT[header[i] as usize] == 0) {
                i += 1;
            }
            const key = header[key_start..i];
            i = lhSkipSpace(header, i);

            let val: string = "";
            if (i < header.len && header[i] == '=') {
                i += 1; // consume '='
                i = lhSkipSpace(header, i);
                if (i < header.len && header[i] == '"') {
                    i += 1; // consume opening '"'
                    const val_start = i;
                    while (i < header.len) {
                        i = lhFindQuoteOrBackslash(header, i);
                        if (i >= header.len) {
                            break;
                        }
                        if (header[i] == '"') {
                            break;
                        }
                        if (header[i] == '\\' && i + 1 < header.len) {
                            i += 2;
                            continue;
                        }
                        i += 1;
                    }
                    val = header[val_start..i];
                    if (i < header.len && header[i] == '"') {
                        i += 1; // consume closing '"'
                    }
                } else {
                    const val_start = i;
                    while (i < header.len && header[i] != ';' && header[i] != ',' && LH_SPACE_LUT[header[i] as usize] == 0) {
                        i += 1;
                    }
                    val = header[val_start..i];
                }
            }

            if (key.len > 0) {
                if (eqIgnoreCase(key, "rel")) {
                    // RFC 8288 §3.3: only the first `rel` parameter is significant
                    if (entry.rel.len == 0) {
                        entry.rel = val;
                    }
                } else if (eqIgnoreCase(key, "title")) {
                    entry.title = val;
                } else if (eqIgnoreCase(key, "type")) {
                    entry.type_val = val;
                } else if (eqIgnoreCase(key, "hreflang")) {
                    entry.hreflang = val;
                }
            }
        }

        if (entry.url.len > 0) {
            res.entries[res.count] = entry;
            res.count += 1;
        }
    }

    return res;
}

// ---------------------------------------------------------------------------
// RFC 8288 Formatter & Pagination Builder (Zero Heap Allocation)
// ---------------------------------------------------------------------------

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn appendParam(out: []u8, pos: usize, name: string, val: string) -> usize {
    if (val.len == 0) {
        return pos;
    }
    let p = lhWriteStr("; ", out, pos);
    p = lhWriteStr(name, out, p);
    p = lhWriteStr("=\"", out, p);
    p = lhWriteStr(val, out, p);
    p = appendByte(out, p, '"');
    return p;
}

/// Serializes a slice of `LinkEntry` items into an RFC 8288 `Link` header string in `out`.
pub fn formatLinkHeader(entries: []LinkEntry, out: []u8) -> string {
    let pos: usize = 0;
    let emitted: usize = 0;
    let i: usize = 0;
    while (i < entries.len) {
        const e = entries[i];
        if (e.url.len > 0) {
            if (emitted > 0) {
                pos = lhWriteStr(", ", out, pos);
            }
            pos = appendByte(out, pos, '<');
            pos = lhWriteStr(e.url, out, pos);
            pos = appendByte(out, pos, '>');
            pos = appendParam(out, pos, "rel", e.rel);
            pos = appendParam(out, pos, "title", e.title);
            pos = appendParam(out, pos, "type", e.type_val);
            pos = appendParam(out, pos, "hreflang", e.hreflang);
            emitted += 1;
        }
        i += 1;
    }
    return out[0..pos];
}

fn appendPageLink(
    out: []u8,
    start_pos: usize,
    base_url: string,
    sep: u8,
    target_page: usize,
    rel: string,
    emitted: usize,
) -> usize {
    let pos = start_pos;
    if (emitted > 0) {
        pos = lhWriteStr(", ", out, pos);
    }
    pos = appendByte(out, pos, '<');
    pos = lhWriteStr(base_url, out, pos);
    if (sep != 0) {
        pos = appendByte(out, pos, sep);
    }
    pos = lhWriteStr("page=", out, pos);
    pos = writeU64(target_page as u64, out, pos);
    pos = lhWriteStr(">; rel=\"", out, pos);
    pos = lhWriteStr(rel, out, pos);
    pos = appendByte(out, pos, '"');
    return pos;
}

/// Formats standard REST/GitHub pagination `Link` header (`prev`, `next`, `last`, `first`).
/// Automatically appends `?page=N` or `&page=N` based on whether `base_url` already has a query string.
pub fn formatPaginationLinks(base_url: string, page: usize, total_pages: usize, out: []u8) -> string {
    if (base_url.len == 0 || total_pages <= 1 || page == 0) {
        return out[0..0];
    }

    let cur = page;
    if (cur > total_pages) {
        cur = total_pages;
    }

    let sep: u8 = '?';
    if (findByte(base_url, '?', 0) != null) {
        const last = base_url[base_url.len - 1];
        if (last == '?' || last == '&') {
            sep = 0;
        } else {
            sep = '&';
        }
    }

    let pos: usize = 0;
    let emitted: usize = 0;

    if (cur > 1) {
        pos = appendPageLink(out, pos, base_url, sep, cur - 1, "prev", emitted);
        emitted += 1;
    }
    if (cur < total_pages) {
        pos = appendPageLink(out, pos, base_url, sep, cur + 1, "next", emitted);
        emitted += 1;
        pos = appendPageLink(out, pos, base_url, sep, total_pages, "last", emitted);
        emitted += 1;
    }
    if (cur > 1) {
        pos = appendPageLink(out, pos, base_url, sep, 1 as usize, "first", emitted);
        emitted += 1;
    }

    return out[0..pos];
}
