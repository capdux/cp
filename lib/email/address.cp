// Source: cp/lib/email/address.cp (RFC 5322 Email Address Parser & Header Injection Guard)
// Pure CP RFC 5322 email address parser, validator, formatter, and CRLF header injection guard.

const std = @import("std");

/// Unified error set for `cap:email` address parsing, MIME, and SMTP codec operations.
pub err EmailError {
    InvalidAddress,
    HeaderInjection,
    InvalidMime,
    InvalidSmtpReply,
    OutOfMemory,
}

/// Returns `true` if `s` contains CR (`\r`), LF (`\n`), or NUL (`\x00`) bytes that could enable header/command injection.
pub fn containsHeaderInjection(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch == '\r' || ch == '\n' || ch == 0) {
            return true;
        }
    }
    return false;
}

fn trimSpacesAndTabs(s: string): string {
    return std.mem.trim(u8, s, " \t");
}

fn isValidLocalChar(ch: u8): bool {
    if (ch >= 'a' && ch <= 'z') return true;
    if (ch >= 'A' && ch <= 'Z') return true;
    if (ch >= '0' && ch <= '9') return true;
    return switch (ch) {
        '.' | '!' | '#' | '$' | '%' | '&' | '\'' | '*' | '+' | '-' | '/' | '=' | '?' | '^' | '_' | '`' | '{' | '|' | '}' | '~' => true,
        else => false,
    };
}

fn isValidDomainLabelChar(ch: u8): bool {
    if (ch >= 'a' && ch <= 'z') return true;
    if (ch >= 'A' && ch <= 'Z') return true;
    if (ch >= '0' && ch <= '9') return true;
    return ch == '-';
}

fn isValidLocalPart(local: string): bool {
    if (local.len == 0 || local.len > 64) return false;
    if (local[0] == '.' || local[local.len - 1] == '.') return false;

    let prev_dot = false;
    let i: usize = 0;
    while (i < local.len) : (i += 1) {
        const ch = local[i];
        if (!isValidLocalChar(ch)) return false;
        if (ch == '.') {
            if (prev_dot) return false;
            prev_dot = true;
        } else {
            prev_dot = false;
        }
    }
    return true;
}

fn isValidDomain(domain: string): bool {
    if (domain.len == 0 || domain.len > 253) return false;
    if (domain[0] == '.' || domain[domain.len - 1] == '.') return false;

    let label_count: usize = 0;
    let label_len: usize = 0;
    let last_label_len: usize = 0;
    let last_label_all_digits = true;
    let prev_dot = false;

    let i: usize = 0;
    while (i < domain.len) : (i += 1) {
        const ch = domain[i];
        if (ch == '.') {
            if (prev_dot || label_len == 0 || label_len > 63) return false;
            if (domain[i - 1] == '-') return false;
            label_count += 1;
            label_len = 0;
            last_label_len = 0;
            last_label_all_digits = true;
            prev_dot = true;
        } else {
            if (!isValidDomainLabelChar(ch)) return false;
            if (label_len == 0 && ch == '-') return false;
            prev_dot = false;
            label_len += 1;
            if (label_len > 63) return false;
            last_label_len = label_len;
            if (ch < '0' || ch > '9') {
                last_label_all_digits = false;
            }
        }
    }

    if (prev_dot || label_len == 0 || label_len > 63) return false;
    if (domain[domain.len - 1] == '-') return false;
    label_count += 1;

    // Require at least two labels (e.g. example.com) and non-numeric TLD of length >= 2
    if (label_count < 2) return false;
    if (last_label_len < 2 || last_label_all_digits) return false;

    return true;
}

struct AddrSpecParts {
    local: string = "",
    domain: string = "",
}

fn validateAddrSpec(addr_spec: string): AddrSpecParts ! EmailError {
    if (addr_spec.len < 3 || addr_spec.len > 254) {
        return EmailError.InvalidAddress;
    }

    let at_idx: usize? = null;
    let i: usize = 0;
    while (i < addr_spec.len) : (i += 1) {
        const ch = addr_spec[i];
        if (ch == ' ' || ch == '\t' || ch < 0x20 || ch >= 0x7F) {
            return EmailError.InvalidAddress;
        }
        if (ch == '@') {
            if (at_idx != null) {
                return EmailError.InvalidAddress;
            }
            at_idx = i;
        }
    }

    if (at_idx == null) {
        return EmailError.InvalidAddress;
    }
    const idx = at_idx.?;
    const local = addr_spec[0..idx];
    const domain = addr_spec[idx + 1 ..];

    if (!isValidLocalPart(local) || !isValidDomain(domain)) {
        return EmailError.InvalidAddress;
    }

    return AddrSpecParts{ local: local, domain: domain };
}

fn displayNeedsQuoting(name: string): bool {
    let i: usize = 0;
    while (i < name.len) : (i += 1) {
        switch (name[i]) {
            '"' | '\\' | '<' | '>' | '(' | ')' | '[' | ']' | ':' | ';' | '@' | ',' | '.' => return true,
            else => {},
        }
    }
    return false;
}

/// Parsed RFC 5322 email address with optional display name, local part, and domain.
pub struct EmailAddress {
    display_name: string = "",
    local_part: string = "",
    domain: string = "",

    /// Parses either `"Display Name <user@domain>"` or `"user@domain"` with strict RFC 5322 & CRLF injection checks.
    pub static fn parse(raw: string): EmailAddress ! EmailError {
        if (containsHeaderInjection(raw)) {
            return EmailError.HeaderInjection;
        }

        const trimmed = trimSpacesAndTabs(raw);
        if (trimmed.len == 0) {
            return EmailError.InvalidAddress;
        }

        let in_quotes = false;
        let escaped = false;
        let lt_idx: usize? = null;
        let i: usize = 0;
        while (i < trimmed.len) : (i += 1) {
            const ch = trimmed[i];
            if (escaped) {
                escaped = false;
                continue;
            }
            if (in_quotes) {
                if (ch == '\\') {
                    escaped = true;
                } else if (ch == '"') {
                    in_quotes = false;
                }
                continue;
            }
            if (ch == '"') {
                in_quotes = true;
            } else if (ch == '<') {
                if (lt_idx != null) {
                    return EmailError.InvalidAddress;
                }
                lt_idx = i;
            } else if (ch == '>') {
                if (lt_idx == null || i != trimmed.len - 1) {
                    return EmailError.InvalidAddress;
                }
            }
        }

        if (in_quotes || escaped) {
            return EmailError.InvalidAddress;
        }

        if (lt := lt_idx) {
            if (trimmed[trimmed.len - 1] != '>') {
                return EmailError.InvalidAddress;
            }
            const addr_spec = trimmed[lt + 1 .. trimmed.len - 1];
            const parts = validateAddrSpec(addr_spec)?;

            let disp = trimSpacesAndTabs(trimmed[0..lt]);
            if (disp.len >= 2 && disp[0] == '"' && disp[disp.len - 1] == '"') {
                disp = disp[1 .. disp.len - 1];
            }

            return EmailAddress{
                display_name: disp,
                local_part: parts.local,
                domain: parts.domain,
            };
        }

        const parts = validateAddrSpec(trimmed)?;
        return EmailAddress{
            display_name: "",
            local_part: parts.local,
            domain: parts.domain,
        };
    }

    /// Returns `true` if `raw` is a valid, injection-free RFC 5322 email address.
    pub static fn isValid(raw: string): bool {
        * = EmailAddress.parse(raw) catch return false;
        return true;
    }

    /// Formats the bare `local_part@domain` address without any display name.
    pub fn emailOnly(alloc: std.mem.Allocator): ([]u8) ! EmailError {
        const total = _.local_part.len + 1 + _.domain.len;
        let out = alloc.alloc(u8, total) catch return EmailError.OutOfMemory;
        @memcpy(out[0.._.local_part.len], _.local_part);
        out[_.local_part.len] = '@';
        @memcpy(out[_.local_part.len + 1 .. total], _.domain);
        return out;
    }

    /// Formats as `"Display Name <local@domain>"` (quoting specials if needed) or `"local@domain"`.
    pub fn format(alloc: std.mem.Allocator): ([]u8) ! EmailError {
        if (containsHeaderInjection(_.display_name) || containsHeaderInjection(_.local_part) || containsHeaderInjection(_.domain)) {
            return EmailError.HeaderInjection;
        }
        if (_.display_name.len == 0) {
            return _.emailOnly(alloc);
        }

        let buf: std.ArrayListUnmanaged(u8) = .empty;
        errdefer buf.deinit(alloc);

        const quote = displayNeedsQuoting(_.display_name);
        if (quote) {
            buf.append(alloc, '"') catch return EmailError.OutOfMemory;
            let i: usize = 0;
            while (i < _.display_name.len) : (i += 1) {
                const ch = _.display_name[i];
                if (ch == '"' || ch == '\\') {
                    buf.append(alloc, '\\') catch return EmailError.OutOfMemory;
                }
                buf.append(alloc, ch) catch return EmailError.OutOfMemory;
            }
            buf.append(alloc, '"') catch return EmailError.OutOfMemory;
        } else {
            buf.appendSlice(alloc, _.display_name) catch return EmailError.OutOfMemory;
        }

        buf.appendSlice(alloc, " <") catch return EmailError.OutOfMemory;
        buf.appendSlice(alloc, _.local_part) catch return EmailError.OutOfMemory;
        buf.append(alloc, '@') catch return EmailError.OutOfMemory;
        buf.appendSlice(alloc, _.domain) catch return EmailError.OutOfMemory;
        buf.append(alloc, '>') catch return EmailError.OutOfMemory;

        return buf.toOwnedSlice(alloc) catch return EmailError.OutOfMemory;
    }
}
