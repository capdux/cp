// Origin: Pure-CP Stdlib (Zero-Alloc RFC 3912 WHOIS Protocol Encoder & Response Parser) | Created: 2026-11-22

// ---------------------------------------------------------------------------
// Protocol Constants (RFC 3912)
// ---------------------------------------------------------------------------

pub const WHOIS_TCP_PORT: u16 = 43;
pub const WHOIS_IANA_HOST: []const u8 = "whois.iana.org";
pub const WHOIS_MAX_QUERY_LEN: usize = 255;

// ---------------------------------------------------------------------------
// Internal ASCII Helpers
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

fn isAlphaNumAscii(b: u8): bool {
    return (b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') || (b >= '0' && b <= '9');
}

fn isCommentLine(trimmed: []const u8): bool {
    if (trimmed.len == 0) {
        return true;
    }
    const first = trimmed[0];
    if (first == '#' || first == '%') {
        return true;
    }
    if (trimmed.len >= 3 && trimmed[0] == '>' && trimmed[1] == '>' && trimmed[2] == '>') {
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Query Validation & Zero-Alloc Encoding
// ---------------------------------------------------------------------------

/// Validates whether `domain` is a syntactically valid ASCII DNS domain name for WHOIS queries.
pub fn isValidWhoisDomain(domain: []const u8): bool {
    if (domain.len == 0 || domain.len > WHOIS_MAX_QUERY_LEN) {
        return false;
    }
    let label_len: usize = 0;
    let i: usize = 0;
    while (i < domain.len) {
        const b = domain[i];
        if (b == '.') {
            if (label_len == 0 || label_len > 63) {
                return false;
            }
            if (domain[i - 1] == '-') {
                return false;
            }
            label_len = 0;
        } else if (isAlphaNumAscii(b)) {
            label_len += 1;
            if (label_len > 63) {
                return false;
            }
        } else if (b == '-') {
            if (label_len == 0) {
                return false;
            }
            label_len += 1;
            if (label_len > 63) {
                return false;
            }
        } else {
            return false;
        }
        i += 1;
    }
    if (label_len == 0 || label_len > 63 || domain[domain.len - 1] == '-') {
        return false;
    }
    return true;
}

/// Encodes a raw RFC 3912 WHOIS query into `out` as `query + "\r\n"`.
/// Rejects empty queries, queries exceeding `WHOIS_MAX_QUERY_LEN`, queries containing
/// `\r`, `\n`, or `\0` (CRLF/NUL injection guard), or insufficient `out` capacity.
pub fn encodeWhoisQuery(query: []const u8, out: []u8): usize? {
    if (query.len == 0 || query.len > WHOIS_MAX_QUERY_LEN) {
        return null;
    }
    if (out.len < query.len + 2) {
        return null;
    }
    let i: usize = 0;
    while (i < query.len) {
        const b = query[i];
        if (b == '\r' || b == '\n' || b == 0) {
            return null;
        }
        out[i] = b;
        i += 1;
    }
    out[query.len] = '\r';
    out[query.len + 1] = '\n';
    return query.len + 2;
}

/// Validates `domain` as a legal ASCII domain name and encodes it as `domain + "\r\n"`.
pub fn encodeWhoisDomainQuery(domain: []const u8, out: []u8): usize? {
    if (!isValidWhoisDomain(domain)) {
        return null;
    }
    return encodeWhoisQuery(domain, out);
}

// ---------------------------------------------------------------------------
// Zero-Copy Key-Value Response Iterator & Field Lookup
// ---------------------------------------------------------------------------

/// A single trimmed `key: value` entry from a WHOIS response body.
pub struct WhoisField {
    key: []const u8,
    value: []const u8,

    /// Case-insensitive ASCII comparison against `field.key`.
    fn keyEqualsIgnoreCase(target: []const u8): bool {
        return eqIgnoreCase(_.key, target);
    }
}

/// Zero-allocation line-by-line WHOIS key-value iterator.
/// Automatically skips blank lines, `#`/`%`/`>>>` comment lines, and lines without `:`.
/// Supports both `\r\n` and `\n` line endings.
pub struct WhoisFieldIter {
    body: []const u8,
    offset: usize,

    fn init(body: []const u8): WhoisFieldIter {
        return WhoisFieldIter {
            body: body,
            offset: 0,
        };
    }

    fn next(*_): WhoisField? {
        while (_.offset < _.body.len) {
            const line_start = _.offset;
            while (_.offset < _.body.len && _.body[_.offset] != '\n') {
                _.offset += 1;
            }
            let line_end = _.offset;
            if (_.offset < _.body.len && _.body[_.offset] == '\n') {
                _.offset += 1;
            }
            if (line_end > line_start && _.body[line_end - 1] == '\r') {
                line_end -= 1;
            }

            const trimmed = trimAscii(_.body[line_start..line_end]);
            if (isCommentLine(trimmed)) {
                continue;
            }

            // Find first ':' delimiter
            let colon_pos: usize? = null;
            let i: usize = 0;
            while (i < trimmed.len) {
                if (trimmed[i] == ':') {
                    colon_pos = i;
                    break;
                }
                i += 1;
            }
            if (colon := colon_pos) {
                const key = trimAscii(trimmed[0..colon]);
                if (key.len == 0) {
                    continue;
                }
                const value = trimAscii(trimmed[colon + 1..]);
                return WhoisField {
                    key: key,
                    value: value,
                };
            }
        }
        return null;
    }
}

/// Case-insensitively searches `body` for the first field matching `key` and returns its value.
pub fn findWhoisField(body: []const u8, key: []const u8): ([]const u8)? {
    let it = WhoisFieldIter.init(body);
    while (field := it.next()) {
        if (eqIgnoreCase(field.key, key)) {
            return field.value;
        }
    }
    return null;
}

/// Extracts the referral WHOIS server hostname from an IANA or Registrar WHOIS response.
/// Checks `refer`, `Registrar WHOIS Server`, `Whois Server`, and `whois` in priority order.
pub fn parseWhoisRefer(body: []const u8): ([]const u8)? {
    if (v := findWhoisField(body, "refer")) {
        if (v.len > 0) {
            return v;
        }
    }
    if (v := findWhoisField(body, "Registrar WHOIS Server")) {
        if (v.len > 0) {
            return v;
        }
    }
    if (v := findWhoisField(body, "Whois Server")) {
        if (v.len > 0) {
            return v;
        }
    }
    if (v := findWhoisField(body, "whois")) {
        if (v.len > 0) {
            return v;
        }
    }
    return null;
}

// ---------------------------------------------------------------------------
// Normalized WHOIS Summary (ICANN / Verisign / RIPE / ARIN)
// ---------------------------------------------------------------------------

/// Normalized zero-copy summary extracted from a WHOIS response body.
pub struct WhoisSummary {
    domain_name: []const u8,
    registrar: []const u8,
    whois_server: []const u8,
    created: []const u8,
    updated: []const u8,
    expires: []const u8,
    nameserver_count: usize,
    status_count: usize,

    fn hasDomain(): bool {
        return _.domain_name.len > 0;
    }

    fn hasRegistrar(): bool {
        return _.registrar.len > 0;
    }

    fn hasWhoisServer(): bool {
        return _.whois_server.len > 0;
    }
}

/// Single-pass zero-allocation WHOIS summary extractor normalizing common
/// ICANN, Verisign, RIPE, and ARIN field aliases.
pub fn parseWhoisSummary(body: []const u8): WhoisSummary {
    let domain_name: []const u8 = "";
    let registrar: []const u8 = "";
    let whois_server: []const u8 = "";
    let created: []const u8 = "";
    let updated: []const u8 = "";
    let expires: []const u8 = "";
    let nameserver_count: usize = 0;
    let status_count: usize = 0;

    let it = WhoisFieldIter.init(body);
    while (field := it.next()) {
        const k = field.key;
        const v = field.value;
        if (v.len == 0) {
            continue;
        }

        if (domain_name.len == 0 && (eqIgnoreCase(k, "Domain Name") || eqIgnoreCase(k, "domain"))) {
            domain_name = v;
        } else if (registrar.len == 0 && (eqIgnoreCase(k, "Registrar") || eqIgnoreCase(k, "Sponsoring Registrar") || eqIgnoreCase(k, "org-name") || eqIgnoreCase(k, "OrgName"))) {
            registrar = v;
        } else if (whois_server.len == 0 && (eqIgnoreCase(k, "Registrar WHOIS Server") || eqIgnoreCase(k, "Whois Server") || eqIgnoreCase(k, "refer") || eqIgnoreCase(k, "whois"))) {
            whois_server = v;
        } else if (created.len == 0 && (eqIgnoreCase(k, "Creation Date") || eqIgnoreCase(k, "created") || eqIgnoreCase(k, "Registered") || eqIgnoreCase(k, "Registration Time") || eqIgnoreCase(k, "RegDate"))) {
            created = v;
        } else if (updated.len == 0 && (eqIgnoreCase(k, "Updated Date") || eqIgnoreCase(k, "last-modified") || eqIgnoreCase(k, "Last Updated") || eqIgnoreCase(k, "changed") || eqIgnoreCase(k, "Updated"))) {
            updated = v;
        } else if (expires.len == 0 && (eqIgnoreCase(k, "Registry Expiry Date") || eqIgnoreCase(k, "Registrar Registration Expiration Date") || eqIgnoreCase(k, "Expiry Date") || eqIgnoreCase(k, "Expiration Date") || eqIgnoreCase(k, "paid-till") || eqIgnoreCase(k, "expires"))) {
            expires = v;
        } else if (eqIgnoreCase(k, "Name Server") || eqIgnoreCase(k, "nserver") || eqIgnoreCase(k, "Nameserver")) {
            nameserver_count += 1;
        } else if (eqIgnoreCase(k, "Domain Status") || eqIgnoreCase(k, "status") || eqIgnoreCase(k, "state")) {
            status_count += 1;
        }
    }

    return WhoisSummary {
        domain_name: domain_name,
        registrar: registrar,
        whois_server: whois_server,
        created: created,
        updated: updated,
        expires: expires,
        nameserver_count: nameserver_count,
        status_count: status_count,
    };
}
