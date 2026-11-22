// Source: cp/lib/guardrail/pii.cp (PII & Secret Scanner / Redactor)
// Pure CP zero-dependency PII & credential scanner and redactor with Luhn validation.

const std = @import("std");

/// Error set shared across `cap:guardrail` scanning and policy evaluation operations.
pub err GuardrailError {
    InvalidRule,
    OutOfMemory,
}

/// Classification of detected sensitive entities.
pub enum PiiKind: u8 {
    Email = 0,
    Ipv4 = 1,
    CreditCard = 2,
    ApiKey = 3,
    AwsAccessKey = 4,
    JwtToken = 5,
    Phone = 6,

    /// Returns the canonical uppercase redaction tag for this entity kind.
    fn label(): string {
        switch (_) {
            .Email => return "EMAIL",
            .Ipv4 => return "IPV4",
            .CreditCard => return "CREDIT_CARD",
            .ApiKey => return "API_KEY",
            .AwsAccessKey => return "AWS_ACCESS_KEY",
            .JwtToken => return "JWT_TOKEN",
            .Phone => return "PHONE",
        }
    }
}

/// Single sensitive span detected inside an inspected text.
pub struct PiiFinding {
    kind: PiiKind = .Email;
    start: usize = 0;
    end: usize = 0;
    masked_preview: string = "";
}

/// Output of `PiiScanner.redact` / `PiiScanner.redactTyped`.
pub struct RedactionResult {
    text: string = "";
    finding_count: usize = 0;
    findings: []PiiFinding = [];

    /// Releases all heap allocations owned by this redaction result.
    fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.text.len > 0) {
            alloc.free(_.text);
            _.text = "";
        }
        PiiScanner.freeFindings(alloc, _.findings);
        _.findings = [];
        _.finding_count = 0;
    }
}

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isAsciiAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn isAsciiUpperOrDigit(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

fn isAsciiAlnum(c: u8): bool {
    return isAsciiAlpha(c) || isAsciiDigit(c);
}

fn isIdentChar(c: u8): bool {
    return isAsciiAlnum(c) || c == '_';
}

fn isBase64UrlChar(c: u8): bool {
    return isAsciiAlnum(c) || c == '-' || c == '_';
}

fn isEmailLocalChar(c: u8): bool {
    return isAsciiAlnum(c) || c == '.' || c == '_' || c == '%' || c == '+' || c == '-';
}

fn isDomainLabelChar(c: u8): bool {
    return isAsciiAlnum(c) || c == '-';
}

fn startsWithAt(text: string, pos: usize, prefix: string): bool {
    if (pos + prefix.len > text.len) return false;
    return std.mem.eql(u8, text[pos .. pos + prefix.len], prefix);
}

fn checkBoundaryBefore(text: string, pos: usize): bool {
    if (pos == 0) return true;
    const prev = text[pos - 1];
    return !isIdentChar(prev);
}

fn checkBoundaryAfter(text: string, end: usize): bool {
    if (end >= text.len) return true;
    const next = text[end];
    return !isIdentChar(next);
}

/// Validates a digit-containing string against the Luhn checksum algorithm.
/// Ignores spaces and hyphens; rejects any other non-digit character.
fn validateLuhn(slice: string): bool {
    let digits: [24]u8 = undefined;
    let dlen: usize = 0;
    for (c of slice) {
        if (isAsciiDigit(c)) {
            if (dlen >= 24) return false;
            digits[dlen] = c - '0';
            dlen += 1;
        } else if (c == ' ' || c == '-') {
            continue;
        } else {
            return false;
        }
    }
    if (dlen < 13 || dlen > 19) return false;

    let sum: u32 = 0;
    let double_digit = false;
    let idx: usize = dlen;
    while (idx > 0) {
        idx -= 1;
        let d: u32 = u32(digits[idx]);
        if (double_digit) {
            d *= 2;
            if (d > 9) d -= 9;
        }
        sum += d;
        double_digit = !double_digit;
    }
    return (sum % 10) == 0;
}

/// Matches a 3-segment Base64URL JWT starting with `eyJ`.
fn matchJwt(text: string, pos: usize): usize? {
    if (!startsWithAt(text, pos, "eyJ")) return null;
    if (pos > 0) {
        const prev = text[pos - 1];
        if (isBase64UrlChar(prev) || prev == '.') return null;
    }

    let i: usize = pos;
    let seg_lens: [3]usize = [0, 0, 0];
    let seg_idx: usize = 0;

    while (i < text.len) : (i += 1) {
        const c = text[i];
        if (c == '.') {
            if (seg_idx >= 2) return null;
            if (seg_lens[seg_idx] < 4) return null;
            seg_idx += 1;
        } else if (isBase64UrlChar(c)) {
            seg_lens[seg_idx] += 1;
        } else {
            break;
        }
    }

    if (seg_idx != 2 || seg_lens[2] < 4) return null;
    if (!checkBoundaryAfter(text, i)) return null;
    return i;
}

/// Matches AWS Access Key ID: `AKIA` + 16 uppercase alphanumeric characters.
fn matchAwsAccessKey(text: string, pos: usize): usize? {
    if (!checkBoundaryBefore(text, pos)) return null;
    if (!startsWithAt(text, pos, "AKIA")) return null;
    if (pos + 20 > text.len) return null;
    let i: usize = pos + 4;
    while (i < pos + 20) : (i += 1) {
        if (!isAsciiUpperOrDigit(text[i])) return null;
    }
    if (!checkBoundaryAfter(text, pos + 20)) return null;
    return pos + 20;
}

/// Matches common API keys (`sk-`, `ghp_`, `gho_`, `ghu_`, `ghs_`, `ghr_`, `xoxb-`, `xoxp-`, `xoxa-`, `xoxr-`).
fn matchApiKey(text: string, pos: usize): usize? {
    if (!checkBoundaryBefore(text, pos)) return null;

    let prefix_len: usize = 0;
    let min_tail: usize = 16;
    let allow_dash = false;

    if (startsWithAt(text, pos, "sk-")) {
        prefix_len = 3;
        min_tail = 16;
        allow_dash = true;
    } else if (startsWithAt(text, pos, "ghp_") ||
               startsWithAt(text, pos, "gho_") ||
               startsWithAt(text, pos, "ghu_") ||
               startsWithAt(text, pos, "ghs_") ||
               startsWithAt(text, pos, "ghr_")) {
        prefix_len = 4;
        min_tail = 16;
        allow_dash = false;
    } else if (startsWithAt(text, pos, "xoxb-") ||
               startsWithAt(text, pos, "xoxp-") ||
               startsWithAt(text, pos, "xoxa-") ||
               startsWithAt(text, pos, "xoxr-")) {
        prefix_len = 5;
        min_tail = 12;
        allow_dash = true;
    } else {
        return null;
    }

    let i: usize = pos + prefix_len;
    while (i < text.len) : (i += 1) {
        const c = text[i];
        if (isIdentChar(c) || (allow_dash && c == '-')) {
            continue;
        }
        break;
    }

    if (i - (pos + prefix_len) < min_tail) return null;
    if (!checkBoundaryAfter(text, i)) return null;
    return i;
}

/// Matches RFC-like email address with strict domain and TLD checks.
fn matchEmail(text: string, pos: usize): usize? {
    const first = text[pos];
    if (!isAsciiAlnum(first)) return null;
    if (pos > 0) {
        const prev = text[pos - 1];
        if (isEmailLocalChar(prev)) return null;
    }

    let i: usize = pos;
    let prev_dot = false;
    while (i < text.len) : (i += 1) {
        const c = text[i];
        if (c == '@') break;
        if (!isEmailLocalChar(c)) return null;
        if (c == '.') {
            if (prev_dot) return null;
            prev_dot = true;
        } else {
            prev_dot = false;
        }
    }

    if (i >= text.len || text[i] != '@' || i == pos || prev_dot) return null;
    i += 1; // skip '@'
    if (i >= text.len) return null;

    let label_count: usize = 0;
    let cur_label_len: usize = 0;
    let last_label_start: usize = i;
    let last_char: u8 = 0;

    while (i < text.len) : (i += 1) {
        const c = text[i];
        if (c == '.') {
            if (cur_label_len == 0 || last_char == '-') return null;
            label_count += 1;
            cur_label_len = 0;
            last_label_start = i + 1;
            last_char = '.';
        } else if (isDomainLabelChar(c)) {
            if (cur_label_len == 0 && c == '-') return null;
            cur_label_len += 1;
            last_char = c;
        } else {
            break;
        }
    }

    if (cur_label_len < 2 || last_char == '-' || label_count < 1) return null;
    // TLD must be alphabetic (e.g., .com, .org, .io)
    let k: usize = last_label_start;
    while (k < i) : (k += 1) {
        if (!isAsciiAlpha(text[k])) return null;
    }
    if (i < text.len && (isIdentChar(text[i]) || text[i] == '-')) return null;
    return i;
}

/// Matches a Luhn-validated credit/debit card number (13..19 digits with optional single space/dash separators).
fn matchCreditCard(text: string, pos: usize): usize? {
    if (!isAsciiDigit(text[pos])) return null;
    if (pos > 0) {
        const prev = text[pos - 1];
        if (isAsciiAlnum(prev) || prev == '.' || prev == '_') return null;
    }

    let i: usize = pos;
    let digit_count: usize = 0;
    let prev_sep = false;

    while (i < text.len) : (i += 1) {
        const c = text[i];
        if (isAsciiDigit(c)) {
            digit_count += 1;
            prev_sep = false;
            if (digit_count > 19) return null;
        } else if (c == ' ' || c == '-') {
            if (prev_sep || digit_count == 0) break;
            prev_sep = true;
        } else {
            break;
        }
    }

    // Trim trailing separator if loop stopped right after one.
    if (prev_sep && i > pos) {
        i -= 1;
    }

    if (digit_count < 13 || digit_count > 19) return null;
    if (i < text.len) {
        const next = text[i];
        if (isAsciiAlnum(next) || next == '.' || next == '_') return null;
    }

    if (!validateLuhn(text[pos..i])) return null;
    return i;
}

/// Matches dotted-quad IPv4 address (`0.0.0.0` .. `255.255.255.255`) without leading zeros.
fn matchIpv4(text: string, pos: usize): usize? {
    if (!isAsciiDigit(text[pos])) return null;
    if (pos > 0) {
        const prev = text[pos - 1];
        if (isAsciiAlnum(prev) || prev == '.' || prev == '_') return null;
    }

    let i: usize = pos;
    let octets: usize = 0;

    while (octets < 4) : (octets += 1) {
        if (i >= text.len || !isAsciiDigit(text[i])) return null;
        const start_oct = i;
        let val: u32 = 0;
        let len_oct: usize = 0;
        while (i < text.len && isAsciiDigit(text[i])) : (i += 1) {
            val = val * 10 + u32(text[i] - '0');
            len_oct += 1;
            if (len_oct > 3 || val > 255) return null;
        }
        // Reject leading zeros such as "01" or "001"
        if (len_oct > 1 && text[start_oct] == '0') return null;

        if (octets < 3) {
            if (i >= text.len || text[i] != '.') return null;
            i += 1;
        }
    }

    if (i < text.len) {
        const next = text[i];
        if (isAsciiAlnum(next) || next == '.' || next == '_') return null;
    }
    return i;
}

/// Matches formatted phone numbers (`+...`, `(xxx) xxx-xxxx`, or `xxx-xxx-xxxx`) while avoiding bare integer false positives.
fn matchPhone(text: string, pos: usize): usize? {
    const first = text[pos];
    if (first != '+' && first != '(' && !isAsciiDigit(first)) return null;
    if (pos > 0) {
        const prev = text[pos - 1];
        if (isAsciiAlnum(prev) || prev == '.' || prev == '_') return null;
    }

    let i: usize = pos;
    let has_plus = false;
    let has_sep = false;
    let digit_count: usize = 0;

    if (first == '+') {
        has_plus = true;
        i += 1;
        if (i >= text.len || !isAsciiDigit(text[i])) return null;
    }

    let prev_sep = false;
    while (i < text.len) : (i += 1) {
        const c = text[i];
        if (isAsciiDigit(c)) {
            digit_count += 1;
            prev_sep = false;
            if (digit_count > 15) return null;
        } else if (c == '-' || c == ' ' || c == '(' || c == ')') {
            if (c == '-' && prev_sep) break;
            has_sep = true;
            prev_sep = (c == '-' || c == ' ');
        } else {
            break;
        }
    }

    while (i > pos && (text[i - 1] == ' ' || text[i - 1] == '-')) {
        i -= 1;
    }

    if (digit_count < 10 || digit_count > 15) return null;
    if (!has_plus && !has_sep) return null;
    if (i < text.len) {
        const next = text[i];
        if (isAsciiAlnum(next) || next == '_') return null;
    }
    return i;
}

fn buildMaskedPreview(alloc: std.mem.Allocator, kind: PiiKind, raw: string): string ! GuardrailError {
    let buf: std.ArrayList(u8) = .empty;
    errdefer buf.deinit(alloc);

    if (raw.len <= 4) {
        buf.appendSlice(alloc, "****") catch return GuardrailError.OutOfMemory;
        return buf.toOwnedSlice(alloc) catch return GuardrailError.OutOfMemory;
    }

    switch (kind) {
        .Email => {
            const at_idx = std.mem.indexOfScalar(u8, raw, '@') ?? 0;
            buf.append(alloc, raw[0]) catch return GuardrailError.OutOfMemory;
            buf.appendSlice(alloc, "***") catch return GuardrailError.OutOfMemory;
            if (at_idx < raw.len) {
                buf.appendSlice(alloc, raw[at_idx..]) catch return GuardrailError.OutOfMemory;
            }
        },
        .CreditCard => {
            let tail: [4]u8 = undefined;
            let tlen: usize = 0;
            let j: usize = raw.len;
            while (j > 0 && tlen < 4) {
                j -= 1;
                if (isAsciiDigit(raw[j])) {
                    tail[3 - tlen] = raw[j];
                    tlen += 1;
                }
            }
            buf.appendSlice(alloc, "****-****-****-") catch return GuardrailError.OutOfMemory;
            if (tlen == 4) {
                buf.appendSlice(alloc, tail[0..4]) catch return GuardrailError.OutOfMemory;
            } else {
                buf.appendSlice(alloc, raw[raw.len - 4 ..]) catch return GuardrailError.OutOfMemory;
            }
        },
        else => {
            if (raw.len >= 10) {
                buf.appendSlice(alloc, raw[0..3]) catch return GuardrailError.OutOfMemory;
                buf.appendSlice(alloc, "***") catch return GuardrailError.OutOfMemory;
                buf.appendSlice(alloc, raw[raw.len - 4 ..]) catch return GuardrailError.OutOfMemory;
            } else {
                buf.appendSlice(alloc, "***") catch return GuardrailError.OutOfMemory;
                buf.appendSlice(alloc, raw[raw.len - 2 ..]) catch return GuardrailError.OutOfMemory;
            }
        },
    }

    return buf.toOwnedSlice(alloc) catch return GuardrailError.OutOfMemory;
}

/// Stateless scanner and redactor for PII and high-risk credentials.
pub static class PiiScanner {
    /// Returns true if `digits_or_formatted` passes the Luhn checksum validation.
    fn isValidLuhn(text: string): bool {
        return validateLuhn(text);
    }

    /// Scans `text` and returns all non-overlapping sensitive findings in left-to-right order.
    fn scan(alloc: std.mem.Allocator, text: string): []PiiFinding ! GuardrailError {
        let findings: std.ArrayList(PiiFinding) = .empty;
        errdefer {
            for (f of findings.items) {
                if (f.masked_preview.len > 0) alloc.free(f.masked_preview);
            }
            findings.deinit(alloc);
        }

        let pos: usize = 0;
        while (pos < text.len) {
            let matched_kind: PiiKind = .Email;
            let matched_end: usize? = null;

            if (end_jwt := matchJwt(text, pos)) {
                matched_kind = .JwtToken;
                matched_end = end_jwt;
            } else if (end_aws := matchAwsAccessKey(text, pos)) {
                matched_kind = .AwsAccessKey;
                matched_end = end_aws;
            } else if (end_key := matchApiKey(text, pos)) {
                matched_kind = .ApiKey;
                matched_end = end_key;
            } else if (end_email := matchEmail(text, pos)) {
                matched_kind = .Email;
                matched_end = end_email;
            } else if (end_cc := matchCreditCard(text, pos)) {
                matched_kind = .CreditCard;
                matched_end = end_cc;
            } else if (end_ip := matchIpv4(text, pos)) {
                matched_kind = .Ipv4;
                matched_end = end_ip;
            } else if (end_phone := matchPhone(text, pos)) {
                matched_kind = .Phone;
                matched_end = end_phone;
            }

            if (end_pos := matched_end) {
                const preview = buildMaskedPreview(alloc, matched_kind, text[pos..end_pos])?;
                findings.append(alloc, PiiFinding{
                    kind: matched_kind,
                    start: pos,
                    end: end_pos,
                    masked_preview: preview,
                }) catch {
                    alloc.free(preview);
                    return GuardrailError.OutOfMemory;
                };
                pos = end_pos;
            } else {
                pos += 1;
            }
        }

        return findings.toOwnedSlice(alloc) catch return GuardrailError.OutOfMemory;
    }

    /// Frees a slice of findings previously returned by `PiiScanner.scan`.
    fn freeFindings(alloc: std.mem.Allocator, findings: []PiiFinding) {
        for (f of findings) {
            if (f.masked_preview.len > 0) {
                alloc.free(f.masked_preview);
            }
        }
        if (findings.len > 0) {
            alloc.free(findings);
        }
    }

    /// Replaces all detected sensitive spans with a fixed `mask_token`.
    fn redact(alloc: std.mem.Allocator, text: string, mask_token: string): RedactionResult ! GuardrailError {
        if (mask_token.len == 0) return GuardrailError.InvalidRule;
        const findings = PiiScanner.scan(alloc, text)?;
        errdefer PiiScanner.freeFindings(alloc, findings);

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let cursor: usize = 0;
        for (f of findings) {
            if (f.start > cursor) {
                out.appendSlice(alloc, text[cursor..f.start]) catch return GuardrailError.OutOfMemory;
            }
            out.appendSlice(alloc, mask_token) catch return GuardrailError.OutOfMemory;
            cursor = f.end;
        }
        if (cursor < text.len) {
            out.appendSlice(alloc, text[cursor..]) catch return GuardrailError.OutOfMemory;
        }

        const owned_text = out.toOwnedSlice(alloc) catch return GuardrailError.OutOfMemory;
        return RedactionResult{
            text: owned_text,
            finding_count: findings.len,
            findings: findings,
        };
    }

    /// Replaces each detected sensitive span with a typed placeholder such as `[REDACTED:API_KEY]`.
    fn redactTyped(alloc: std.mem.Allocator, text: string): RedactionResult ! GuardrailError {
        const findings = PiiScanner.scan(alloc, text)?;
        errdefer PiiScanner.freeFindings(alloc, findings);

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let cursor: usize = 0;
        for (f of findings) {
            if (f.start > cursor) {
                out.appendSlice(alloc, text[cursor..f.start]) catch return GuardrailError.OutOfMemory;
            }
            out.appendSlice(alloc, "[REDACTED:") catch return GuardrailError.OutOfMemory;
            out.appendSlice(alloc, f.kind.label()) catch return GuardrailError.OutOfMemory;
            out.append(alloc, ']') catch return GuardrailError.OutOfMemory;
            cursor = f.end;
        }
        if (cursor < text.len) {
            out.appendSlice(alloc, text[cursor..]) catch return GuardrailError.OutOfMemory;
        }

        const owned_text = out.toOwnedSlice(alloc) catch return GuardrailError.OutOfMemory;
        return RedactionResult{
            text: owned_text,
            finding_count: findings.len,
            findings: findings,
        };
    }
}
