// Origin: Pure-CP Stdlib (Zero-Alloc UPnP / DLNA SSDP Discovery Protocol Codec) | Created: 2026-11-22

// ---------------------------------------------------------------------------
// Protocol Constants (UPnP / DLNA SSDP)
// ---------------------------------------------------------------------------

pub const SSDP_UDP_PORT: u16 = 1900;
pub const SSDP_MULTICAST_V4: []const u8 = "239.255.255.250";
pub const SSDP_MULTICAST_V6_LINK: []const u8 = "ff02::c";
pub const SSDP_MAN_DISCOVER: []const u8 = "\"ssdp:discover\"";
pub const SSDP_ST_ALL: []const u8 = "ssdp:all";
pub const SSDP_ST_ROOTDEVICE: []const u8 = "upnp:rootdevice";
pub const SSDP_NTS_ALIVE: []const u8 = "ssdp:alive";
pub const SSDP_NTS_BYEBYE: []const u8 = "ssdp:byebye";
pub const SSDP_NTS_UPDATE: []const u8 = "ssdp:update";

pub enum SsdpKind: u8 {
    m_search = 1,
    notify = 2,
    response = 3,
}

// ---------------------------------------------------------------------------
// Internal ASCII & Buffer Helpers
// ---------------------------------------------------------------------------

fn toLowerAscii(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') {
        return b + 32;
    }
    return b;
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

fn bytesEq(a: []const u8, b: []const u8): bool {
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

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return bytesEq(s[0..prefix.len], prefix);
}

fn isSpaceOrTab(b: u8): bool {
    return b == ' ' || b == '\t';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpaceOrTab(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isSpaceOrTab(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn hasControlOrCrLf(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const b = s[i];
        if (b == '\r' || b == '\n' || b == 0) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn parseU32Decimal(s: []const u8): u32? {
    const trimmed = trimAscii(s);
    if (trimmed.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < trimmed.len) {
        const b = trimmed[i];
        if (b < '0' || b > '9') {
            return null;
        }
        acc = acc * 10 + ((b - '0') as u64);
        if (acc > 4294967295) {
            return null;
        }
        i += 1;
    }
    return acc as u32;
}

fn parseMaxAgeDirective(cache_control: []const u8): u32? {
    let pos: usize = 0;
    while (pos < cache_control.len) {
        const seg_start = pos;
        while (pos < cache_control.len && cache_control[pos] != ',') {
            pos += 1;
        }
        const seg = trimAscii(cache_control[seg_start..pos]);
        if (pos < cache_control.len && cache_control[pos] == ',') {
            pos += 1;
        }
        // Check for "max-age=..."
        let eq_pos: usize? = null;
        let i: usize = 0;
        while (i < seg.len) {
            if (seg[i] == '=') {
                eq_pos = i;
                break;
            }
            i += 1;
        }
        if (eq := eq_pos) {
            const key = trimAscii(seg[0..eq]);
            if (eqIgnoreCase(key, "max-age")) {
                return parseU32Decimal(seg[eq + 1..]);
            }
        }
    }
    return null;
}

struct BufCursor {
    buf: []u8,
    pos: usize,

    fn writeSlice(*_, s: []const u8): bool {
        if (_.buf.len - _.pos < s.len) {
            return false;
        }
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.pos + i] = s[i];
            i += 1;
        }
        _.pos += s.len;
        return true;
    }

    fn writeU32(*_, val: u32): bool {
        let tmp: [10]u8 = [0; 10];
        let n: usize = 0;
        let v = val;
        if (v == 0) {
            tmp[0] = '0';
            n = 1;
        } else {
            while (v > 0) {
                tmp[n] = ('0' as u8) + ((v % 10) as u8);
                v = v / 10;
                n += 1;
            }
        }
        if (_.buf.len - _.pos < n) {
            return false;
        }
        let i: usize = 0;
        while (i < n) {
            _.buf[_.pos + i] = tmp[n - 1 - i];
            i += 1;
        }
        _.pos += n;
        return true;
    }
}

// ---------------------------------------------------------------------------
// SSDP Headers & Message Models
// ---------------------------------------------------------------------------

/// A single trimmed zero-copy SSDP HTTP-like header (`name: value`).
pub struct SsdpHeader {
    name: []const u8,
    value: []const u8,

    fn nameEqualsIgnoreCase(target: []const u8): bool {
        return eqIgnoreCase(_.name, target);
    }
}

/// Zero-allocation line-by-line iterator over SSDP headers, terminating at the blank line (`\r\n\r\n` or `\n\n`).
pub struct SsdpHeaderIter {
    data: []const u8,
    offset: usize,
    done: bool,

    static fn init(headers_bytes: []const u8): SsdpHeaderIter {
        return SsdpHeaderIter {
            data: headers_bytes,
            offset: 0,
            done: false,
        };
    }

    fn next(*_): SsdpHeader? {
        if (_.done) {
            return null;
        }
        while (_.offset < _.data.len) {
            const line_start = _.offset;
            while (_.offset < _.data.len && _.data[_.offset] != '\n') {
                _.offset += 1;
            }
            let line_end = _.offset;
            if (_.offset < _.data.len && _.data[_.offset] == '\n') {
                _.offset += 1;
            }
            if (line_end > line_start && _.data[line_end - 1] == '\r') {
                line_end -= 1;
            }

            // Blank line terminates HTTP/SSDP header block
            if (line_end == line_start) {
                _.done = true;
                return null;
            }

            const line = _.data[line_start..line_end];
            let colon_pos: usize? = null;
            let i: usize = 0;
            while (i < line.len) {
                if (line[i] == ':') {
                    colon_pos = i;
                    break;
                }
                i += 1;
            }
            if (colon := colon_pos) {
                const name = trimAscii(line[0..colon]);
                if (name.len == 0) {
                    continue;
                }
                const value = trimAscii(line[colon + 1..]);
                return SsdpHeader {
                    name: name,
                    value: value,
                };
            }
        }
        _.done = true;
        return null;
    }
}

/// Zero-copy view of a parsed SSDP discovery message (`M-SEARCH`, `NOTIFY`, or `HTTP/1.1` response).
pub struct SsdpMessage {
    kind: SsdpKind,
    status_code: u16,
    headers_bytes: []const u8,

    fn headers(): SsdpHeaderIter {
        return SsdpHeaderIter.init(_.headers_bytes);
    }

    /// Case-insensitive ASCII lookup for the first header matching `name`.
    fn findHeader(name: []const u8): ([]const u8)? {
        let it = _.headers();
        while (hdr := it.next()) {
            if (eqIgnoreCase(hdr.name, name)) {
                return hdr.value;
            }
        }
        return null;
    }

    fn st(): ([]const u8)? {
        return _.findHeader("ST");
    }

    fn nt(): ([]const u8)? {
        return _.findHeader("NT");
    }

    fn nts(): ([]const u8)? {
        return _.findHeader("NTS");
    }

    fn usn(): ([]const u8)? {
        return _.findHeader("USN");
    }

    fn location(): ([]const u8)? {
        return _.findHeader("LOCATION");
    }

    fn server(): ([]const u8)? {
        return _.findHeader("SERVER");
    }

    fn man(): ([]const u8)? {
        return _.findHeader("MAN");
    }

    fn mx(): u32? {
        if (v := _.findHeader("MX")) {
            return parseU32Decimal(v);
        }
        return null;
    }

    /// Extracts the `max-age=<seconds>` directive from `CACHE-CONTROL`.
    fn maxAge(): u32? {
        if (cc := _.findHeader("CACHE-CONTROL")) {
            return parseMaxAgeDirective(cc);
        }
        return null;
    }

    fn isAlive(): bool {
        if (v := _.nts()) {
            return eqIgnoreCase(v, SSDP_NTS_ALIVE);
        }
        return false;
    }

    fn isByebye(): bool {
        if (v := _.nts()) {
            return eqIgnoreCase(v, SSDP_NTS_BYEBYE);
        }
        return false;
    }
}

// ---------------------------------------------------------------------------
// SSDP Message Parser
// ---------------------------------------------------------------------------

/// Parses an SSDP datagram (`M-SEARCH * HTTP/1.1`, `NOTIFY * HTTP/1.1`, or `HTTP/1.1 <NNN> ...`).
/// Rejects empty datagrams, missing line terminators, or unrecognized start lines.
pub fn parseSsdp(data: []const u8): SsdpMessage? {
    if (data.len == 0) {
        return null;
    }

    // Locate end of start line
    let lf_pos: usize? = null;
    let i: usize = 0;
    while (i < data.len) {
        if (data[i] == '\n') {
            lf_pos = i;
            break;
        }
        i += 1;
    }
    const lf = lf_pos ?? return null;
    let line_end = lf;
    if (line_end > 0 && data[line_end - 1] == '\r') {
        line_end -= 1;
    }
    const start_line = data[0..line_end];
    const rest = data[lf + 1..];

    if (bytesEq(start_line, "M-SEARCH * HTTP/1.1") || bytesEq(start_line, "M-SEARCH * HTTP/1.0")) {
        return SsdpMessage {
            kind: SsdpKind.m_search,
            status_code: 0,
            headers_bytes: rest,
        };
    }

    if (bytesEq(start_line, "NOTIFY * HTTP/1.1") || bytesEq(start_line, "NOTIFY * HTTP/1.0")) {
        return SsdpMessage {
            kind: SsdpKind.notify,
            status_code: 0,
            headers_bytes: rest,
        };
    }

    if (startsWith(start_line, "HTTP/1.1 ") || startsWith(start_line, "HTTP/1.0 ")) {
        if (start_line.len < 12) {
            return null;
        }
        const c0 = start_line[9];
        const c1 = start_line[10];
        const c2 = start_line[11];
        if (c0 < '0' || c0 > '9' || c1 < '0' || c1 > '9' || c2 < '0' || c2 > '9') {
            return null;
        }
        if (start_line.len > 12 && start_line[12] != ' ') {
            return null;
        }
        const code: u16 = ((c0 - '0') as u16) * 100 + ((c1 - '0') as u16) * 10 + ((c2 - '0') as u16);
        return SsdpMessage {
            kind: SsdpKind.response,
            status_code: code,
            headers_bytes: rest,
        };
    }

    return null;
}

/// Fast-path check whether `data` is a valid SSDP message.
pub fn isSsdp(data: []const u8): bool {
    return parseSsdp(data) != null;
}

// ---------------------------------------------------------------------------
// Zero-Alloc SSDP Encoders (with CRLF/NUL Injection Guards)
// ---------------------------------------------------------------------------

/// Encodes an SSDP `M-SEARCH * HTTP/1.1` discovery request into `out`.
/// Rejects empty `st`, CRLF/NUL injection in `st`, or insufficient `out` buffer.
pub fn encodeSsdpMSearch(st: []const u8, mx: u32, out: []u8): usize? {
    if (st.len == 0 || hasControlOrCrLf(st)) {
        return null;
    }
    let cur = BufCursor { buf: out, pos: 0 };
    if (!cur.writeSlice("M-SEARCH * HTTP/1.1\r\n")) return null;
    if (!cur.writeSlice("HOST: 239.255.255.250:1900\r\n")) return null;
    if (!cur.writeSlice("MAN: \"ssdp:discover\"\r\n")) return null;
    if (!cur.writeSlice("MX: ")) return null;
    if (!cur.writeU32(mx)) return null;
    if (!cur.writeSlice("\r\nST: ")) return null;
    if (!cur.writeSlice(st)) return null;
    if (!cur.writeSlice("\r\n\r\n")) return null;
    return cur.pos;
}

/// Encodes an SSDP `NOTIFY * HTTP/1.1` announcement (`ssdp:alive` / `ssdp:byebye` / `ssdp:update`) into `out`.
/// Rejects empty `nt`/`nts`/`usn`, CRLF/NUL injection in any field, or insufficient `out` buffer.
pub fn encodeSsdpNotify(
    nt: []const u8,
    nts: []const u8,
    usn: []const u8,
    location: []const u8,
    max_age: u32,
    out: []u8,
): usize? {
    if (nt.len == 0 || nts.len == 0 || usn.len == 0) {
        return null;
    }
    if (hasControlOrCrLf(nt) || hasControlOrCrLf(nts) || hasControlOrCrLf(usn) || hasControlOrCrLf(location)) {
        return null;
    }
    let cur = BufCursor { buf: out, pos: 0 };
    if (!cur.writeSlice("NOTIFY * HTTP/1.1\r\n")) return null;
    if (!cur.writeSlice("HOST: 239.255.255.250:1900\r\n")) return null;
    if (max_age > 0) {
        if (!cur.writeSlice("CACHE-CONTROL: max-age=")) return null;
        if (!cur.writeU32(max_age)) return null;
        if (!cur.writeSlice("\r\n")) return null;
    }
    if (location.len > 0) {
        if (!cur.writeSlice("LOCATION: ")) return null;
        if (!cur.writeSlice(location)) return null;
        if (!cur.writeSlice("\r\n")) return null;
    }
    if (!cur.writeSlice("NT: ")) return null;
    if (!cur.writeSlice(nt)) return null;
    if (!cur.writeSlice("\r\nNTS: ")) return null;
    if (!cur.writeSlice(nts)) return null;
    if (!cur.writeSlice("\r\nUSN: ")) return null;
    if (!cur.writeSlice(usn)) return null;
    if (!cur.writeSlice("\r\n\r\n")) return null;
    return cur.pos;
}
