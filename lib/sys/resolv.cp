// Source: Absorbed from /data/dev/bun/src/dns.zig | Synced: 2026-11-21T15:21:05Z (POSIX /etc/resolv.conf Zero-Alloc Parser)
// Pure-CP zero-allocation POSIX /etc/resolv.conf parser (nameserver, search, domain, options).

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return strEq(s[0..prefix.len], prefix);
}

fn parseU32(s: []const u8): u32? {
    if (s.len == 0) return null;
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        if (val > 429496729) return null;
        val = val * 10 + @as(u32, c - '0');
        i += 1;
    }
    return val;
}

fn nextTok(line: []const u8, pos: *usize): ([]const u8)? {
    while (pos.* < line.len && isWs(line[pos.*])) pos.* += 1;
    if (pos.* >= line.len) return null;
    const start = pos.*;
    while (pos.* < line.len && !isWs(line[pos.*])) pos.* += 1;
    return line[start..pos.*];
}

fn stripComment(raw_line: []const u8): []const u8 {
    let end = raw_line.len;
    if (end > 0 && raw_line[end - 1] == '\r') end -= 1;
    let i: usize = 0;
    while (i < end) {
        if (raw_line[i] == '#' || raw_line[i] == ';') {
            return raw_line[0..i];
        }
        i += 1;
    }
    return raw_line[0..end];
}

export struct NameserverIter {
    src: []const u8,
    pos: usize,

    pub fn init(content: []const u8): NameserverIter {
        return NameserverIter { src: content, pos: 0 };
    }

    pub fn next(*_): ([]const u8)? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') _.pos += 1;
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == '\n') _.pos += 1;

            const line = stripComment(_.src[start..end]);
            let p: usize = 0;
            const kw = nextTok(line, &p) ?? continue;
            if (!strEq(kw, "nameserver")) continue;
            if (ip := nextTok(line, &p)) return ip;
        }
        return null;
    }
}

export struct ResolvOptions {
    ndots: u32,
    timeout_sec: u32,
    attempts: u32,
    rotate: bool,
    edns0: bool,
    use_vc: bool,
    trust_ad: bool,

    pub fn defaults(): ResolvOptions {
        return ResolvOptions {
            ndots: 1,
            timeout_sec: 5,
            attempts: 2,
            rotate: false,
            edns0: false,
            use_vc: false,
            trust_ad: false,
        };
    }

    /// Parse all `options ...` lines in `/etc/resolv.conf` (clamping `ndots<=15`, `timeout<=30`, `attempts<=5` per glibc).
    pub fn parse(content: []const u8): ResolvOptions {
        let opts = ResolvOptions.defaults();
        let pos: usize = 0;
        while (pos < content.len) {
            const start = pos;
            while (pos < content.len && content[pos] != '\n') pos += 1;
            const end = pos;
            if (pos < content.len && content[pos] == '\n') pos += 1;

            const line = stripComment(content[start..end]);
            let p: usize = 0;
            const kw = nextTok(line, &p) ?? continue;
            if (!strEq(kw, "options")) continue;

            while (tok := nextTok(line, &p)) {
                if (startsWith(tok, "ndots:")) {
                    if (v := parseU32(tok[6..])) {
                        opts.ndots = if (v > 15) 15 else v;
                    }
                } else if (startsWith(tok, "timeout:")) {
                    if (v := parseU32(tok[8..])) {
                        opts.timeout_sec = if (v > 30) 30 else if (v == 0) 1 else v;
                    }
                } else if (startsWith(tok, "attempts:")) {
                    if (v := parseU32(tok[9..])) {
                        opts.attempts = if (v > 5) 5 else if (v == 0) 1 else v;
                    }
                } else if (strEq(tok, "rotate")) {
                    opts.rotate = true;
                } else if (strEq(tok, "edns0")) {
                    opts.edns0 = true;
                } else if (strEq(tok, "use-vc") || strEq(tok, "usevc")) {
                    opts.use_vc = true;
                } else if (strEq(tok, "trust-ad")) {
                    opts.trust_ad = true;
                }
            }
        }
        return opts;
    }
}

/// Return the effective search list line (last `search` or `domain` directive wins per resolv.conf(5)).
export fn effectiveSearchRaw(content: []const u8): ([]const u8)? {
    let result: ([]const u8)? = null;
    let pos: usize = 0;
    while (pos < content.len) {
        const start = pos;
        while (pos < content.len && content[pos] != '\n') pos += 1;
        const end = pos;
        if (pos < content.len && content[pos] == '\n') pos += 1;

        const line = stripComment(content[start..end]);
        let p: usize = 0;
        const kw = nextTok(line, &p) ?? continue;
        if (strEq(kw, "search") || strEq(kw, "domain")) {
            while (p < line.len && isWs(line[p])) p += 1;
            let tail_end = line.len;
            while (tail_end > p && isWs(line[tail_end - 1])) tail_end -= 1;
            if (tail_end > p) {
                result = line[p..tail_end];
            }
        }
    }
    return result;
}

/// Return the first configured `nameserver` IP, or null if none present.
export fn firstNameserver(content: []const u8): ([]const u8)? {
    let it = NameserverIter.init(content);
    return it.next();
}
