// @cap/link-header — Zero-allocation RFC 8288 HTTP Link header parser & builder
// Clean-room MIT implementation inspired by npm `parse-link-header`, `http-link-header`,
// and Rust `parse_link_header`.

import { isSpace, eqIgnoreCase, findByte, trim } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";

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
        while (i < rel_attr.len && isSpace(rel_attr[i])) {
            i += 1;
        }
        const start = i;
        while (i < rel_attr.len && !isSpace(rel_attr[i])) {
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
        const c = s[i];
        if (in_quotes) {
            if (c == '\\' && i + 1 < s.len) {
                i += 2;
                continue;
            }
            if (c == '"') {
                in_quotes = false;
            }
            i += 1;
            continue;
        }
        if (c == '"') {
            in_quotes = true;
            i += 1;
            continue;
        }
        if (c == ',') {
            return i + 1;
        }
        i += 1;
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
        while (i < header.len && (isSpace(header[i]) || header[i] == ',')) {
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
            while (i < header.len && isSpace(header[i])) {
                i += 1;
            }
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
            while (i < header.len && isSpace(header[i])) {
                i += 1;
            }
            const key_start = i;
            while (i < header.len && header[i] != '=' && header[i] != ';' && header[i] != ',' && !isSpace(header[i])) {
                i += 1;
            }
            const key = header[key_start..i];
            while (i < header.len && isSpace(header[i])) {
                i += 1;
            }

            let val: string = "";
            if (i < header.len && header[i] == '=') {
                i += 1; // consume '='
                while (i < header.len && isSpace(header[i])) {
                    i += 1;
                }
                if (i < header.len && header[i] == '"') {
                    i += 1; // consume opening '"'
                    const val_start = i;
                    while (i < header.len && header[i] != '"') {
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
                    while (i < header.len && header[i] != ';' && header[i] != ',' && !isSpace(header[i])) {
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
    let p = writeStr("; ", out, pos);
    p = writeStr(name, out, p);
    p = writeStr("=\"", out, p);
    p = writeStr(val, out, p);
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
                pos = writeStr(", ", out, pos);
            }
            pos = appendByte(out, pos, '<');
            pos = writeStr(e.url, out, pos);
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
        pos = writeStr(", ", out, pos);
    }
    pos = appendByte(out, pos, '<');
    pos = writeStr(base_url, out, pos);
    if (sep != 0) {
        pos = appendByte(out, pos, sep);
    }
    pos = writeStr("page=", out, pos);
    pos = writeU64(target_page as u64, out, pos);
    pos = writeStr(">; rel=\"", out, pos);
    pos = writeStr(rel, out, pos);
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
