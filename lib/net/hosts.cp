// Source: Absorbed from /data/dev/bun/src/dns.zig | Synced: 2026-11-21T14:36:08Z (/etc/hosts Parser & Resolver Lookup)
// Pure-CP zero-allocation /etc/hosts parser supporting IPv4/IPv6, multiple aliases, and comments.

fn isSpaceOrTab(b: u8): bool {
    return b == ' ' || b == '\t';
}

fn toLowerAscii(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') return b + 32;
    return b;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn nextToken(line: []const u8, pos: *usize): ([]const u8)? {
    while (pos.* < line.len && isSpaceOrTab(line[pos.*])) {
        pos.* += 1;
    }
    if (pos.* >= line.len) return null;
    const start = pos.*;
    while (pos.* < line.len && !isSpaceOrTab(line[pos.*])) {
        pos.* += 1;
    }
    return line[start..pos.*];
}

fn isIpv6Token(ip: []const u8): bool {
    let i: usize = 0;
    while (i < ip.len) {
        if (ip[i] == ':') return true;
        i += 1;
    }
    return false;
}

export struct HostsLine {
    ip: []const u8,
    canonical_host: []const u8,
    aliases_rest: []const u8,
    is_ipv6: bool,

    /// Check if this hosts entry matches `hostname` (canonical name or any alias, case-insensitive).
    pub fn matchesHost(hostname: []const u8): bool {
        if (hostname.len == 0) return false;
        if (eqIgnoreCase(_.canonical_host, hostname)) return true;
        let pos: usize = 0;
        while (tok := nextToken(_.aliases_rest, &pos)) {
            if (eqIgnoreCase(tok, hostname)) return true;
        }
        return false;
    }

    /// Iterate over all hostnames (canonical + aliases) on this line.
    pub fn hostCount(): usize {
        let count: usize = 1;
        let pos: usize = 0;
        while (nextToken(_.aliases_rest, &pos) != null) {
            count += 1;
        }
        return count;
    }
}

export struct HostsIter {
    src: []const u8,
    pos: usize,

    pub fn init(content: []const u8): HostsIter {
        let start: usize = 0;
        if (content.len >= 3 && content[0] == 0xEF && content[1] == 0xBB && content[2] == 0xBF) {
            start = 3;
        }
        return HostsIter { src: content, pos: start };
    }

    pub fn next(*_): HostsLine? {
        while (_.pos < _.src.len) {
            const line_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                _.pos += 1;
            }
            let line_end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                _.pos += 1;
            }
            if (line_end > line_start && _.src[line_end - 1] == '\r') {
                line_end -= 1;
            }
            let line = _.src[line_start..line_end];

            // Strip '#' comment
            let ci: usize = 0;
            while (ci < line.len) {
                if (line[ci] == '#') {
                    line = line[0..ci];
                    break;
                }
                ci += 1;
            }

            let tpos: usize = 0;
            const ip = nextToken(line, &tpos) ?? continue;
            const host = nextToken(line, &tpos) ?? continue;
            const rest = line[tpos..];
            return HostsLine {
                ip: ip,
                canonical_host: host,
                aliases_rest: rest,
                is_ipv6: isIpv6Token(ip),
            };
        }
        return null;
    }
}

/// Lookup the first IP address (IPv4 or IPv6) mapped to `hostname`.
export fn lookupHost(content: []const u8, hostname: []const u8): ([]const u8)? {
    let it = HostsIter.init(content);
    while (entry := it.next()) {
        if (entry.matchesHost(hostname)) return entry.ip;
    }
    return null;
}

/// Lookup the first IPv4 address mapped to `hostname`.
export fn lookupHostV4(content: []const u8, hostname: []const u8): ([]const u8)? {
    let it = HostsIter.init(content);
    while (entry := it.next()) {
        if (!entry.is_ipv6 && entry.matchesHost(hostname)) return entry.ip;
    }
    return null;
}

/// Lookup the first IPv6 address mapped to `hostname`.
export fn lookupHostV6(content: []const u8, hostname: []const u8): ([]const u8)? {
    let it = HostsIter.init(content);
    while (entry := it.next()) {
        if (entry.is_ipv6 && entry.matchesHost(hostname)) return entry.ip;
    }
    return null;
}

/// Reverse-lookup the canonical hostname for a given IP string (case-insensitive for IPv6 hex).
export fn reverseLookup(content: []const u8, ip: []const u8): ([]const u8)? {
    let it = HostsIter.init(content);
    while (entry := it.next()) {
        if (eqIgnoreCase(entry.ip, ip)) return entry.canonical_host;
    }
    return null;
}
