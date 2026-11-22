// Origin: Pure-CP Stdlib (Zero-Alloc Prompt Guardrail Compliance & Sensitive Credential Redaction Scanner) | Created: 2026-11-22
// Pure CP zero-heap-allocation Prompt Input/Output Compliance Guardrail & PII/Credential Redaction Scanner:
//   - Detects Email addresses, API credential prefixes (`sk-`, `ghp_`, `AKIA`, `Bearer `), 3-segment JWT tokens,
//     Luhn-verified 13..19-digit payment card numbers, RFC 1918 / loopback private IPv4 addresses,
//     control/zero-width character ratio anomalies, and custom case-insensitive term lists
//   - Configurable weighted `GuardrailPolicy` (`allow | redact | block`) with max-length truncation guard
//   - Zero-alloc `redactSensitiveSpans` with automatic overlapping-span merging and typed `[REDACTED:*]` markers

/// Maximum findings collected in a single guardrail scan pass (zero dynamic heap).
pub const GUARDRAIL_MAX_FINDINGS: usize = 64;

/// Maximum custom sensitive terms registered on `PromptGuardrailScanner`.
pub const GUARDRAIL_MAX_CUSTOM_TERMS: usize = 16;

/// Maximum byte length of a single custom sensitive term.
pub const GUARDRAIL_MAX_TERM_LEN: usize = 64;

// ============================================================================
// 1. Public Enums & Data Models
// ============================================================================

/// Category of a compliance or sensitive-data finding.
pub enum GuardrailCategory {
    email,
    api_secret,
    jwt_token,
    payment_card,
    private_ipv4,
    control_anomaly,
    custom_term,
    length_exceeded,
}

/// Final compliance disposition emitted by `PromptGuardrailScanner.scan`.
pub enum GuardrailAction {
    allow,
    redact,
    block,
}

/// Byte span `[start, end)` and risk weight for a single finding.
pub struct GuardrailFinding {
    category: GuardrailCategory = GuardrailCategory.email;
    start: usize = 0;
    end: usize = 0;
    weight: u32 = 0;
}

/// Configurable rule weights, thresholds, and input limits for `PromptGuardrailScanner`.
pub struct GuardrailPolicy {
    max_input_bytes: usize = 65536;
    /// Maximum tolerated control + zero-width character ratio in basis points (`0..=10000`, default 200 = 2%).
    max_control_ratio_bps: u16 = 200;
    redact_score_threshold: u32 = 20;
    block_score_threshold: u32 = 80;
    weight_email: u32 = 20;
    weight_api_secret: u32 = 90;
    weight_jwt_token: u32 = 85;
    weight_payment_card: u32 = 95;
    weight_private_ipv4: u32 = 25;
    weight_control_anomaly: u32 = 60;
    weight_custom_term: u32 = 50;
    weight_length_exceeded: u32 = 80;

    /// Returns production-tuned default compliance policy.
    static fn defaults(): GuardrailPolicy {
        return GuardrailPolicy{};
    }

    /// Validates basis-point ratio and monotonic threshold ordering (`redact <= block`).
    fn isValid(): bool {
        if (_.max_control_ratio_bps > 10000) return false;
        if (_.redact_score_threshold > _.block_score_threshold) return false;
        return true;
    }
}

/// Aggregate compliance audit report returned by `PromptGuardrailScanner.scan`.
pub struct GuardrailScanReport {
    action: GuardrailAction = GuardrailAction.allow;
    risk_score: u32 = 0;
    scanned_bytes: usize = 0;
    truncated_input: bool = false;
    finding_count: usize = 0;
    control_anomaly_count: usize = 0;
    control_ratio_bps: u16 = 0;
    email_hits: usize = 0;
    secret_hits: usize = 0;
    jwt_hits: usize = 0;
    card_hits: usize = 0;
    private_ip_hits: usize = 0;
    custom_term_hits: usize = 0;
}

// ============================================================================
// 2. Character, Luhn & Pattern Helpers
// ============================================================================

fn isAsciiDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn isAsciiAlpha(c: u8): bool {
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
}

fn isAsciiAlnum(c: u8): bool {
    return isAsciiAlpha(c) || isAsciiDigit(c);
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 65 && c <= 90) return c + 32;
    return c;
}

fn isTokenChar(c: u8): bool {
    return isAsciiAlnum(c) || c == 95 || c == 45 || c == 46; // '_', '-', '.'
}

fn isBase64UrlChar(c: u8): bool {
    return isAsciiAlnum(c) || c == 45 || c == 95; // '-', '_'
}

fn isEmailLocalChar(c: u8): bool {
    return isAsciiAlnum(c) || c == 46 || c == 95 || c == 45 || c == 43; // '.', '_', '-', '+'
}

fn isEmailDomainChar(c: u8): bool {
    return isAsciiAlnum(c) || c == 45 || c == 46; // '-', '.'
}

fn hasPrefixAt(s: []const u8, pos: usize, prefix: []const u8): bool {
    if (pos + prefix.len > s.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (s[pos + i] != prefix[i]) return false;
    }
    return true;
}

fn eqIgnoreCaseAt(s: []const u8, pos: usize, term: []const u8): bool {
    if (pos + term.len > s.len) return false;
    let i: usize = 0;
    while (i < term.len) : (i += 1) {
        if (toLowerAscii(s[pos + i]) != toLowerAscii(term[i])) return false;
    }
    return true;
}

/// Performs standard Luhn checksum validation on a slice of ASCII digit bytes (`'0'..'9'`, length 13..19).
pub fn luhnCheckDigits(digits: []const u8): bool {
    if (digits.len < 13 || digits.len > 19) return false;
    let sum: u32 = 0;
    let double_next: bool = false;
    let i: usize = digits.len;
    while (i > 0) : (i -= 1) {
        const ch: u8 = digits[i - 1];
        if (!isAsciiDigit(ch)) return false;
        let d: u32 = (ch - 48) as u32;
        if (double_next) {
            d *= 2;
            if (d > 9) d -= 9;
        }
        sum += d;
        double_next = !double_next;
    }
    return (sum % 10) == 0;
}

fn isPrivateIpv4Octets(a: u32, b: u32, c: u32, d: u32): bool {
    if (a > 255 || b > 255 || c > 255 || d > 255) return false;
    if (a == 10) return true;                          // 10.0.0.0/8
    if (a == 127) return true;                         // 127.0.0.0/8 loopback
    if (a == 172 && b >= 16 && b <= 31) return true;   // 172.16.0.0/12
    if (a == 192 && b == 168) return true;             // 192.168.0.0/16
    return false;
}

fn redactionLabelFor(cat: GuardrailCategory): []const u8 {
    if (cat == GuardrailCategory.email) return "[REDACTED:EMAIL]";
    if (cat == GuardrailCategory.api_secret) return "[REDACTED:SECRET]";
    if (cat == GuardrailCategory.jwt_token) return "[REDACTED:JWT]";
    if (cat == GuardrailCategory.payment_card) return "[REDACTED:CARD]";
    if (cat == GuardrailCategory.private_ipv4) return "[REDACTED:PRIVATE_IP]";
    if (cat == GuardrailCategory.custom_term) return "[REDACTED:TERM]";
    return "";
}

fn appendBytes(out: *[]u8, pos: *usize, src: []const u8): bool {
    if (pos.* > out.len || src.len > out.len - pos.*) return false;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out[pos.* + i] = src[i];
    }
    pos.* += src.len;
    return true;
}

/// Rewrites `input` into `out_buf`, replacing all valid sensitive `findings` spans with typed
/// `[REDACTED:*]` placeholders. Automatically sorts and merges overlapping spans (keeping the
/// higher-weight category label). Returns written byte count, or `null` if `out_buf` is too small.
pub fn redactSensitiveSpans(
    input: []const u8,
    findings: []const GuardrailFinding,
    out_buf: *[]u8,
): usize? {
    let spans: [GUARDRAIL_MAX_FINDINGS]GuardrailFinding = [GuardrailFinding{}; GUARDRAIL_MAX_FINDINGS];
    let span_cnt: usize = 0;

    let i: usize = 0;
    while (i < findings.len && span_cnt < GUARDRAIL_MAX_FINDINGS) : (i += 1) {
        const f = findings[i];
        if (f.category == GuardrailCategory.control_anomaly || f.category == GuardrailCategory.length_exceeded) {
            continue;
        }
        if (f.start >= f.end || f.end > input.len) continue;
        spans[span_cnt] = f;
        span_cnt += 1;
    }

    // Insertion-sort spans by (start asc, end desc)
    let a: usize = 1;
    while (a < span_cnt) : (a += 1) {
        const key = spans[a];
        let b: usize = a;
        while (b > 0 && (spans[b - 1].start > key.start ||
            (spans[b - 1].start == key.start && spans[b - 1].end < key.end))) : (b -= 1) {
            spans[b] = spans[b - 1];
        }
        spans[b] = key;
    }

    // Merge overlapping spans
    let merged: [GUARDRAIL_MAX_FINDINGS]GuardrailFinding = [GuardrailFinding{}; GUARDRAIL_MAX_FINDINGS];
    let merged_cnt: usize = 0;
    let m: usize = 0;
    while (m < span_cnt) : (m += 1) {
        const cur = spans[m];
        if (merged_cnt == 0 || cur.start > merged[merged_cnt - 1].end) {
            merged[merged_cnt] = cur;
            merged_cnt += 1;
        } else {
            if (cur.end > merged[merged_cnt - 1].end) {
                merged[merged_cnt - 1].end = cur.end;
            }
            if (cur.weight > merged[merged_cnt - 1].weight) {
                merged[merged_cnt - 1].category = cur.category;
                merged[merged_cnt - 1].weight = cur.weight;
            }
        }
    }

    let out_pos: usize = 0;
    let cursor: usize = 0;
    let k: usize = 0;
    while (k < merged_cnt) : (k += 1) {
        const sp = merged[k];
        if (sp.start > cursor) {
            if (!appendBytes(out_buf, *out_pos, input[cursor..sp.start])) return null;
        }
        const label = redactionLabelFor(sp.category);
        if (!appendBytes(out_buf, *out_pos, label)) return null;
        cursor = sp.end;
    }
    if (cursor < input.len) {
        if (!appendBytes(out_buf, *out_pos, input[cursor..])) return null;
    }
    return out_pos;
}

// ============================================================================
// 3. Zero-Alloc Prompt Guardrail Scanner Engine
// ============================================================================

/// Zero-heap-allocation Prompt compliance guardrail & sensitive credential scanner.
pub struct PromptGuardrailScanner {
    policy: GuardrailPolicy = GuardrailPolicy{},
    custom_terms: [GUARDRAIL_MAX_CUSTOM_TERMS][GUARDRAIL_MAX_TERM_LEN]u8 = [[0; GUARDRAIL_MAX_TERM_LEN]; GUARDRAIL_MAX_CUSTOM_TERMS],
    custom_term_lens: [GUARDRAIL_MAX_CUSTOM_TERMS]usize = [0; GUARDRAIL_MAX_CUSTOM_TERMS],
    custom_term_count: usize = 0;

    /// Initializes a `PromptGuardrailScanner` with validated `policy`.
    static fn init(policy: GuardrailPolicy): PromptGuardrailScanner? {
        if (!policy.isValid()) return null;
        return PromptGuardrailScanner{
            policy: policy,
            custom_terms: [[0; GUARDRAIL_MAX_TERM_LEN]; GUARDRAIL_MAX_CUSTOM_TERMS],
            custom_term_lens: [0; GUARDRAIL_MAX_CUSTOM_TERMS],
            custom_term_count: 0,
        };
    }

    /// Registers a custom case-insensitive sensitive term (`1..=64` bytes).
    fn addCustomTerm(*_, term: []const u8): bool {
        if (term.len == 0 || term.len > GUARDRAIL_MAX_TERM_LEN) return false;
        if (_.custom_term_count >= GUARDRAIL_MAX_CUSTOM_TERMS) return false;
        const slot: usize = _.custom_term_count;
        let i: usize = 0;
        while (i < term.len) : (i += 1) {
            _.custom_terms[slot][i] = term[i];
        }
        _.custom_term_lens[slot] = term.len;
        _.custom_term_count += 1;
        return true;
    }

    fn recordFinding(
        out_findings: *[]GuardrailFinding,
        report: *GuardrailScanReport,
        cat: GuardrailCategory,
        start: usize,
        end: usize,
        weight: u32,
    ) {
        if (report.finding_count < out_findings.len && report.finding_count < GUARDRAIL_MAX_FINDINGS) {
            out_findings[report.finding_count] = GuardrailFinding{
                category: cat,
                start: start,
                end: end,
                weight: weight,
            };
        }
        report.finding_count += 1;
        report.risk_score += weight;
    }

    /// Scans `input` for credentials, PII, private IPv4s, control/zero-width anomalies, length limits,
    /// and custom terms, populating `out_findings` and returning a `GuardrailScanReport`.
    fn scan(input: []const u8, out_findings: *[]GuardrailFinding): GuardrailScanReport {
        let report = GuardrailScanReport{};
        if (input.len == 0) return report;

        let scan_len: usize = input.len;
        if (_.policy.max_input_bytes > 0 && input.len > _.policy.max_input_bytes) {
            scan_len = _.policy.max_input_bytes;
            report.truncated_input = true;
            PromptGuardrailScanner.recordFinding(
                out_findings,
                *report,
                GuardrailCategory.length_exceeded,
                scan_len,
                input.len,
                _.policy.weight_length_exceeded,
            );
        }
        report.scanned_bytes = scan_len;
        const s: []const u8 = input[0..scan_len];

        // Pass 1: Control characters (excluding \t, \n, \r) & UTF-8 zero-width chars (U+200B/C/D, U+FEFF)
        let ctrl_cnt: usize = 0;
        let ci: usize = 0;
        while (ci < s.len) {
            const b: u8 = s[ci];
            if ((b < 32 && b != 9 && b != 10 && b != 13) || b == 127) {
                ctrl_cnt += 1;
                ci += 1;
                continue;
            }
            if (ci + 2 < s.len) {
                // U+200B..U+200D: E2 80 8B..8D ; U+FEFF: EF BB BF
                if (b == 0xE2 && s[ci + 1] == 0x80 && (s[ci + 2] >= 0x8B && s[ci + 2] <= 0x8D)) {
                    ctrl_cnt += 1;
                    ci += 3;
                    continue;
                }
                if (b == 0xEF && s[ci + 1] == 0xBB && s[ci + 2] == 0xBF) {
                    ctrl_cnt += 1;
                    ci += 3;
                    continue;
                }
            }
            ci += 1;
        }
        report.control_anomaly_count = ctrl_cnt;
        const ratio_bps: u64 = ((ctrl_cnt as u64) * 10000) / (s.len as u64);
        report.control_ratio_bps = if (ratio_bps > 10000) (10000 as u16) else (ratio_bps as u16);
        if (ctrl_cnt > 0 && report.control_ratio_bps > _.policy.max_control_ratio_bps) {
            PromptGuardrailScanner.recordFinding(
                out_findings,
                *report,
                GuardrailCategory.control_anomaly,
                0,
                s.len,
                _.policy.weight_control_anomaly,
            );
        }

        // Pass 2: JWT 3-segment tokens (eyJ<b64>.<b64>.<b64>)
        let ji: usize = 0;
        while (ji + 10 < s.len) {
            if (hasPrefixAt(s, ji, "eyJ") && (ji == 0 || !isBase64UrlChar(s[ji - 1]))) {
                let p: usize = ji;
                while (p < s.len && isBase64UrlChar(s[p])) : (p += 1) {}
                const seg1_len: usize = p - ji;
                if (seg1_len >= 6 && p < s.len && s[p] == 46) {
                    p += 1;
                    const s2_start: usize = p;
                    while (p < s.len && isBase64UrlChar(s[p])) : (p += 1) {}
                    const seg2_len: usize = p - s2_start;
                    if (seg2_len >= 6 && p < s.len && s[p] == 46) {
                        p += 1;
                        const s3_start: usize = p;
                        while (p < s.len && isBase64UrlChar(s[p])) : (p += 1) {}
                        const seg3_len: usize = p - s3_start;
                        if (seg3_len >= 6) {
                            report.jwt_hits += 1;
                            PromptGuardrailScanner.recordFinding(
                                out_findings,
                                *report,
                                GuardrailCategory.jwt_token,
                                ji,
                                p,
                                _.policy.weight_jwt_token,
                            );
                            ji = p;
                            continue;
                        }
                    }
                }
            }
            ji += 1;
        }

        // Pass 3: API secret prefixes (`sk-`, `ghp_`, `AKIA`, `Bearer `)
        let si: usize = 0;
        while (si < s.len) {
            const at_boundary: bool = si == 0 || !isAsciiAlnum(s[si - 1]);
            if (at_boundary && (hasPrefixAt(s, si, "sk-") || hasPrefixAt(s, si, "ghp_") || hasPrefixAt(s, si, "AKIA"))) {
                let p: usize = si + 3;
                while (p < s.len && isTokenChar(s[p])) : (p += 1) {}
                if (p - si >= 16) {
                    report.secret_hits += 1;
                    PromptGuardrailScanner.recordFinding(
                        out_findings,
                        *report,
                        GuardrailCategory.api_secret,
                        si,
                        p,
                        _.policy.weight_api_secret,
                    );
                    si = p;
                    continue;
                }
            } else if (at_boundary && hasPrefixAt(s, si, "Bearer ")) {
                let p: usize = si + 7;
                while (p < s.len && isTokenChar(s[p])) : (p += 1) {}
                if (p - (si + 7) >= 12) {
                    report.secret_hits += 1;
                    PromptGuardrailScanner.recordFinding(
                        out_findings,
                        *report,
                        GuardrailCategory.api_secret,
                        si,
                        p,
                        _.policy.weight_api_secret,
                    );
                    si = p;
                    continue;
                }
            }
            si += 1;
        }

        // Pass 4: Email addresses (<local>@<domain>.<tld>)
        let ei: usize = 0;
        while (ei < s.len) {
            if (s[ei] == 64 && ei > 0 && ei + 3 < s.len) { // '@'
                let l_start: usize = ei;
                while (l_start > 0 && isEmailLocalChar(s[l_start - 1])) : (l_start -= 1) {}
                let r_end: usize = ei + 1;
                let has_dot: bool = false;
                let last_dot: usize = ei;
                while (r_end < s.len && isEmailDomainChar(s[r_end])) : (r_end += 1) {
                    if (s[r_end] == 46) {
                        has_dot = true;
                        last_dot = r_end;
                    }
                }
                const local_len: usize = ei - l_start;
                const tld_len: usize = if (r_end > last_dot + 1) (r_end - last_dot - 1) else 0;
                if (local_len >= 1 && has_dot && last_dot > ei + 1 && tld_len >= 2) {
                    report.email_hits += 1;
                    PromptGuardrailScanner.recordFinding(
                        out_findings,
                        *report,
                        GuardrailCategory.email,
                        l_start,
                        r_end,
                        _.policy.weight_email,
                    );
                    ei = r_end;
                    continue;
                }
            }
            ei += 1;
        }

        // Pass 5: Luhn-verified 13..19-digit payment card numbers (allowing single space or '-' separators)
        let pi: usize = 0;
        while (pi < s.len) {
            if (isAsciiDigit(s[pi]) && (pi == 0 || !isAsciiDigit(s[pi - 1]))) {
                let dig_buf: [24]u8 = [0; 24];
                let dig_cnt: usize = 0;
                let p: usize = pi;
                let last_digit_end: usize = pi;
                while (p < s.len && dig_cnt < 20) {
                    const ch: u8 = s[p];
                    if (isAsciiDigit(ch)) {
                        dig_buf[dig_cnt] = ch;
                        dig_cnt += 1;
                        p += 1;
                        last_digit_end = p;
                    } else if ((ch == 32 || ch == 45) && p + 1 < s.len && isAsciiDigit(s[p + 1])) {
                        p += 1;
                    } else {
                        break;
                    }
                }
                if (dig_cnt >= 13 && dig_cnt <= 19 && luhnCheckDigits(dig_buf[0..dig_cnt])) {
                    report.card_hits += 1;
                    PromptGuardrailScanner.recordFinding(
                        out_findings,
                        *report,
                        GuardrailCategory.payment_card,
                        pi,
                        last_digit_end,
                        _.policy.weight_payment_card,
                    );
                    pi = last_digit_end;
                    continue;
                }
                pi = last_digit_end;
                continue;
            }
            pi += 1;
        }

        // Pass 6: RFC 1918 & loopback private IPv4 addresses
        let ii: usize = 0;
        while (ii < s.len) {
            if (isAsciiDigit(s[ii]) && (ii == 0 || (!isAsciiDigit(s[ii - 1]) && s[ii - 1] != 46))) {
                let p: usize = ii;
                let octets: [4]u32 = [0; 4];
                let oct_idx: usize = 0;
                let valid_ip: bool = true;
                while (oct_idx < 4) : (oct_idx += 1) {
                    const o_start: usize = p;
                    let val: u32 = 0;
                    while (p < s.len && isAsciiDigit(s[p]) && (p - o_start) < 4) : (p += 1) {
                        val = val * 10 + ((s[p] - 48) as u32);
                    }
                    if (p == o_start || p - o_start > 3 || val > 255) {
                        valid_ip = false;
                        break;
                    }
                    octets[oct_idx] = val;
                    if (oct_idx < 3) {
                        if (p >= s.len || s[p] != 46) {
                            valid_ip = false;
                            break;
                        }
                        p += 1;
                    }
                }
                if (valid_ip && (p == s.len || (!isAsciiDigit(s[p]) && s[p] != 46)) &&
                    isPrivateIpv4Octets(octets[0], octets[1], octets[2], octets[3])) {
                    report.private_ip_hits += 1;
                    PromptGuardrailScanner.recordFinding(
                        out_findings,
                        *report,
                        GuardrailCategory.private_ipv4,
                        ii,
                        p,
                        _.policy.weight_private_ipv4,
                    );
                    ii = p;
                    continue;
                }
            }
            ii += 1;
        }

        // Pass 7: Custom case-insensitive terms
        let t_idx: usize = 0;
        while (t_idx < _.custom_term_count) : (t_idx += 1) {
            const t_len: usize = _.custom_term_lens[t_idx];
            const term_s: []const u8 = _.custom_terms[t_idx][0..t_len];
            let pos: usize = 0;
            while (pos + t_len <= s.len) {
                if (eqIgnoreCaseAt(s, pos, term_s)) {
                    report.custom_term_hits += 1;
                    PromptGuardrailScanner.recordFinding(
                        out_findings,
                        *report,
                        GuardrailCategory.custom_term,
                        pos,
                        pos + t_len,
                        _.policy.weight_custom_term,
                    );
                    pos += t_len;
                } else {
                    pos += 1;
                }
            }
        }

        // Determine final action
        if (report.risk_score >= _.policy.block_score_threshold) {
            report.action = GuardrailAction.block;
        } else if (report.risk_score >= _.policy.redact_score_threshold) {
            report.action = GuardrailAction.redact;
        } else {
            report.action = GuardrailAction.allow;
        }
        return report;
    }
}
