// @cap/resolv-conf — Zero-heap /etc/resolv.conf DNS configuration parser in pure CP
// Clean-room MIT implementation of crates.io `resolv-conf` (#807, 146.5M downloads).

pub const RESOLV_MAX_NS: usize = 8;
pub const RESOLV_MAX_SEARCH: usize = 8;

pub struct ResolvConfig {
    pub nameservers: [8]string,
    pub nameserver_count: usize,
    pub search_domains: [8]string,
    pub search_count: usize,
    pub domain: string,
    pub ndots: u32,
    pub timeout_sec: u32,
    pub attempts: u32,
    pub rotate: bool,
    pub edns0: bool,
    pub use_vc: bool,
    pub trust_ad: bool,
}

fn rcfIsSpace(b: u8): bool {
    return b == 32 || b == 9 || b == 13;
}

fn rcfStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn rcfStartsWith(s: string, prefix: string): bool {
    if (prefix.len > s.len) { return false; }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) { return false; }
        i += 1;
    }
    return true;
}

fn rcfParseU32(s: string): u32? {
    if (s.len == 0) { return null; }
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const b = s[i];
        if (b < 48 || b > 57) { return null; }
        acc = acc * 10 + ((b - 48) as u32);
        i += 1;
    }
    return acc;
}

/// Returns a `ResolvConfig` populated with POSIX defaults (`ndots=1`, `timeout=5`, `attempts=2`).
pub fn defaultResolvConfig(): ResolvConfig {
    return ResolvConfig {
        nameservers: ["", "", "", "", "", "", "", ""],
        nameserver_count: 0,
        search_domains: ["", "", "", "", "", "", "", ""],
        search_count: 0,
        domain: "",
        ndots: 1,
        timeout_sec: 5,
        attempts: 2,
        rotate: false,
        edns0: false,
        use_vc: false,
        trust_ad: false,
    };
}

/// Parses the contents of a `/etc/resolv.conf` file into a `ResolvConfig`.
/// Ignores `#` and `;` comments and blank lines.
pub fn parseResolvConf(content: string): ResolvConfig {
    let cfg = defaultResolvConfig();
    let pos: usize = 0;

    while (pos < content.len) {
        const line_start = pos;
        while (pos < content.len && content[pos] != 10) {
            pos += 1;
        }
        let line = content[line_start..pos];
        if (pos < content.len && content[pos] == 10) {
            pos += 1;
        }

        // Strip inline '#' or ';' comment
        let c_idx: usize = 0;
        while (c_idx < line.len) {
            if (line[c_idx] == 35 || line[c_idx] == 59) {
                line = line[0..c_idx];
                break;
            }
            c_idx += 1;
        }

        // Tokenize whitespace-separated words on this line
        let toks: [16]string = ["", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""];
        let tcount: usize = 0;
        let i: usize = 0;
        while (i < line.len && tcount < 16) {
            while (i < line.len && rcfIsSpace(line[i])) { i += 1; }
            if (i >= line.len) { break; }
            const s0 = i;
            while (i < line.len && !rcfIsSpace(line[i])) { i += 1; }
            toks[tcount] = line[s0..i];
            tcount += 1;
        }

        if (tcount < 2) {
            continue;
        }

        const directive = toks[0];
        if (rcfStrEq(directive, "nameserver")) {
            if (cfg.nameserver_count < RESOLV_MAX_NS) {
                cfg.nameservers[cfg.nameserver_count] = toks[1];
                cfg.nameserver_count += 1;
            }
        } else if (rcfStrEq(directive, "domain")) {
            cfg.domain = toks[1];
            // POSIX: last of domain/search wins; mirror into search_domains
            cfg.search_domains[0] = toks[1];
            cfg.search_count = 1;
        } else if (rcfStrEq(directive, "search")) {
            cfg.search_count = 0;
            let k: usize = 1;
            while (k < tcount && cfg.search_count < RESOLV_MAX_SEARCH) {
                cfg.search_domains[cfg.search_count] = toks[k];
                cfg.search_count += 1;
                k += 1;
            }
        } else if (rcfStrEq(directive, "options")) {
            let k: usize = 1;
            while (k < tcount) {
                const opt = toks[k];
                if (rcfStartsWith(opt, "ndots:")) {
                    const v = rcfParseU32(opt[6..opt.len]);
                    if (v != null) { cfg.ndots = v ?? 1; }
                } else if (rcfStartsWith(opt, "timeout:")) {
                    const v = rcfParseU32(opt[8..opt.len]);
                    if (v != null) { cfg.timeout_sec = v ?? 5; }
                } else if (rcfStartsWith(opt, "attempts:")) {
                    const v = rcfParseU32(opt[9..opt.len]);
                    if (v != null) { cfg.attempts = v ?? 2; }
                } else if (rcfStrEq(opt, "rotate")) {
                    cfg.rotate = true;
                } else if (rcfStrEq(opt, "edns0")) {
                    cfg.edns0 = true;
                } else if (rcfStrEq(opt, "use-vc")) {
                    cfg.use_vc = true;
                } else if (rcfStrEq(opt, "trust-ad")) {
                    cfg.trust_ad = true;
                }
                k += 1;
            }
        }
    }

    return cfg;
}

export {
    RESOLV_MAX_NS,
    RESOLV_MAX_SEARCH,
    ResolvConfig,
    defaultResolvConfig,
    parseResolvConf,
};
