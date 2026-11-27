// @cap/email-address — Zero-heap RFC 5321/5322 email address parser & validator in pure CP
// Clean-room MIT implementation of crates.io `email_address` (#935, 110.4M downloads).
// Accelerated with comptime 256-entry atext/label LUTs and 16-lane @Vector(16, u8) SIMD scanning.

type EmVec16 = @Vector(16, u8);

pub struct EmailAddress {
    pub local_part: string,
    pub domain: string,
    pub display_name: string,
    pub valid: bool,
}

fn emBuildAtextLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let b: usize = 0;
    while (b < 256) {
        if ((b >= 65 && b <= 90) || (b >= 97 && b <= 122) || (b >= 48 && b <= 57) ||
            b == 33 || b == 35 || b == 36 || b == 37 || b == 38 || b == 39 || b == 42 ||
            b == 43 || b == 45 || b == 47 || b == 61 || b == 63 || b == 94 || b == 95 ||
            b == 96 || b == 123 || b == 124 || b == 125 || b == 126)
        {
            lut[b] = 1;
        }
        b += 1;
    }
    return lut;
}

fn emBuildLabelLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let b: usize = 0;
    while (b < 256) {
        if ((b >= 65 && b <= 90) || (b >= 97 && b <= 122) || (b >= 48 && b <= 57) || b == 45) {
            lut[b] = 1;
        }
        b += 1;
    }
    return lut;
}

const EM_ATEXT_LUT: [256]u8 = emBuildAtextLut();
const EM_LABEL_LUT: [256]u8 = emBuildLabelLut();

fn emFindByte(s: string, start: usize, target: u8): usize {
    let i: usize = start;
    const len = s.len;
    const vt: EmVec16 = @splat(target);
    while (i + 16 <= len) {
        const v: EmVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == vt)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == target) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (s[i] == target) {
            return i;
        }
        i += 1;
    }
    return len;
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
        } else if (EM_ATEXT_LUT[b as usize] != 0) {
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
    while (label_start < domain.len) {
        const dot = emFindByte(domain, label_start, 46);
        const label = domain[label_start..dot];
        if (label.len == 0 || label.len > 63) { return false; }
        if (label[0] == 45 || label[label.len - 1] == 45) { return false; }
        let k: usize = 0;
        const llen = label.len;
        while (k + 4 <= llen) {
            if ((EM_LABEL_LUT[label[k] as usize] &
                EM_LABEL_LUT[label[k + 1] as usize] &
                EM_LABEL_LUT[label[k + 2] as usize] &
                EM_LABEL_LUT[label[k + 3] as usize]) == 0)
            {
                return false;
            }
            k += 4;
        }
        while (k < llen) {
            if (EM_LABEL_LUT[label[k] as usize] == 0) { return false; }
            k += 1;
        }
        if (dot >= domain.len) {
            break;
        }
        label_start = dot + 1;
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
        const lt = emFindByte(s, 0, 60);
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
    const at_pos = emFindByte(addr, 0, 64);
    if (at_pos == addr.len) {
        return inv;
    }
    if (emFindByte(addr, at_pos + 1, 64) != addr.len) {
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
