// Origin: Pure-CP Stdlib (AI Prompt Safety: Heuristic Boundary Check & PII Redactor) | Created: 2026-11-22
// Zero-heap-allocation heuristic prompt boundary inspector and PII scanner/redactor.

/// Bitmask flags describing heuristic boundary findings and PII presence in prompt text.
pub struct PromptRiskFlags {
    bits: u32 = 0;

    pub static const NONE: u32 = 0;
    pub static const ROLE_OVERRIDE: u32 = 1 << 0;
    pub static const SYSTEM_LEAK_PROBE: u32 = 1 << 1;
    pub static const DELIMITER_ESCAPE: u32 = 1 << 2;
    pub static const CONTROL_CHAR_SMUGGLING: u32 = 1 << 3;
    pub static const PII_DETECTED: u32 = 1 << 4;
    pub static const ALL_KNOWN: u32 = 0x1F;

    /// Creates a flag set from raw bits, masking out unknown high bits.
    static fn fromBits(raw: u32): PromptRiskFlags {
        return PromptRiskFlags{
            bits: raw & PromptRiskFlags.ALL_KNOWN,
        };
    }

    /// Returns `true` if all bits in `mask` are set.
    fn has(mask: u32): bool {
        return (_.bits & mask) == mask;
    }

    /// Returns `true` if any bit in `mask` is set.
    fn hasAny(mask: u32): bool {
        return (_.bits & mask) != 0;
    }

    /// Returns a new flag set with `mask` bits enabled.
    fn with(mask: u32): PromptRiskFlags {
        return PromptRiskFlags{
            bits: (_.bits | mask) & PromptRiskFlags.ALL_KNOWN,
        };
    }

    /// Returns `true` when no risk flags are set.
    fn isEmpty(): bool {
        return _.bits == 0;
    }
}

/// Summary report produced by `inspectPromptSafety`.
pub struct PromptSafetyReport {
    flags: PromptRiskFlags = PromptRiskFlags{};
    risk_score: u32 = 0;
    matched_rules: u32 = 0;
    pii_hits: u32 = 0;
    blocked: bool = false;
}

/// Category of sensitive personal or credential span detected by `scanPiiSpans`.
pub enum PiiKind {
    Email,
    ApiKey,
    Ipv4,
    Phone,
}

/// Half-open byte range `[start, end)` of a detected PII span inside the input string.
pub struct PiiSpan {
    kind: PiiKind = PiiKind.Email;
    start: usize = 0;
    end: usize = 0;
}

const REDACTED_TOKEN: string = "[REDACTED]";

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn isAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isAlnum(c: u8): bool {
    return isAlpha(c) || isDigit(c);
}

fn isKeyTokenChar(c: u8): bool {
    return isAlnum(c) || c == '_' || c == '-';
}

fn isEmailLocalChar(c: u8): bool {
    return isAlnum(c) || c == '.' || c == '_' || c == '%' || c == '+' || c == '-';
}

fn isDomainLabelChar(c: u8): bool {
    return isAlnum(c) || c == '-';
}

fn startsWithAt(s: string, pos: usize, prefix: string): bool {
    if (pos + prefix.len > s.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (s[pos + i] != prefix[i]) return false;
    }
    return true;
}

fn containsIgnoreCase(haystack: string, needle: string): bool {
    if (needle.len == 0) return false;
    if (needle.len > haystack.len) return false;
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) : (i += 1) {
        let j: usize = 0;
        let matched = true;
        while (j < needle.len) : (j += 1) {
            if (asciiLower(haystack[i + j]) != asciiLower(needle[j])) {
                matched = false;
                break;
            }
        }
        if (matched) return true;
    }
    return false;
}

/// Attempts to match a known credential prefix (`sk-`, `ghp_`, `AKIA`, `xoxb-`, `xoxp-`) at `pos`.
fn matchApiKeyAt(s: string, pos: usize): usize? {
    if (pos > 0 && isKeyTokenChar(s[pos - 1])) {
        return null;
    }

    if (startsWithAt(s, pos, "sk-")) {
        let end = pos + 3;
        while (end < s.len && isKeyTokenChar(s[end])) : (end += 1) {}
        if (end - (pos + 3) >= 12) return end;
        return null;
    }
    if (startsWithAt(s, pos, "ghp_")) {
        let end = pos + 4;
        while (end < s.len && (isAlnum(s[end]) || s[end] == '_')) : (end += 1) {}
        if (end - (pos + 4) >= 12) return end;
        return null;
    }
    if (startsWithAt(s, pos, "AKIA")) {
        let end = pos + 4;
        while (end < s.len && ((s[end] >= 'A' && s[end] <= 'Z') || isDigit(s[end]))) : (end += 1) {}
        if (end - (pos + 4) >= 12) return end;
        return null;
    }
    if (startsWithAt(s, pos, "xoxb-") || startsWithAt(s, pos, "xoxp-")) {
        let end = pos + 5;
        while (end < s.len && isKeyTokenChar(s[end])) : (end += 1) {}
        if (end - (pos + 5) >= 10) return end;
        return null;
    }
    return null;
}

/// Attempts to match an email address starting at `pos`.
fn matchEmailAt(s: string, pos: usize): usize? {
    if (pos >= s.len || !isAlnum(s[pos])) return null;
    if (pos > 0 && (isEmailLocalChar(s[pos - 1]) || s[pos - 1] == '@')) {
        return null;
    }

    let i = pos;
    while (i < s.len && isEmailLocalChar(s[i])) : (i += 1) {}
    if (i == pos || i >= s.len || s[i] != '@') return null;

    // Parse domain labels separated by '.'
    i += 1;
    const domain_start = i;
    let label_len: usize = 0;
    let last_label_start = i;
    let dot_count: usize = 0;

    while (i < s.len) {
        const c = s[i];
        if (isDomainLabelChar(c)) {
            if (label_len == 0 && c == '-') return null;
            label_len += 1;
            i += 1;
        } else if (c == '.') {
            if (label_len == 0) break;
            if (s[i - 1] == '-') return null;
            // Check if next char starts a valid label; otherwise stop before trailing sentence dot
            if (i + 1 >= s.len || !isAlnum(s[i + 1])) break;
            dot_count += 1;
            label_len = 0;
            i += 1;
            last_label_start = i;
        } else {
            break;
        }
    }

    if (dot_count == 0 || i <= domain_start || label_len < 2) return null;
    if (s[i - 1] == '-') return null;

    // Validate TLD consists purely of ASCII letters
    let k = last_label_start;
    while (k < i) : (k += 1) {
        if (!isAlpha(s[k])) return null;
    }
    return i;
}

/// Attempts to match a strict IPv4 dotted-quad (`0..255` x 4) starting at `pos`.
fn matchIpv4At(s: string, pos: usize): usize? {
    if (pos >= s.len || !isDigit(s[pos])) return null;
    if (pos > 0 && (isAlnum(s[pos - 1]) || s[pos - 1] == '.')) {
        return null;
    }

    let i = pos;
    let octet: usize = 0;
    while (octet < 4) : (octet += 1) {
        if (i >= s.len || !isDigit(s[i])) return null;
        const start_d = i;
        let val: u32 = 0;
        let digits: usize = 0;
        while (i < s.len && isDigit(s[i])) : (i += 1) {
            if (digits >= 3) return null;
            val = val * 10 + u32(s[i] - '0');
            digits += 1;
        }
        if (digits == 0 || val > 255) return null;
        if (digits > 1 && s[start_d] == '0') return null;

        if (octet < 3) {
            if (i >= s.len || s[i] != '.') return null;
            i += 1;
        }
    }

    if (i < s.len && (isAlnum(s[i]) || s[i] == '.')) {
        return null;
    }
    return i;
}

/// Attempts to match a phone number (international or formatted 10..15 digits) starting at `pos`.
fn matchPhoneAt(s: string, pos: usize): usize? {
    if (pos >= s.len) return null;
    const first = s[pos];
    if (!isDigit(first) && first != '+' && first != '(') return null;
    if (pos > 0 && (isAlnum(s[pos - 1]) || s[pos - 1] == '+' || s[pos - 1] == '.' || s[pos - 1] == '_')) {
        return null;
    }

    let i = pos;
    let has_plus = false;
    let has_sep = false;
    let digit_count: usize = 0;
    let last_digit_end: usize = pos;

    if (s[i] == '+') {
        has_plus = true;
        i += 1;
        if (i >= s.len || !isDigit(s[i])) return null;
    }

    while (i < s.len) {
        const c = s[i];
        if (isDigit(c)) {
            digit_count += 1;
            if (digit_count > 15) return null;
            i += 1;
            last_digit_end = i;
        } else if (c == '-' || c == ' ' || c == '(' || c == ')') {
            has_sep = true;
            i += 1;
        } else {
            break;
        }
    }

    if (digit_count < 10 || digit_count > 15) return null;
    if (!has_plus && !has_sep && digit_count != 11) return null;
    if (last_digit_end < s.len && (isAlnum(s[last_digit_end]) || s[last_digit_end] == '_')) {
        return null;
    }
    return last_digit_end;
}

fn matchNextPiiAt(s: string, pos: usize): PiiSpan? {
    if (end := matchApiKeyAt(s, pos)) {
        return PiiSpan{ kind: PiiKind.ApiKey, start: pos, end: end };
    }
    if (end := matchEmailAt(s, pos)) {
        return PiiSpan{ kind: PiiKind.Email, start: pos, end: end };
    }
    if (end := matchIpv4At(s, pos)) {
        return PiiSpan{ kind: PiiKind.Ipv4, start: pos, end: end };
    }
    if (end := matchPhoneAt(s, pos)) {
        return PiiSpan{ kind: PiiKind.Phone, start: pos, end: end };
    }
    return null;
}

/// Scans `input` for non-overlapping PII spans (`ApiKey`, `Email`, `Ipv4`, `Phone`).
/// Writes up to `out_spans.len` matches into `out_spans` and returns the total number of matches found.
pub fn scanPiiSpans(input: string, out_spans: *[]PiiSpan): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (span := matchNextPiiAt(input, i)) {
            if (total < out_spans.len) {
                out_spans[total] = span;
            }
            total += 1;
            i = span.end;
        } else {
            i += 1;
        }
    }
    return total;
}

fn countPiiHits(input: string): u32 {
    let total: u32 = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (span := matchNextPiiAt(input, i)) {
            total += 1;
            i = span.end;
        } else {
            i += 1;
        }
    }
    return total;
}

/// Zero-heap-allocation PII redactor that replaces detected emails, API keys, IPv4 addresses,
/// and phone numbers with `[REDACTED]`.
/// Returns the number of bytes written to `out_buf`, or `null` if `out_buf` is too small.
pub fn redactPii(input: string, out_buf: *[]u8): usize? {
    let out_pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (span := matchNextPiiAt(input, i)) {
            if (out_pos + REDACTED_TOKEN.len > out_buf.len) {
                return null;
            }
            let k: usize = 0;
            while (k < REDACTED_TOKEN.len) : (k += 1) {
                out_buf[out_pos + k] = REDACTED_TOKEN[k];
            }
            out_pos += REDACTED_TOKEN.len;
            i = span.end;
        } else {
            if (out_pos >= out_buf.len) {
                return null;
            }
            out_buf[out_pos] = input[i];
            out_pos += 1;
            i += 1;
        }
    }
    return out_pos;
}

/// Detects non-printable ASCII control bytes (excluding `\t`, `\n`, `\r`) and invisible UTF-8
/// zero-width / bidirectional override sequences (`U+200B..U+200F`, `U+202A..U+202E`, `U+2060..U+2064`, `U+FEFF`).
fn hasHiddenControlChars(input: string): bool {
    let i: usize = 0;
    while (i < input.len) : (i += 1) {
        const b = input[i];
        if ((b < 0x20 && b != 0x09 && b != 0x0A && b != 0x0D) || b == 0x7F) {
            return true;
        }
        if (b == 0xE2 && i + 2 < input.len) {
            const b1 = input[i + 1];
            const b2 = input[i + 2];
            if (b1 == 0x80 && ((b2 >= 0x8B && b2 <= 0x8F) || (b2 >= 0xAA && b2 <= 0xAE))) {
                return true;
            }
            if (b1 == 0x81 && b2 >= 0xA0 && b2 <= 0xA4) {
                return true;
            }
        }
        if (b == 0xEF && i + 2 < input.len && input[i + 1] == 0xBB && input[i + 2] == 0xBF) {
            return true;
        }
    }
    return false;
}

/// Performs zero-heap-allocation heuristic boundary risk inspection on `input`.
/// Evaluates role-override phrases, system-prompt disclosure probes, model delimiter escapes,
/// hidden control/zero-width characters, and PII presence.
/// Sets `report.blocked = true` when `block_threshold > 0 && report.risk_score >= block_threshold`.
pub fn inspectPromptSafety(input: string, block_threshold: u32): PromptSafetyReport {
    let report = PromptSafetyReport{};
    if (input.len == 0) {
        return report;
    }

    let score: u32 = 0;

    // 1. Role override heuristic phrases (case-insensitive)
    if (
        containsIgnoreCase(input, "ignore previous instructions") ||
        containsIgnoreCase(input, "ignore all previous") ||
        containsIgnoreCase(input, "disregard previous instructions") ||
        containsIgnoreCase(input, "override system instructions") ||
        containsIgnoreCase(input, "forget your instructions") ||
        containsIgnoreCase(input, "you are now in developer mode")
    ) {
        report.flags = report.flags.with(PromptRiskFlags.ROLE_OVERRIDE);
        report.matched_rules += 1;
        score += 40;
    }

    // 2. System prompt disclosure probe phrases (case-insensitive)
    if (
        containsIgnoreCase(input, "reveal your system prompt") ||
        containsIgnoreCase(input, "show your system prompt") ||
        containsIgnoreCase(input, "print your system instructions") ||
        containsIgnoreCase(input, "repeat the above instructions") ||
        containsIgnoreCase(input, "output your hidden prompt")
    ) {
        report.flags = report.flags.with(PromptRiskFlags.SYSTEM_LEAK_PROBE);
        report.matched_rules += 1;
        score += 35;
    }

    // 3. Special template delimiter escape markers (case-insensitive)
    if (
        containsIgnoreCase(input, "<|im_start|>") ||
        containsIgnoreCase(input, "<|im_end|>") ||
        containsIgnoreCase(input, "<|system|>") ||
        containsIgnoreCase(input, "<|assistant|>") ||
        containsIgnoreCase(input, "<|start_header_id|>") ||
        containsIgnoreCase(input, "<|eot_id|>") ||
        containsIgnoreCase(input, "[inst]") ||
        containsIgnoreCase(input, "[/inst]") ||
        containsIgnoreCase(input, "</system>")
    ) {
        report.flags = report.flags.with(PromptRiskFlags.DELIMITER_ESCAPE);
        report.matched_rules += 1;
        score += 45;
    }

    // 4. Hidden ASCII control bytes and zero-width / bidirectional UTF-8 controls
    if (hasHiddenControlChars(input)) {
        report.flags = report.flags.with(PromptRiskFlags.CONTROL_CHAR_SMUGGLING);
        report.matched_rules += 1;
        score += 30;
    }

    // 5. PII & credential token presence
    const pii_hits = countPiiHits(input);
    if (pii_hits > 0) {
        report.flags = report.flags.with(PromptRiskFlags.PII_DETECTED);
        report.matched_rules += 1;
        report.pii_hits = pii_hits;
        const capped_hits: u32 = if (pii_hits > 3) 3 else pii_hits;
        score += capped_hits * 20;
    }

    report.risk_score = if (score > 100) 100 else score;
    report.blocked = block_threshold > 0 && report.risk_score >= block_threshold;
    return report;
}
