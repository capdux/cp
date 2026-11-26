// @cap/redact — Zero-allocation PII & secret redaction/masking engine for CP
// Clean-room MIT implementation inspired by npm `fast-redact`, `maskdata`, and Rust `secrecy`.

pub struct RedactOptions {
    pub redact_emails: bool,
    pub redact_tokens: bool,
    pub redact_cards: bool,
    pub redact_phones: bool,
    pub redact_ipv4: bool,

    pub static fn defaultAll() -> RedactOptions {
        return RedactOptions{
            redact_emails: true,
            redact_tokens: true,
            redact_cards: true,
            redact_phones: true,
            redact_ipv4: false,
        };
    }

    pub static fn strict() -> RedactOptions {
        return RedactOptions{
            redact_emails: true,
            redact_tokens: true,
            redact_cards: true,
            redact_phones: true,
            redact_ipv4: true,
        };
    }
}

fn isDigit(c: u8) -> bool {
    return c >= '0' && c <= '9';
}

fn isAlpha(c: u8) -> bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn isAlphaNum(c: u8) -> bool {
    return isAlpha(c) || isDigit(c);
}

fn toLower(c: u8) -> u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn isTokenChar(c: u8) -> bool {
    return isAlphaNum(c) || c == '_' || c == '-' || c == '.' || c == '+' || c == '/' || c == '=';
}

fn isEmailLocalChar(c: u8) -> bool {
    return isAlphaNum(c) || c == '.' || c == '_' || c == '-' || c == '+' || c == '%';
}

fn isDomainChar(c: u8) -> bool {
    return isAlphaNum(c) || c == '.' || c == '-';
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn startsWithAt(s: string, idx: usize, prefix: string) -> bool {
    if (idx + prefix.len > s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[idx + i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn eqIgnoreCase(a: string, b: string) -> bool {
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

// ---------------------------------------------------------------------------
// Single-value masking utilities (zero heap allocation)
// ---------------------------------------------------------------------------

pub fn maskEmail(email: string, out: []u8) -> string {
    let at: usize = email.len;
    let i: usize = 0;
    while (i < email.len) {
        if (email[i] == '@') {
            at = i;
            break;
        }
        i += 1;
    }
    if (at == 0 || at >= email.len - 1) {
        const n = appendStr(out, 0 as usize, "***");
        return out[0..n];
    }

    let pos: usize = 0;
    if (at <= 2) {
        pos = appendByte(out, pos, email[0]);
        pos = appendStr(out, pos, "***");
    } else {
        pos = appendByte(out, pos, email[0]);
        pos = appendStr(out, pos, "***");
        pos = appendByte(out, pos, email[at - 1]);
    }
    pos = appendStr(out, pos, email[at..email.len]);
    return out[0..pos];
}

pub fn maskPhone(phone: string, out: []u8) -> string {
    // Keep prefix (up to first 3 digits after optional +CC) and last 4 digits; mask middle digits
    let total_digits: usize = 0;
    let i: usize = 0;
    while (i < phone.len) {
        if (isDigit(phone[i])) {
            total_digits += 1;
        }
        i += 1;
    }
    if (total_digits <= 4) {
        const n = appendStr(out, 0 as usize, "****");
        return out[0..n];
    }
    let keep_head: usize = 3;
    if (phone.len > 0 && phone[0] == '+') {
        keep_head = 5;
    }
    if (keep_head + 4 >= total_digits) {
        keep_head = 2;
    }

    let pos: usize = 0;
    let seen_digits: usize = 0;
    i = 0;
    while (i < phone.len) {
        const c = phone[i];
        if (isDigit(c)) {
            if (seen_digits < keep_head || seen_digits + 4 >= total_digits) {
                pos = appendByte(out, pos, c);
            } else {
                pos = appendByte(out, pos, '*');
            }
            seen_digits += 1;
        } else {
            pos = appendByte(out, pos, c);
        }
        i += 1;
    }
    return out[0..pos];
}

pub fn maskCard(card: string, out: []u8) -> string {
    let total_digits: usize = 0;
    let i: usize = 0;
    while (i < card.len) {
        if (isDigit(card[i])) {
            total_digits += 1;
        }
        i += 1;
    }
    let pos: usize = 0;
    let seen: usize = 0;
    i = 0;
    while (i < card.len) {
        const c = card[i];
        if (isDigit(c)) {
            if (seen + 4 >= total_digits) {
                pos = appendByte(out, pos, c);
            } else {
                pos = appendByte(out, pos, '*');
            }
            seen += 1;
        } else {
            pos = appendByte(out, pos, c);
        }
        i += 1;
    }
    return out[0..pos];
}

pub fn maskSecret(secret: string, out: []u8) -> string {
    let pos: usize = 0;
    if (secret.len <= 8) {
        pos = appendStr(out, pos, "****");
        return out[0..pos];
    }
    pos = appendStr(out, pos, secret[0..4]);
    pos = appendStr(out, pos, "***");
    pos = appendStr(out, pos, secret[secret.len - 4..secret.len]);
    return out[0..pos];
}

pub fn maskIp(ip: string, out: []u8) -> string {
    // Mask last two octets of IPv4: 192.168.1.42 -> 192.168.*.*
    let dots: usize = 0;
    let second_dot: usize = ip.len;
    let i: usize = 0;
    while (i < ip.len) {
        if (ip[i] == '.') {
            dots += 1;
            if (dots == 2) {
                second_dot = i;
                break;
            }
        }
        i += 1;
    }
    if (dots < 2) {
        const n = appendStr(out, 0 as usize, "*.*.*.*");
        return out[0..n];
    }
    let pos: usize = 0;
    pos = appendStr(out, pos, ip[0..second_dot]);
    pos = appendStr(out, pos, ".*.*");
    return out[0..pos];
}

// ---------------------------------------------------------------------------
// Pattern scanners for redactText
// ---------------------------------------------------------------------------

fn checkLuhnRange(s: string, start: usize, end: usize) -> bool {
    let sum: i64 = 0;
    let digits: usize = 0;
    let double_next = false;
    let i = end;
    while (i > start) {
        i -= 1;
        const c = s[i];
        if (c == ' ' || c == '-') {
            continue;
        }
        if (!isDigit(c)) {
            return false;
        }
        let d: i64 = (c - '0') as i64;
        if (double_next) {
            d = d * 2;
            if (d > 9) {
                d -= 9;
            }
        }
        sum += d;
        double_next = !double_next;
        digits += 1;
    }
    return digits >= 13 && digits <= 19 && @rem(sum, 10) == 0;
}

fn matchTokenEnd(s: string, idx: usize) -> usize {
    // Check well-known secret/API key prefixes & JWTs
    let matched_prefix = false;
    if (startsWithAt(s, idx, "eyJ") ||
        startsWithAt(s, idx, "sk-") ||
        startsWithAt(s, idx, "ghp_") ||
        startsWithAt(s, idx, "gho_") ||
        startsWithAt(s, idx, "cfut_") ||
        startsWithAt(s, idx, "capx_") ||
        startsWithAt(s, idx, "capdux_") ||
        startsWithAt(s, idx, "AKIA")) {
        matched_prefix = true;
    }
    if (!matched_prefix) {
        return 0;
    }
    let end = idx;
    while (end < s.len && isTokenChar(s[end])) {
        end += 1;
    }
    const len = end - idx;
    if (startsWithAt(s, idx, "eyJ")) {
        // JWT must have at least two '.' separators and length >= 24
        let dots: usize = 0;
        let k = idx;
        while (k < end) {
            if (s[k] == '.') {
                dots += 1;
            }
            k += 1;
        }
        if (dots >= 2 && len >= 24) {
            return end;
        }
        return 0;
    }
    if (len >= 16) {
        return end;
    }
    return 0;
}

fn matchEmailEnd(s: string, idx: usize) -> usize {
    if (!isAlphaNum(s[idx])) {
        return 0;
    }
    // Must be at word boundary
    if (idx > 0 && isEmailLocalChar(s[idx - 1])) {
        return 0;
    }
    let i = idx;
    while (i < s.len && isEmailLocalChar(s[i])) {
        i += 1;
    }
    if (i == idx || i >= s.len || s[i] != '@') {
        return 0;
    }
    const domain_start = i + 1;
    let j = domain_start;
    let saw_dot = false;
    while (j < s.len && isDomainChar(s[j])) {
        if (s[j] == '.') {
            saw_dot = true;
        }
        j += 1;
    }
    // Strip trailing punctuation dot if sentence ends with email.
    while (j > domain_start && s[j - 1] == '.') {
        j -= 1;
    }
    if (!saw_dot || j <= domain_start + 3) {
        return 0;
    }
    // Verify at least one '.' remains inside domain
    let k = domain_start;
    let valid_dot = false;
    while (k < j) {
        if (s[k] == '.' && k > domain_start && k + 2 < j) {
            valid_dot = true;
            break;
        }
        k += 1;
    }
    if (!valid_dot) {
        return 0;
    }
    return j;
}

fn matchCardEnd(s: string, idx: usize) -> usize {
    if (!isDigit(s[idx])) {
        return 0;
    }
    if (idx > 0 && isDigit(s[idx - 1])) {
        return 0;
    }
    let i = idx;
    let digits: usize = 0;
    let last_digit_end = idx;
    while (i < s.len && digits < 19) {
        const c = s[i];
        if (isDigit(c)) {
            digits += 1;
            last_digit_end = i + 1;
        } else if (c == '-' || c == ' ') {
            if (i + 1 >= s.len || !isDigit(s[i + 1])) {
                break;
            }
        } else {
            break;
        }
        i += 1;
    }
    if (last_digit_end < s.len && isDigit(s[last_digit_end])) {
        return 0; // more than 19 consecutive digits
    }
    if (digits >= 13 && digits <= 19 && checkLuhnRange(s, idx, last_digit_end)) {
        return last_digit_end;
    }
    return 0;
}

fn matchPhoneEnd(s: string, idx: usize) -> usize {
    // Match E.164 (+[1-9]...) or CN 11-digit mobile (1[3-9]xxxxxxxxx)
    if (s[idx] == '+') {
        if (idx > 0 && isAlphaNum(s[idx - 1])) {
            return 0;
        }
        if (idx + 1 >= s.len || s[idx + 1] < '1' || s[idx + 1] > '9') {
            return 0;
        }
        let i = idx + 1;
        let digits: usize = 0;
        while (i < s.len && isDigit(s[i])) {
            digits += 1;
            i += 1;
        }
        if (digits >= 8 && digits <= 15) {
            return i;
        }
        return 0;
    }
    // CN 11-digit mobile starting with 1[3-9]
    if (s[idx] == '1' && idx + 11 <= s.len && s[idx + 1] >= '3' && s[idx + 1] <= '9') {
        if (idx > 0 && isAlphaNum(s[idx - 1])) {
            return 0;
        }
        let k: usize = 0;
        while (k < 11) {
            if (!isDigit(s[idx + k])) {
                return 0;
            }
            k += 1;
        }
        if (idx + 11 < s.len && isDigit(s[idx + 11])) {
            return 0;
        }
        return idx + 11;
    }
    return 0;
}

fn matchIpv4End(s: string, idx: usize) -> usize {
    if (!isDigit(s[idx])) {
        return 0;
    }
    if (idx > 0 && (isAlphaNum(s[idx - 1]) || s[idx - 1] == '.')) {
        return 0;
    }
    let i = idx;
    let octets: usize = 0;
    while (octets < 4) {
        let val: i32 = 0;
        let dcount: usize = 0;
        while (i < s.len && isDigit(s[i]) && dcount < 3) {
            val = val * 10 + ((s[i] - '0') as i32);
            dcount += 1;
            i += 1;
        }
        if (dcount == 0 || val > 255) {
            return 0;
        }
        octets += 1;
        if (octets < 4) {
            if (i >= s.len || s[i] != '.') {
                return 0;
            }
            i += 1;
        }
    }
    if (i < s.len && (isAlphaNum(s[i]) || s[i] == '.')) {
        return 0;
    }
    return i;
}

pub fn redactText(input: string, opts: RedactOptions, out: []u8) -> string {
    let pos: usize = 0;
    let i: usize = 0;
    let tmp: [128]u8 = undefined;

    while (i < input.len) {
        if (opts.redact_tokens) {
            const tok_end = matchTokenEnd(input, i);
            if (tok_end > i) {
                pos = appendStr(out, pos, maskSecret(input[i..tok_end], tmp[0..128]));
                i = tok_end;
                continue;
            }
        }
        if (opts.redact_emails) {
            const em_end = matchEmailEnd(input, i);
            if (em_end > i) {
                pos = appendStr(out, pos, maskEmail(input[i..em_end], tmp[0..128]));
                i = em_end;
                continue;
            }
        }
        if (opts.redact_cards) {
            const card_end = matchCardEnd(input, i);
            if (card_end > i) {
                pos = appendStr(out, pos, maskCard(input[i..card_end], tmp[0..128]));
                i = card_end;
                continue;
            }
        }
        if (opts.redact_phones) {
            const ph_end = matchPhoneEnd(input, i);
            if (ph_end > i) {
                pos = appendStr(out, pos, maskPhone(input[i..ph_end], tmp[0..128]));
                i = ph_end;
                continue;
            }
        }
        if (opts.redact_ipv4) {
            const ip_end = matchIpv4End(input, i);
            if (ip_end > i) {
                pos = appendStr(out, pos, maskIp(input[i..ip_end], tmp[0..128]));
                i = ip_end;
                continue;
            }
        }

        pos = appendByte(out, pos, input[i]);
        i += 1;
    }
    return out[0..pos];
}

pub fn containsSensitive(input: string) -> bool {
    let i: usize = 0;
    while (i < input.len) {
        if (matchTokenEnd(input, i) > i) {
            return true;
        }
        if (matchCardEnd(input, i) > i) {
            return true;
        }
        if (matchEmailEnd(input, i) > i) {
            return true;
        }
        if (matchPhoneEnd(input, i) > i) {
            return true;
        }
        i += 1;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Sensitive JSON field redactor (fast-redact style)
// ---------------------------------------------------------------------------

fn isSensitiveKey(key: string) -> bool {
    return eqIgnoreCase(key, "password") ||
        eqIgnoreCase(key, "passwd") ||
        eqIgnoreCase(key, "secret") ||
        eqIgnoreCase(key, "token") ||
        eqIgnoreCase(key, "api_key") ||
        eqIgnoreCase(key, "apikey") ||
        eqIgnoreCase(key, "authorization") ||
        eqIgnoreCase(key, "access_token") ||
        eqIgnoreCase(key, "refresh_token") ||
        eqIgnoreCase(key, "private_key") ||
        eqIgnoreCase(key, "credit_card") ||
        eqIgnoreCase(key, "card_number") ||
        eqIgnoreCase(key, "ssn") ||
        eqIgnoreCase(key, "cookie");
}

fn findJsonStringEnd(s: string, open_quote: usize) -> usize {
    let i = open_quote + 1;
    while (i < s.len) {
        if (s[i] == '\\') {
            i += 2;
            continue;
        }
        if (s[i] == '"') {
            return i;
        }
        i += 1;
    }
    return s.len;
}

pub fn redactJsonFields(json_str: string, out: []u8) -> string {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < json_str.len) {
        if (json_str[i] == '"') {
            const key_end = findJsonStringEnd(json_str, i);
            if (key_end < json_str.len) {
                const key = json_str[i + 1..key_end];
                // Look ahead past whitespace for ':'
                let j = key_end + 1;
                while (j < json_str.len && (json_str[j] == ' ' || json_str[j] == '\t' || json_str[j] == '\r' || json_str[j] == '\n')) {
                    j += 1;
                }
                if (j < json_str.len && json_str[j] == ':' && isSensitiveKey(key)) {
                    // Copy key and ':' and whitespace
                    let k = j + 1;
                    while (k < json_str.len && (json_str[k] == ' ' || json_str[k] == '\t' || json_str[k] == '\r' || json_str[k] == '\n')) {
                        k += 1;
                    }
                    pos = appendStr(out, pos, json_str[i..k]);
                    if (k < json_str.len && json_str[k] == '"') {
                        const val_end = findJsonStringEnd(json_str, k);
                        pos = appendStr(out, pos, "\"[REDACTED]\"");
                        if (val_end < json_str.len) {
                            i = val_end + 1;
                        } else {
                            i = json_str.len;
                        }
                        continue;
                    }
                    i = k;
                    continue;
                }
                pos = appendStr(out, pos, json_str[i..key_end + 1]);
                i = key_end + 1;
                continue;
            }
        }
        pos = appendByte(out, pos, json_str[i]);
        i += 1;
    }
    return out[0..pos];
}

export {
    RedactOptions,
    maskEmail,
    maskPhone,
    maskCard,
    maskSecret,
    maskIp,
    redactText,
    containsSensitive,
    redactJsonFields,
};
