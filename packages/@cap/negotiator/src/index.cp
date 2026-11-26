// @cap/negotiator — Zero-heap RFC 7231 Section 5.3 HTTP content negotiation in pure CP
// MIT License — Clean-room implementation inspired by npm `negotiator`

pub const SPEC_WILDCARD: u8 = 1;
pub const SPEC_PARTIAL: u8 = 2;
pub const SPEC_EXACT: u8 = 4;

pub struct NegSpec {
    pub start: usize,
    pub len: usize,
    pub q_milli: u16,
    pub specificity: u8,
    pub order: usize,
}

fn isSpace(ch: u8): bool {
    return ch == 32 || ch == 9 || ch == 13 || ch == 10;
}

fn toLower(ch: u8): u8 {
    if (ch >= 65 && ch <= 90) {
        return ch + 32;
    }
    return ch;
}

fn eqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLower(a[i]) != toLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn findByte(s: string, ch: u8): i64 {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) {
            return i as i64;
        }
        i += 1;
    }
    return -1;
}

fn trimStart(s: string, lo: usize, hi: usize): usize {
    let i: usize = lo;
    while (i < hi && isSpace(s[i])) {
        i += 1;
    }
    return i;
}

fn trimEnd(s: string, lo: usize, hi: usize): usize {
    let j: usize = hi;
    while (j > lo && isSpace(s[j - 1])) {
        j -= 1;
    }
    return j;
}

/// Parse a quality value string (e.g. "1", "0.85", "0.125", "0") into 0..1000 milli-units.
fn parseQMilli(val: string): u16 {
    if (val.len == 0) {
        return 1000;
    }
    let i: usize = 0;
    let whole: u32 = 0;
    let saw_digit: bool = false;
    while (i < val.len && val[i] >= 48 && val[i] <= 57) {
        whole = whole * 10 + ((val[i] - 48) as u32);
        saw_digit = true;
        i += 1;
    }
    let frac: u32 = 0;
    let frac_scale: u32 = 100;
    if (i < val.len && val[i] == 46) { // '.'
        i += 1;
        while (i < val.len && val[i] >= 48 && val[i] <= 57) {
            saw_digit = true;
            if (frac_scale > 0) {
                frac = frac + ((val[i] - 48) as u32) * frac_scale;
                frac_scale = frac_scale / 10;
            }
            i += 1;
        }
    }
    if (!saw_digit || i < val.len) {
        return 0;
    }
    const total: u32 = whole * 1000 + frac;
    if (total > 1000) {
        return 1000;
    }
    return total as u16;
}

fn classifyToken(tok: string): u8 {
    if (tok.len == 1 && tok[0] == 42) { // "*"
        return SPEC_WILDCARD;
    }
    if (tok.len == 3 && tok[0] == 42 && tok[1] == 47 && tok[2] == 42) { // "*/*"
        return SPEC_WILDCARD;
    }
    if (tok.len >= 2 && tok[tok.len - 2] == 47 && tok[tok.len - 1] == 42) { // "type/*"
        return SPEC_PARTIAL;
    }
    return SPEC_EXACT;
}

/// Parse a comma-separated RFC 7231 Accept* header into `out` without heap allocation.
/// Extracts token spans, `;q=...` quality weights (0..1000, default 1000), specificity, and order.
pub fn parseAcceptList(header: string, out: []NegSpec): usize {
    if (header.len == 0 || out.len == 0) {
        return 0;
    }
    let count: usize = 0;
    let pos: usize = 0;

    while (pos < header.len && count < out.len) {
        // Find end of comma-separated item
        let item_end: usize = pos;
        while (item_end < header.len && header[item_end] != 44) { // ','
            item_end += 1;
        }

        const seg_lo: usize = trimStart(header, pos, item_end);
        const seg_hi: usize = trimEnd(header, seg_lo, item_end);

        if (seg_lo < seg_hi) {
            // Split token and optional parameters on first ';'
            let semi: usize = seg_lo;
            while (semi < seg_hi && header[semi] != 59) { // ';'
                semi += 1;
            }
            const tok_lo: usize = trimStart(header, seg_lo, semi);
            const tok_hi: usize = trimEnd(header, tok_lo, semi);

            if (tok_lo < tok_hi) {
                let q_milli: u16 = 1000;
                let p_cur: usize = semi;
                while (p_cur < seg_hi) {
                    p_cur += 1; // skip ';'
                    let p_end: usize = p_cur;
                    while (p_end < seg_hi && header[p_end] != 59) {
                        p_end += 1;
                    }
                    const p_lo: usize = trimStart(header, p_cur, p_end);
                    const p_hi: usize = trimEnd(header, p_lo, p_end);
                    let eq_pos: usize = p_lo;
                    while (eq_pos < p_hi && header[eq_pos] != 61) { // '='
                        eq_pos += 1;
                    }
                    if (eq_pos < p_hi) {
                        const k_lo: usize = trimStart(header, p_lo, eq_pos);
                        const k_hi: usize = trimEnd(header, k_lo, eq_pos);
                        if (k_hi - k_lo == 1 && (header[k_lo] == 113 || header[k_lo] == 81)) { // 'q' or 'Q'
                            const v_lo: usize = trimStart(header, eq_pos + 1, p_hi);
                            const v_hi: usize = trimEnd(header, v_lo, p_hi);
                            q_milli = parseQMilli(header[v_lo..v_hi]);
                        }
                    }
                    p_cur = p_end;
                }

                const tok_str: string = header[tok_lo..tok_hi];
                out[count] = NegSpec {
                    start: tok_lo,
                    len: tok_hi - tok_lo,
                    q_milli: q_milli,
                    specificity: classifyToken(tok_str),
                    order: count,
                };
                count += 1;
            }
        }

        pos = if (item_end < header.len) item_end + 1 else header.len;
    }

    return count;
}

fn isBetterCandidate(
    q: u16,
    spec: u8,
    order: usize,
    best_q: u16,
    best_spec: u8,
    best_order: usize,
    has_best: bool,
): bool {
    if (!has_best) {
        return true;
    }
    if (q != best_q) {
        return q > best_q;
    }
    if (spec != best_spec) {
        return spec > best_spec;
    }
    return order < best_order;
}

/// Select the best matching content encoding index from `available` given an `Accept-Encoding` header.
/// Returns `-1` if `available` is empty or no encoding is acceptable (`q=0` explicitly rejects).
/// Empty header falls back to `"identity"` if present, otherwise index `0`.
pub fn bestEncoding(header: string, available: []const string): i64 {
    if (available.len == 0) {
        return -1;
    }
    let specs: [32]NegSpec = undefined;
    const n: usize = parseAcceptList(header, &specs);
    if (n == 0) {
        let i: usize = 0;
        while (i < available.len) {
            if (eqIgnoreCase(available[i], "identity")) {
                return i as i64;
            }
            i += 1;
        }
        return 0;
    }

    let best_idx: i64 = -1;
    let best_q: u16 = 0;
    let best_spec: u8 = 0;
    let best_order: usize = 0;

    let i: usize = 0;
    while (i < available.len) {
        const cand: string = available[i];
        let matched: bool = false;
        let m_spec: u8 = 0;
        let m_q: u16 = 0;
        let m_order: usize = 0;

        let j: usize = 0;
        while (j < n) {
            const s: NegSpec = specs[j];
            const tok: string = header[s.start .. s.start + s.len];
            let cur_spec: u8 = 0;
            if (eqIgnoreCase(tok, cand)) {
                cur_spec = SPEC_EXACT;
            } else if (tok.len == 1 && tok[0] == 42) {
                cur_spec = SPEC_WILDCARD;
            }
            if (cur_spec > 0) {
                if (!matched || cur_spec > m_spec || (cur_spec == m_spec && s.q_milli > m_q)) {
                    matched = true;
                    m_spec = cur_spec;
                    m_q = s.q_milli;
                    m_order = s.order;
                }
            }
            j += 1;
        }

        if (matched && m_q > 0) {
            if (isBetterCandidate(m_q, m_spec, m_order, best_q, best_spec, best_order, best_idx >= 0)) {
                best_idx = i as i64;
                best_q = m_q;
                best_spec = m_spec;
                best_order = m_order;
            }
        }
        i += 1;
    }

    return best_idx;
}

/// Select the best matching media type index from `available` given an `Accept` header.
/// Ranks by quality `q`, then specificity (`type/sub` > `type/*` > `*/*`), then header order.
/// Returns `-1` if `available` is empty or no media type is acceptable. Empty header defaults to `0`.
pub fn bestMediaType(header: string, available: []const string): i64 {
    if (available.len == 0) {
        return -1;
    }
    let specs: [32]NegSpec = undefined;
    const n: usize = parseAcceptList(header, &specs);
    if (n == 0) {
        return 0;
    }

    let best_idx: i64 = -1;
    let best_q: u16 = 0;
    let best_spec: u8 = 0;
    let best_order: usize = 0;

    let i: usize = 0;
    while (i < available.len) {
        const cand: string = available[i];
        const c_slash: i64 = findByte(cand, 47);
        const c_type: string = if (c_slash >= 0) cand[0..(c_slash as usize)] else cand;
        const c_sub: string = if (c_slash >= 0) cand[((c_slash as usize) + 1)..cand.len] else "";

        let matched: bool = false;
        let m_spec: u8 = 0;
        let m_q: u16 = 0;
        let m_order: usize = 0;

        let j: usize = 0;
        while (j < n) {
            const s: NegSpec = specs[j];
            const tok: string = header[s.start .. s.start + s.len];
            let cur_spec: u8 = 0;

            if ((tok.len == 1 && tok[0] == 42) || eqIgnoreCase(tok, "*/*")) {
                cur_spec = SPEC_WILDCARD;
            } else {
                const s_slash: i64 = findByte(tok, 47);
                if (s_slash >= 0) {
                    const s_type: string = tok[0..(s_slash as usize)];
                    const s_sub: string = tok[((s_slash as usize) + 1)..tok.len];
                    if (eqIgnoreCase(s_type, c_type)) {
                        if (s_sub.len == 1 && s_sub[0] == 42) {
                            cur_spec = SPEC_PARTIAL;
                        } else if (eqIgnoreCase(s_sub, c_sub)) {
                            cur_spec = SPEC_EXACT;
                        }
                    }
                }
            }

            if (cur_spec > 0) {
                if (!matched || cur_spec > m_spec || (cur_spec == m_spec && s.q_milli > m_q)) {
                    matched = true;
                    m_spec = cur_spec;
                    m_q = s.q_milli;
                    m_order = s.order;
                }
            }
            j += 1;
        }

        if (matched && m_q > 0) {
            if (isBetterCandidate(m_q, m_spec, m_order, best_q, best_spec, best_order, best_idx >= 0)) {
                best_idx = i as i64;
                best_q = m_q;
                best_spec = m_spec;
                best_order = m_order;
            }
        }
        i += 1;
    }

    return best_idx;
}

fn langPrefixMatch(longer: string, shorter: string): bool {
    if (shorter.len == 0 || longer.len <= shorter.len) {
        return false;
    }
    if (longer[shorter.len] != 45) { // '-'
        return false;
    }
    return eqIgnoreCase(longer[0..shorter.len], shorter);
}

/// Select the best matching language index from `available` given an `Accept-Language` header.
/// Supports case-insensitive exact matching and BCP-47 prefix fallback (`en-US` <-> `en`).
/// Returns `-1` if `available` is empty or no language is acceptable. Empty header defaults to `0`.
pub fn bestLanguage(header: string, available: []const string): i64 {
    if (available.len == 0) {
        return -1;
    }
    let specs: [32]NegSpec = undefined;
    const n: usize = parseAcceptList(header, &specs);
    if (n == 0) {
        return 0;
    }

    let best_idx: i64 = -1;
    let best_q: u16 = 0;
    let best_spec: u8 = 0;
    let best_order: usize = 0;

    let i: usize = 0;
    while (i < available.len) {
        const cand: string = available[i];
        let matched: bool = false;
        let m_spec: u8 = 0;
        let m_q: u16 = 0;
        let m_order: usize = 0;

        let j: usize = 0;
        while (j < n) {
            const s: NegSpec = specs[j];
            const tok: string = header[s.start .. s.start + s.len];
            let cur_spec: u8 = 0;

            if (eqIgnoreCase(tok, cand)) {
                cur_spec = SPEC_EXACT;
            } else if (langPrefixMatch(tok, cand) || langPrefixMatch(cand, tok)) {
                cur_spec = SPEC_PARTIAL;
            } else if (tok.len == 1 && tok[0] == 42) {
                cur_spec = SPEC_WILDCARD;
            }

            if (cur_spec > 0) {
                if (!matched || cur_spec > m_spec || (cur_spec == m_spec && s.q_milli > m_q)) {
                    matched = true;
                    m_spec = cur_spec;
                    m_q = s.q_milli;
                    m_order = s.order;
                }
            }
            j += 1;
        }

        if (matched && m_q > 0) {
            if (isBetterCandidate(m_q, m_spec, m_order, best_q, best_spec, best_order, best_idx >= 0)) {
                best_idx = i as i64;
                best_q = m_q;
                best_spec = m_spec;
                best_order = m_order;
            }
        }
        i += 1;
    }

    return best_idx;
}
