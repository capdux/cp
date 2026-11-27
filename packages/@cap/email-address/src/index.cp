// @cap/email-address — Zero-heap RFC 5321/5322 email address parser & validator in pure CP
// Clean-room MIT implementation of crates.io `email_address` (#935, 110.4M downloads).

pub struct EmailAddress {
    pub local_part: string,
    pub domain: string,
    pub display_name: string,
    pub valid: bool,
}

fn emIsAtext(b: u8): bool {
    if ((b >= 65 && b <= 90) || (b >= 97 && b <= 122) || (b >= 48 && b <= 57)) {
        return true;
    }
    if (b == 33 || b == 35 || b == 36 || b == 37 || b == 38 || b == 39 || b == 42 ||
        b == 43 || b == 45 || b == 47 || b == 61 || b == 63 || b == 94 || b == 95 ||
        b == 96 || b == 123 || b == 124 || b == 125 || b == 126) {
        return true;
    }
    return false;
}

fn emIsLabelChar(b: u8): bool {
    return (b >= 65 && b <= 90) || (b >= 97 && b <= 122) || (b >= 48 && b <= 57) || b == 45;
}

fn emTrim(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == 32 || s[start] == 9)) { start += 1; }
    while (end > start && (s[end - 1] == 32 || s[end - 1] == 9)) { end -= 1; }
    return s[start..end];
}

fn emValidLocal(local: string): bool {
    if (local.len == 0 || local.len > 64) {
        return false;
    }
    if (local[0] == 46 || local[local.len - 1] == 46) {
        return false; // leading or trailing dot
    }
    let prev_dot = false;
    let i: usize = 0;
    while (i < local.len) {
        const b = local[i];
        if (b == 46) {
            if (prev_dot) { return false; } // consecutive dots
            prev_dot = true;
        } else if (emIsAtext(b)) {
            prev_dot = false;
        } else {
            return false;
        }
        i += 1;
    }
    return true;
}

fn emValidDomain(domain: string): bool {
    if (domain.len == 0 || domain.len > 253) {
        return false;
    }
    if (domain[0] == 46 || domain[domain.len - 1] == 46) {
        return false;
    }
    let label_start: usize = 0;
    let i: usize = 0;
    while (i <= domain.len) {
        if (i == domain.len || domain[i] == 46) {
            const label = domain[label_start..i];
            if (label.len == 0 || label.len > 63) { return false; }
            if (label[0] == 45 || label[label.len - 1] == 45) { return false; }
            let k: usize = 0;
            while (k < label.len) {
                if (!emIsLabelChar(label[k])) { return false; }
                k += 1;
            }
            label_start = i + 1;
        }
        i += 1;
    }
    return true;
}

/// Parses an RFC 5321/5322 email address (`user@example.com` or `Alice <alice@example.com>`).
pub fn parseEmail(input: string): EmailAddress {
    const inv = EmailAddress { local_part: "", domain: "", display_name: "", valid: false };
    const s = emTrim(input);
    if (s.len < 3) {
        return inv;
    }

    let display = "";
    let addr = s;
    if (s[s.len - 1] == 62) { // '>'
        let lt: usize = s.len;
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == 60) { lt = i; break; }
            i += 1;
        }
        if (lt == s.len) { return inv; }
        display = emTrim(s[0..lt]);
        if (display.len >= 2 && display[0] == 34 && display[display.len - 1] == 34) {
            display = display[1..(display.len - 1)];
        }
        addr = emTrim(s[(lt + 1)..(s.len - 1)]);
    }

    if (addr.len > 254) {
        return inv;
    }

    // Must have exactly one '@'
    let at_pos: usize = addr.len;
    let i: usize = 0;
    while (i < addr.len) {
        if (addr[i] == 64) {
            if (at_pos != addr.len) { return inv; }
            at_pos = i;
        }
        i += 1;
    }
    if (at_pos == addr.len) {
        return inv;
    }

    const local = addr[0..at_pos];
    const domain = addr[(at_pos + 1)..addr.len];
    if (!emValidLocal(local) || !emValidDomain(domain)) {
        return inv;
    }

    return EmailAddress {
        local_part: local,
        domain: domain,
        display_name: display,
        valid: true,
    };
}

/// Returns `true` if `input` is a valid RFC 5321/5322 email address.
pub fn isValidEmail(input: string): bool {
    return parseEmail(input).valid;
}

export {
    EmailAddress,
    parseEmail,
    isValidEmail,
};
