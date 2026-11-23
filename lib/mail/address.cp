// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (RFC 5322 Email Mailbox & Address List Parser)
// Pure-CP zero-allocation RFC 5322 email mailbox ("Display Name" <local@domain>) parser, validator, and list iterator.

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) start += 1;
    let end: usize = s.len;
    while (end > start && isWs(s[end - 1])) end -= 1;
    return s[start..end];
}

fn isAtext(b: u8): bool {
    if ((b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9')) return true;
    return b == '!' || b == '#' || b == '$' || b == '%' || b == '&' || b == '\'' ||
        b == '*' || b == '+' || b == '-' || b == '/' || b == '=' || b == '?' ||
        b == '^' || b == '_' || b == '`' || b == '{' || b == '|' || b == '}' || b == '~';
}

/// Validate a bare email address `local@domain` (RFC 5322 dot-atom local part + RFC 1035 domain).
export fn isValidAddrSpec(addr: []const u8): bool {
    if (addr.len < 3 || addr.len > 254) return false;
    let at_pos: usize? = null;
    let i: usize = 0;
    while (i < addr.len) {
        if (addr[i] == '@') {
            if (at_pos != null) return false;
            at_pos = i;
        }
        i += 1;
    }
    const ap = at_pos ?? return false;
    const local = addr[0..ap];
    const domain = addr[ap + 1..];
    if (local.len == 0 || local.len > 64 || domain.len == 0 || domain.len > 253) return false;

    // Local part: dot-atom (no leading/trailing/consecutive '.')
    if (local[0] == '.' || local[local.len - 1] == '.') return false;
    let prev_dot = false;
    let li: usize = 0;
    while (li < local.len) {
        const c = local[li];
        if (c == '.') {
            if (prev_dot) return false;
            prev_dot = true;
        } else if (isAtext(c)) {
            prev_dot = false;
        } else {
            return false;
        }
        li += 1;
    }

    // Domain part: valid DNS labels separated by '.'
    if (domain[0] == '.' || domain[domain.len - 1] == '.') return false;
    let lstart: usize = 0;
    let di: usize = 0;
    while (di <= domain.len) {
        if (di == domain.len || domain[di] == '.') {
            const label = domain[lstart..di];
            if (label.len == 0 || label.len > 63) return false;
            if (label[0] == '-' || label[label.len - 1] == '-') return false;
            let k: usize = 0;
            while (k < label.len) {
                const ch = label[k];
                const ok = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                    (ch >= '0' && ch <= '9') || ch == '-';
                if (!ok) return false;
                k += 1;
            }
            lstart = di + 1;
        }
        di += 1;
    }
    return true;
}

export struct Mailbox {
    display_name: ([]const u8)?,
    address: []const u8,
    local_part: []const u8,
    domain: []const u8,

    /// Parse a single RFC 5322 mailbox: `user@example.com`, `<user@example.com>`,
    /// `Alice Smith <alice@example.com>`, or `"Smith, Alice" <alice@example.com>`.
    pub fn parse(raw: []const u8): Mailbox? {
        const s = trimWs(raw);
        if (s.len == 0) return null;

        let lt_pos: usize? = null;
        let gt_pos: usize? = null;
        let in_quote = false;
        let escaped = false;
        let i: usize = 0;
        while (i < s.len) {
            const c = s[i];
            if (in_quote) {
                if (escaped) {
                    escaped = false;
                } else if (c == '\\') {
                    escaped = true;
                } else if (c == '"') {
                    in_quote = false;
                }
            } else {
                if (c == '"') {
                    in_quote = true;
                } else if (c == '<' && lt_pos == null) {
                    lt_pos = i;
                } else if (c == '>' && lt_pos != null && gt_pos == null) {
                    gt_pos = i;
                }
            }
            i += 1;
        }
        if (in_quote) return null;

        let display: ([]const u8)? = null;
        let addr: []const u8 = s;

        if (lt_pos != null || gt_pos != null) {
            if (lt_pos == null || gt_pos == null) return null;
            const lp = lt_pos.?;
            const gp = gt_pos.?;
            if (gp <= lp + 1) return null;
            // Nothing non-whitespace after '>'
            if (trimWs(s[gp + 1..]).len > 0) return null;
            addr = trimWs(s[lp + 1..gp]);
            let dn = trimWs(s[0..lp]);
            if (dn.len >= 2 && dn[0] == '"' && dn[dn.len - 1] == '"') {
                dn = dn[1..dn.len - 1];
            }
            if (dn.len > 0) display = dn;
        }

        if (!isValidAddrSpec(addr)) return null;
        let at_idx: usize = 0;
        while (at_idx < addr.len && addr[at_idx] != '@') at_idx += 1;

        return Mailbox {
            display_name: display,
            address: addr,
            local_part: addr[0..at_idx],
            domain: addr[at_idx + 1..],
        };
    }
}

/// Zero-copy iterator over a comma-separated RFC 5322 mailbox list (respects `"..."` quotes and `<...>`).
export struct MailboxListIter {
    src: []const u8,
    pos: usize,

    pub fn init(header: []const u8): MailboxListIter {
        return MailboxListIter { src: header, pos: 0 };
    }

    pub fn next(*_): Mailbox? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            let in_quote = false;
            let escaped = false;
            while (_.pos < _.src.len) {
                const c = _.src[_.pos];
                if (in_quote) {
                    if (escaped) {
                        escaped = false;
                    } else if (c == '\\') {
                        escaped = true;
                    } else if (c == '"') {
                        in_quote = false;
                    }
                } else {
                    if (c == '"') {
                        in_quote = true;
                    } else if (c == ',') {
                        break;
                    }
                }
                _.pos += 1;
            }
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == ',') _.pos += 1;
            const seg = trimWs(_.src[start..end]);
            if (seg.len == 0) continue;
            if (mb := Mailbox.parse(seg)) return mb;
        }
        return null;
    }
}
