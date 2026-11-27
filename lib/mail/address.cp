// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (RFC 5322 Email Mailbox & Address List Parser)
// Pure-CP zero-allocation RFC 5322 email mailbox ("Display Name" <local@domain>) parser, validator, and list iterator.

type MailAddrVec16 = @Vector(16, u8);

const MAIL_ATEXT: u8 = 1;
const MAIL_DNS: u8 = 2;

fn makeMailCharLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        const b = @as(u8, @intCast(i));
        let m: u8 = 0;
        const is_alnum = (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9');
        if (is_alnum || b == '-') {
            m |= MAIL_DNS;
        }
        if (is_alnum || b == '!' || b == '#' || b == '$' || b == '%' || b == '&' || b == '\'' ||
            b == '*' || b == '+' || b == '-' || b == '/' || b == '=' || b == '?' ||
            b == '^' || b == '_' || b == '`' || b == '{' || b == '|' || b == '}' || b == '~') {
            m |= MAIL_ATEXT;
        }
        lut[i] = m;
    }
    return lut;
}

const MAIL_CHAR_LUT: [u8; 256] = makeMailCharLut();

fn mailSimdFindByte(s: []const u8, start: usize, needle: u8): usize {
    let i = start;
    const v_n: MailAddrVec16 = @splat(needle);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: MailAddrVec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast(chunk == v_n);
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == needle) return i;
    }
    return s.len;
}

fn mailSimdFindAny2(s: []const u8, start: usize, a: u8, b: u8): usize {
    let i = start;
    const va: MailAddrVec16 = @splat(a);
    const vb: MailAddrVec16 = @splat(b);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: MailAddrVec16 = s[i .. i + 16][0..16].*;
        const ma: u16 = @bitCast(chunk == va);
        const mb: u16 = @bitCast(chunk == vb);
        const mask: u16 = ma | mb;
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == a || s[i] == b) return i;
    }
    return s.len;
}

fn mailSimdFindAny3(s: []const u8, start: usize, a: u8, b: u8, c: u8): usize {
    let i = start;
    const va: MailAddrVec16 = @splat(a);
    const vb: MailAddrVec16 = @splat(b);
    const vc: MailAddrVec16 = @splat(c);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: MailAddrVec16 = s[i .. i + 16][0..16].*;
        const ma: u16 = @bitCast(chunk == va);
        const mb: u16 = @bitCast(chunk == vb);
        const mc: u16 = @bitCast(chunk == vc);
        const mask: u16 = ma | mb | mc;
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == a || s[i] == b || s[i] == c) return i;
    }
    return s.len;
}

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
    return (MAIL_CHAR_LUT[usize(b)] & MAIL_ATEXT) != 0;
}

/// Validate a bare email address `local@domain` (RFC 5322 dot-atom local part + RFC 1035 domain).
export fn isValidAddrSpec(addr: []const u8): bool {
    if (addr.len < 3 || addr.len > 254) return false;
    const ap = mailSimdFindByte(addr, 0, '@');
    if (ap >= addr.len) return false;
    // Reject multiple '@'
    if (mailSimdFindByte(addr, ap + 1, '@') < addr.len) return false;

    const local = addr[0..ap];
    const domain = addr[ap + 1..];
    if (local.len == 0 || local.len > 64 || domain.len == 0 || domain.len > 253) return false;

    // Local part: dot-atom (no leading/trailing/consecutive '.')
    if (local[0] == '.' || local[local.len - 1] == '.') return false;
    let prev_dot = false;
    let li: usize = 0;
    while (li < local.len) : (li += 1) {
        const c = local[li];
        if (c == '.') {
            if (prev_dot) return false;
            prev_dot = true;
        } else if (isAtext(c)) {
            prev_dot = false;
        } else {
            return false;
        }
    }

    // Domain part: valid DNS labels separated by '.'
    if (domain[0] == '.' || domain[domain.len - 1] == '.') return false;
    let lstart: usize = 0;
    while (lstart <= domain.len) {
        const dot_pos = mailSimdFindByte(domain, lstart, '.');
        const label = domain[lstart..dot_pos];
        if (label.len == 0 || label.len > 63) return false;
        if (label[0] == '-' || label[label.len - 1] == '-') return false;
        let k: usize = 0;
        while (k < label.len) : (k += 1) {
            if ((MAIL_CHAR_LUT[usize(label[k])] & MAIL_DNS) == 0) return false;
        }
        if (dot_pos >= domain.len) break;
        lstart = dot_pos + 1;
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
    pub static fn parse(raw: []const u8): Mailbox? {
        const s = trimWs(raw);
        if (s.len == 0) return null;

        let lt_pos: usize? = null;
        let gt_pos: usize? = null;
        let in_quote = false;
        let escaped = false;
        let i: usize = 0;
        while (i < s.len) {
            if (in_quote) {
                if (escaped) {
                    escaped = false;
                    i += 1;
                } else {
                    i = mailSimdFindAny2(s, i, '\\', '"');
                    if (i >= s.len) break;
                    if (s[i] == '\\') {
                        escaped = true;
                    } else {
                        in_quote = false;
                    }
                    i += 1;
                }
            } else {
                i = mailSimdFindAny3(s, i, '"', '<', '>');
                if (i >= s.len) break;
                const c = s[i];
                if (c == '"') {
                    in_quote = true;
                } else if (c == '<' && lt_pos == null) {
                    lt_pos = i;
                } else if (c == '>' && lt_pos != null && gt_pos == null) {
                    gt_pos = i;
                }
                i += 1;
            }
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
        const at_idx = mailSimdFindByte(addr, 0, '@');

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

    pub static fn init(header: []const u8): MailboxListIter {
        return MailboxListIter { src: header, pos: 0 };
    }

    pub fn next(*_): Mailbox? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            let in_quote = false;
            let escaped = false;
            while (_.pos < _.src.len) {
                if (in_quote) {
                    if (escaped) {
                        escaped = false;
                        _.pos += 1;
                    } else {
                        _.pos = mailSimdFindAny2(_.src, _.pos, '\\', '"');
                        if (_.pos >= _.src.len) break;
                        if (_.src[_.pos] == '\\') {
                            escaped = true;
                        } else {
                            in_quote = false;
                        }
                        _.pos += 1;
                    }
                } else {
                    _.pos = mailSimdFindAny2(_.src, _.pos, '"', ',');
                    if (_.pos >= _.src.len) break;
                    if (_.src[_.pos] == '"') {
                        in_quote = true;
                        _.pos += 1;
                    } else {
                        break;
                    }
                }
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
