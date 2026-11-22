// Origin: Pure-CP Stdlib (Zero-Alloc RFC 1436 Gopher Protocol & Menu Codec) | Created: 2026-11-22

pub const GOPHER_TCP_PORT: u16 = 70;

// RFC 1436 & De-Facto Gopher Item Type Constants
pub const TYPE_TEXT: u8 = '0';
pub const TYPE_DIR: u8 = '1';
pub const TYPE_CSO: u8 = '2';
pub const TYPE_ERROR: u8 = '3';
pub const TYPE_BINHEX: u8 = '4';
pub const TYPE_DOS: u8 = '5';
pub const TYPE_UUENCODED: u8 = '6';
pub const TYPE_SEARCH: u8 = '7';
pub const TYPE_TELNET: u8 = '8';
pub const TYPE_BINARY: u8 = '9';
pub const TYPE_MIRROR: u8 = '+';
pub const TYPE_GIF: u8 = 'g';
pub const TYPE_IMAGE: u8 = 'I';
pub const TYPE_INFO: u8 = 'i';
pub const TYPE_HTML: u8 = 'h';
pub const TYPE_SOUND: u8 = 's';

fn stripLineEnding(line: []const u8): []const u8 {
    let end = line.len;
    if (end >= 2 && line[end - 2] == '\r' && line[end - 1] == '\n') {
        end -= 2;
    } else if (end >= 1 && line[end - 1] == '\n') {
        end -= 1;
    }
    return line[0..end];
}

fn hasCrLfOrNul(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '\r' || c == '\n' || c == 0) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn hasTabCrLfOrNul(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '\t' || c == '\r' || c == '\n' || c == 0) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn findTab(s: []const u8, start: usize): usize? {
    let i = start;
    while (i < s.len) {
        if (s[i] == '\t') {
            return i;
        }
        i += 1;
    }
    return null;
}

fn hasPrefixUrl(s: []const u8): bool {
    return s.len >= 4 && s[0] == 'U' && s[1] == 'R' && s[2] == 'L' && s[3] == ':';
}

fn parsePortU16(s: []const u8): u16? {
    if (s.len == 0 || s.len > 5) {
        return null;
    }
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        acc = acc * 10 + ((c - '0') as u32);
        if (acc > 65535) {
            return null;
        }
        i += 1;
    }
    return acc as u16;
}

/// Parsed RFC 1436 Gopher client request (`<selector>[\t<search_query>]\r\n`).
pub struct GopherRequest {
    selector: []const u8,
    search_query: []const u8,
    has_search: bool,
}

/// Parses a Gopher request line, splitting on the first `\t` into selector and optional search query.
/// Empty selector (e.g. `"\r\n"`) is valid and denotes the root menu.
pub fn parseGopherRequest(line: []const u8): GopherRequest? {
    const clean = stripLineEnding(line);
    if (hasCrLfOrNul(clean)) {
        return null;
    }
    if (tab_pos := findTab(clean, 0)) {
        return GopherRequest{
            .selector = clean[0..tab_pos],
            .search_query = clean[tab_pos + 1 ..],
            .has_search = true,
        };
    }
    return GopherRequest{
        .selector = clean,
        .search_query = clean[clean.len..clean.len],
        .has_search = false,
    };
}

/// Zero-alloc encoder for a Gopher request (`<selector>[\t<search_query>]\r\n`).
/// Rejects `\r`, `\n`, NUL injection and `\t` inside selector.
pub fn encodeGopherRequest(selector: []const u8, search_query: []const u8, out: []u8): usize? {
    if (hasTabCrLfOrNul(selector) || hasCrLfOrNul(search_query)) {
        return null;
    }
    let total: usize = selector.len + 2;
    if (search_query.len > 0) {
        total += 1 + search_query.len;
    }
    if (out.len < total) {
        return null;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < selector.len) {
        out[pos] = selector[i];
        pos += 1;
        i += 1;
    }
    if (search_query.len > 0) {
        out[pos] = '\t';
        pos += 1;
        let j: usize = 0;
        while (j < search_query.len) {
            out[pos] = search_query[j];
            pos += 1;
            j += 1;
        }
    }
    out[pos] = '\r';
    out[pos + 1] = '\n';
    return pos + 2;
}

/// Parsed single Gopher Menu directory entry (`<type><display>\t<selector>\t<host>\t<port>`).
pub struct GopherItem {
    item_type: u8,
    display: []const u8,
    selector: []const u8,
    host: []const u8,
    port: u16,

    fn isInfo(): bool {
        return _.item_type == TYPE_INFO;
    }

    fn isDir(): bool {
        return _.item_type == TYPE_DIR;
    }

    fn isText(): bool {
        return _.item_type == TYPE_TEXT;
    }

    fn isSearch(): bool {
        return _.item_type == TYPE_SEARCH;
    }

    fn isHtmlUrl(): bool {
        return _.item_type == TYPE_HTML && hasPrefixUrl(_.selector);
    }

    /// For `h` items with a `URL:` selector prefix, returns the underlying URL slice.
    fn htmlUrl(): ([]const u8)? {
        if (_.isHtmlUrl()) {
            return _.selector[4..];
        }
        return null;
    }
}

/// Strictly parses a single Gopher Menu line into a `GopherItem`.
pub fn parseGopherItem(line: []const u8): GopherItem? {
    const clean = stripLineEnding(line);
    if (clean.len < 4 || hasCrLfOrNul(clean)) {
        return null;
    }
    const item_type = clean[0];
    if (item_type < 0x21 || item_type > 0x7E) {
        return null;
    }

    const t1 = findTab(clean, 1) ?? return null;
    const t2 = findTab(clean, t1 + 1) ?? return null;
    const t3 = findTab(clean, t2 + 1) ?? return null;

    // Optional 4th TAB for Gopher+ extension attributes; port is between t3+1 and t4 (or EOL)
    let port_end = clean.len;
    if (t4 := findTab(clean, t3 + 1)) {
        port_end = t4;
    }

    const port = parsePortU16(clean[t3 + 1 .. port_end]) ?? return null;

    return GopherItem{
        .item_type = item_type,
        .display = clean[1..t1],
        .selector = clean[t1 + 1 .. t2],
        .host = clean[t2 + 1 .. t3],
        .port = port,
    };
}

/// Zero-alloc encoder for a Gopher Menu item (`<type><display>\t<selector>\t<host>\t<port>\r\n`).
/// Rejects `\t`, `\r`, `\n`, and NUL injection in fields.
pub fn encodeGopherItem(
    item_type: u8,
    display: []const u8,
    selector: []const u8,
    host: []const u8,
    port: u16,
    out: []u8,
): usize? {
    if (item_type < 0x21 || item_type > 0x7E) {
        return null;
    }
    if (hasTabCrLfOrNul(display) || hasTabCrLfOrNul(selector) || hasTabCrLfOrNul(host)) {
        return null;
    }

    // Format port into a 5-byte stack buffer
    let port_digits: [5]u8 = [0, 0, 0, 0, 0];
    let port_len: usize = 0;
    if (port == 0) {
        port_digits[0] = '0';
        port_len = 1;
    } else {
        let tmp: u16 = port;
        let rev: [5]u8 = [0, 0, 0, 0, 0];
        let rev_len: usize = 0;
        while (tmp > 0) {
            rev[rev_len] = ('0' as u8) + ((tmp % 10) as u8);
            tmp /= 10;
            rev_len += 1;
        }
        let k: usize = 0;
        while (k < rev_len) {
            port_digits[k] = rev[rev_len - 1 - k];
            k += 1;
        }
        port_len = rev_len;
    }

    const total = 1 + display.len + 1 + selector.len + 1 + host.len + 1 + port_len + 2;
    if (out.len < total) {
        return null;
    }

    let pos: usize = 0;
    out[pos] = item_type;
    pos += 1;

    let i: usize = 0;
    while (i < display.len) {
        out[pos] = display[i];
        pos += 1;
        i += 1;
    }
    out[pos] = '\t';
    pos += 1;

    let j: usize = 0;
    while (j < selector.len) {
        out[pos] = selector[j];
        pos += 1;
        j += 1;
    }
    out[pos] = '\t';
    pos += 1;

    let m: usize = 0;
    while (m < host.len) {
        out[pos] = host[m];
        pos += 1;
        m += 1;
    }
    out[pos] = '\t';
    pos += 1;

    let p: usize = 0;
    while (p < port_len) {
        out[pos] = port_digits[p];
        pos += 1;
        p += 1;
    }
    out[pos] = '\r';
    out[pos + 1] = '\n';
    return pos + 2;
}

/// Zero-copy line-by-line iterator over a Gopher Menu response (stops on standalone `.`).
pub struct GopherMenuIter {
    data: []const u8,
    offset: usize,
    terminated: bool,
    failed: bool,

    fn init(menu: []const u8): GopherMenuIter {
        return GopherMenuIter{
            .data = menu,
            .offset = 0,
            .terminated = false,
            .failed = false,
        };
    }

    fn isTerminated(): bool {
        return _.terminated;
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): GopherItem? {
        if (_.terminated || _.failed || _.offset >= _.data.len) {
            return null;
        }

        let end = _.offset;
        while (end < _.data.len && _.data[end] != '\n') {
            end += 1;
        }
        let line_end = end;
        if (line_end > _.offset && _.data[line_end - 1] == '\r') {
            line_end -= 1;
        }
        const raw_line = _.data[_.offset..line_end];
        if (end < _.data.len) {
            _.offset = end + 1;
        } else {
            _.offset = end;
        }

        // Standalone terminating dot "."
        if (raw_line.len == 1 && raw_line[0] == '.') {
            _.terminated = true;
            return null;
        }

        if (item := parseGopherItem(raw_line)) {
            return item;
        }
        _.failed = true;
        return null;
    }
}
