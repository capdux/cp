// @cap/hosts-file — Zero-allocation POSIX/Windows /etc/hosts parser & lookup engine for CP
// Clean-room MIT implementation inspired by npm `hostile` and Rust `hostsfile`.
//
// Features:
// - Parses standard `/etc/hosts` and Windows `drivers/etc/hosts` files (LF & CRLF).
// - Strips full-line and inline `#` comments, expands multi-alias lines into individual `HostEntry` items.
// - Supports IPv4 and IPv6 addresses (including `::1` and scoped `fe80::1%lo0`).
// - Case-insensitive forward lookup (`lookupIp`, `lookupIpByFamily`), reverse lookup (`reverseLookup`),
//   and aligned canonical formatter (`formatHosts`).
// - Zero heap allocation: all `HostEntry` strings slice directly into the original input.

const MAX_HOST_ENTRIES: usize = 64;

pub struct HostEntry {
    pub ip: string,
    pub hostname: string,
    pub is_ipv6: bool,

    pub static fn empty() -> HostEntry {
        return HostEntry{ ip: "", hostname: "", is_ipv6: false };
    }
}

pub struct HostsTable {
    pub entries: [MAX_HOST_ENTRIES]HostEntry,
    pub count: usize,

    pub static fn empty() -> HostsTable {
        let arr: [MAX_HOST_ENTRIES]HostEntry = undefined;
        return HostsTable{ entries: arr, count: 0 };
    }

    pub fn entryAt(i: usize) -> HostEntry {
        if (i >= _.count) {
            return HostEntry.empty();
        }
        return _.entries[i];
    }
}

fn isBlank(c: u8) -> bool {
    return c == ' ' || c == '\t' || c == '\r';
}

fn toLowerAscii(c: u8) -> u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqIgnoreCase(a: string, b: string) -> bool {
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

fn containsChar(s: string, ch: u8) -> bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Parses `/etc/hosts` content into a zero-allocation `HostsTable`.
pub fn parseHosts(content: string) -> HostsTable {
    let table = HostsTable.empty();
    let cursor: usize = 0;

    while (cursor < content.len) {
        const ls = cursor;
        while (cursor < content.len && content[cursor] != '\n') {
            cursor += 1;
        }
        let le = cursor;
        if (le > ls && content[le - 1] == '\r') {
            le -= 1;
        }
        if (cursor < content.len && content[cursor] == '\n') {
            cursor += 1;
        }

        // Strip inline `#` comment
        let eff_end = ls;
        while (eff_end < le && content[eff_end] != '#') {
            eff_end += 1;
        }

        // Parse IP token
        let p = ls;
        while (p < eff_end && isBlank(content[p])) {
            p += 1;
        }
        if (p >= eff_end) {
            continue;
        }
        const ip_start = p;
        while (p < eff_end && !isBlank(content[p])) {
            p += 1;
        }
        const ip = content[ip_start..p];
        const is_v6 = containsChar(ip, ':');
        const is_v4 = containsChar(ip, '.');
        if (!is_v6 && !is_v4) {
            continue;
        }

        // Expand all hostnames / aliases on the same line
        while (p < eff_end) {
            while (p < eff_end && isBlank(content[p])) {
                p += 1;
            }
            if (p >= eff_end) {
                break;
            }
            const h_start = p;
            while (p < eff_end && !isBlank(content[p])) {
                p += 1;
            }
            const host = content[h_start..p];
            if (host.len > 0 && table.count < MAX_HOST_ENTRIES) {
                table.entries[table.count] = HostEntry{
                    ip: ip,
                    hostname: host,
                    is_ipv6: is_v6,
                };
                table.count += 1;
            }
        }
    }

    return table;
}

/// Looks up the first IP address matching `hostname` (case-insensitive).
pub fn lookupIp(table: HostsTable, hostname: string) -> string? {
    let i: usize = 0;
    while (i < table.count) {
        if (eqIgnoreCase(table.entries[i].hostname, hostname)) {
            return table.entries[i].ip;
        }
        i += 1;
    }
    return null;
}

/// Looks up the first IP address matching `hostname` (case-insensitive) for the specified IP family (`ipv6`).
pub fn lookupIpByFamily(table: HostsTable, hostname: string, ipv6: bool) -> string? {
    let i: usize = 0;
    while (i < table.count) {
        if (table.entries[i].is_ipv6 == ipv6 && eqIgnoreCase(table.entries[i].hostname, hostname)) {
            return table.entries[i].ip;
        }
        i += 1;
    }
    return null;
}

/// Reverse-looks up the primary (first) hostname mapped to `ip` (case-insensitive for IPv6).
pub fn reverseLookup(table: HostsTable, ip: string) -> string? {
    let i: usize = 0;
    while (i < table.count) {
        if (eqIgnoreCase(table.entries[i].ip, ip)) {
            return table.entries[i].hostname;
        }
        i += 1;
    }
    return null;
}

fn pushByte(out: []u8, pos: usize, b: u8) -> usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn pushStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

/// Formats `table` into canonical aligned `/etc/hosts` syntax in caller-provided `out` (zero heap allocation).
/// Consecutive entries sharing the same IP are grouped on a single line as aliases.
pub fn formatHosts(table: HostsTable, out: []u8) -> string {
    if (table.count == 0) {
        return out[0..0];
    }

    // Compute aligned IP column width (minimum 16 columns)
    let col_w: usize = 16;
    let k: usize = 0;
    while (k < table.count) {
        if (table.entries[k].ip.len + 2 > col_w) {
            col_w = table.entries[k].ip.len + 2;
        }
        k += 1;
    }

    let pos: usize = 0;
    let i: usize = 0;
    while (i < table.count) {
        if (i > 0) {
            pos = pushByte(out, pos, '\n');
        }
        const cur_ip = table.entries[i].ip;
        pos = pushStr(out, pos, cur_ip);

        const pad = if (col_w > cur_ip.len) { col_w - cur_ip.len } else { 1 };
        let sp: usize = 0;
        while (sp < pad) {
            pos = pushByte(out, pos, ' ');
            sp += 1;
        }

        pos = pushStr(out, pos, table.entries[i].hostname);
        i += 1;

        // Group consecutive aliases sharing the same IP onto one line
        while (i < table.count && eqIgnoreCase(table.entries[i].ip, cur_ip)) {
            pos = pushByte(out, pos, ' ');
            pos = pushStr(out, pos, table.entries[i].hostname);
            i += 1;
        }
    }

    return out[0..pos];
}
