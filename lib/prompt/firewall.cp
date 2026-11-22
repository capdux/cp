// Origin: Pure-CP Stdlib (Zero-Alloc Prompt Boundary Firewall & Credential DLP Redactor) | Created: 2026-11-22
// Zero-heap-allocation multi-layer input/output boundary scanner and credential redactor for
// production AI Agents: detects role delimiter boundary overrides, credential leaks,
// path traversal sequences, and disallowed URL schemes with deterministic risk scoring.

/// Risk flag: control/role delimiter override or instruction boundary violation.
pub const FIREWALL_FLAG_DELIMITER_OVERRIDE: u32 = 1 << 0;
/// Risk flag: credential / API token / PEM key header detected.
pub const FIREWALL_FLAG_SECRET_LEAK: u32 = 1 << 1;
/// Risk flag: relative parent-directory path traversal sequence detected.
pub const FIREWALL_FLAG_PATH_TRAVERSAL: u32 = 1 << 2;
/// Risk flag: disallowed URL scheme (`file://`, `javascript:`, `data:text/html`) detected.
pub const FIREWALL_FLAG_UNSAFE_URL: u32 = 1 << 3;

const REDACTED_SECRET: string = "[REDACTED_SECRET]";

/// Final enforcement action produced by `evaluatePromptFirewall`.
pub enum FirewallAction {
    Allow,
    Redact,
    Block,
}

/// Configurable policy for `evaluatePromptFirewall`.
pub struct FirewallPolicy {
    block_threshold: u32 = 70;
    redact_secrets: bool = true;
    check_delimiters: bool = true;
    check_secrets: bool = true;
    check_path_traversal: bool = true;
    check_unsafe_urls: bool = true;

    /// Balanced production default: blocks delimiter overrides, traversal, and unsafe URLs (>= 70),
    /// while flagging single credential leaks (50) for automatic redaction.
    static fn default(): FirewallPolicy {
        return FirewallPolicy{};
    }

    /// Strict zero-tolerance policy: blocks immediately on any finding including credential leaks.
    static fn strict(): FirewallPolicy {
        return FirewallPolicy{
            block_threshold: 40,
            redact_secrets: false,
            check_delimiters: true,
            check_secrets: true,
            check_path_traversal: true,
            check_unsafe_urls: true,
        };
    }
}

/// Single boundary or credential finding located in the scanned text.
pub struct FirewallFinding {
    flag: u32 = 0;
    start: usize = 0;
    end: usize = 0;
    severity: u32 = 0;
    rule_name: string = "";
}

/// Aggregate verdict returned by `evaluatePromptFirewall`.
pub struct FirewallVerdict {
    action: FirewallAction = FirewallAction.Allow;
    flags: u32 = 0;
    risk_score: u32 = 0;
    finding_count: usize = 0;
    findings_stored: usize = 0;
    findings_truncated: bool = false;
}

// ============================================================================
// Internal Zero-Alloc ASCII & Pattern Scanning Helpers
// ============================================================================

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn isTokenChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '_' || c == '-';
}

fn isUpperAlnum(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

fn matchExactAt(text: string, pos: usize, pat: string): bool {
    if (pos + pat.len > text.len) return false;
    let i: usize = 0;
    while (i < pat.len) : (i += 1) {
        if (text[pos + i] != pat[i]) return false;
    }
    return true;
}

fn matchLowerAt(text: string, pos: usize, lower_pat: string): bool {
    if (pos + lower_pat.len > text.len) return false;
    let i: usize = 0;
    while (i < lower_pat.len) : (i += 1) {
        if (asciiLower(text[pos + i]) != lower_pat[i]) return false;
    }
    return true;
}

fn addRiskScore(cur: u32, add: u32): u32 {
    if (cur >= 100 || add >= 100 - cur) return 100;
    return cur + add;
}

fn recordFinding(verdict: *FirewallVerdict, out_findings: []FirewallFinding, f: FirewallFinding) {
    verdict.*.flags = verdict.*.flags | f.flag;
    verdict.*.risk_score = addRiskScore(verdict.*.risk_score, f.severity);
    verdict.*.finding_count += 1;
    if (verdict.*.findings_stored < out_findings.len) {
        out_findings[verdict.*.findings_stored] = f;
        verdict.*.findings_stored += 1;
    } else {
        verdict.*.findings_truncated = true;
    }
}

/// Checks if a credential span starts at `text[pos]`. Returns the half-open end offset or `null`.
fn matchSecretSpanAt(text: string, pos: usize): usize? {
    // 1. OpenAI-style API key: "sk-" + >= 8 token chars
    if (matchExactAt(text, pos, "sk-")) {
        let end = pos + 3;
        while (end < text.len && isTokenChar(text[end])) : (end += 1) {}
        if (end - (pos + 3) >= 8) return end;
    }

    // 2. GitHub personal access token: "ghp_" + >= 8 token chars
    if (matchExactAt(text, pos, "ghp_")) {
        let end = pos + 4;
        while (end < text.len && isTokenChar(text[end])) : (end += 1) {}
        if (end - (pos + 4) >= 8) return end;
    }

    // 3. AWS Access Key ID: "AKIA" + >= 12 uppercase alphanumeric chars
    if (matchExactAt(text, pos, "AKIA")) {
        let end = pos + 4;
        while (end < text.len && isUpperAlnum(text[end])) : (end += 1) {}
        if (end - (pos + 4) >= 12) return end;
    }

    // 4. PEM private/key header: "-----BEGIN " up to closing "-----" or end of line
    if (matchExactAt(text, pos, "-----BEGIN ")) {
        let end = pos + 11;
        while (end < text.len && text[end] != '\n' && text[end] != '\r') : (end += 1) {
            if (matchExactAt(text, end, "-----")) {
                return end + 5;
            }
        }
        return end;
    }

    return null;
}

fn matchDelimiterAt(text: string, pos: usize): usize? {
    if (matchLowerAt(text, pos, "<|im_start|>")) return pos + 12;
    if (matchLowerAt(text, pos, "<|im_end|>")) return pos + 10;
    if (matchLowerAt(text, pos, "<|system|>")) return pos + 10;
    if (matchLowerAt(text, pos, "[inst]")) return pos + 6;
    if (matchLowerAt(text, pos, "[/inst]")) return pos + 7;
    if (matchLowerAt(text, pos, "### system:")) return pos + 11;
    if (matchLowerAt(text, pos, "ignore previous instructions")) return pos + 28;
    if (matchLowerAt(text, pos, "disregard all prior instructions")) return pos + 32;
    return null;
}

fn matchTraversalAt(text: string, pos: usize): usize? {
    if (matchExactAt(text, pos, "../")) return pos + 3;
    if (matchExactAt(text, pos, "..\\")) return pos + 3;
    if (matchLowerAt(text, pos, "%2e%2e/")) return pos + 7;
    if (matchLowerAt(text, pos, "%2e%2e%2f")) return pos + 9;
    return null;
}

fn matchUnsafeUrlAt(text: string, pos: usize): usize? {
    if (matchLowerAt(text, pos, "file://")) return pos + 7;
    if (matchLowerAt(text, pos, "javascript:")) return pos + 11;
    if (matchLowerAt(text, pos, "data:text/html")) return pos + 14;
    return null;
}

// ============================================================================
// Public Firewall Evaluation & Zero-Alloc Redaction APIs
// ============================================================================

/// Evaluates `text` against `policy` in a single zero-allocation pass, recording findings into
/// `out_findings` and returning a deterministic `FirewallVerdict` (`Allow | Redact | Block`).
pub fn evaluatePromptFirewall(
    text: string,
    policy: FirewallPolicy,
    out_findings: []FirewallFinding,
): FirewallVerdict {
    let verdict = FirewallVerdict{};
    if (text.len == 0) {
        return verdict;
    }

    let i: usize = 0;
    while (i < text.len) {
        if (policy.check_delimiters) {
            if (end := matchDelimiterAt(text, i)) {
                recordFinding(*verdict, out_findings, FirewallFinding{
                    flag: FIREWALL_FLAG_DELIMITER_OVERRIDE,
                    start: i,
                    end: end,
                    severity: 75,
                    rule_name: "delimiter_override",
                });
                i = end;
                continue;
            }
        }

        if (policy.check_secrets) {
            if (end := matchSecretSpanAt(text, i)) {
                recordFinding(*verdict, out_findings, FirewallFinding{
                    flag: FIREWALL_FLAG_SECRET_LEAK,
                    start: i,
                    end: end,
                    severity: 50,
                    rule_name: "secret_leak",
                });
                i = end;
                continue;
            }
        }

        if (policy.check_path_traversal) {
            if (end := matchTraversalAt(text, i)) {
                recordFinding(*verdict, out_findings, FirewallFinding{
                    flag: FIREWALL_FLAG_PATH_TRAVERSAL,
                    start: i,
                    end: end,
                    severity: 70,
                    rule_name: "path_traversal",
                });
                i = end;
                continue;
            }
        }

        if (policy.check_unsafe_urls) {
            if (end := matchUnsafeUrlAt(text, i)) {
                recordFinding(*verdict, out_findings, FirewallFinding{
                    flag: FIREWALL_FLAG_UNSAFE_URL,
                    start: i,
                    end: end,
                    severity: 75,
                    rule_name: "unsafe_url_scheme",
                });
                i = end;
                continue;
            }
        }

        i += 1;
    }

    if (verdict.finding_count == 0) {
        verdict.action = FirewallAction.Allow;
    } else if (verdict.risk_score >= policy.block_threshold) {
        verdict.action = FirewallAction.Block;
    } else if ((verdict.flags & FIREWALL_FLAG_SECRET_LEAK) != 0 && policy.redact_secrets) {
        verdict.action = FirewallAction.Redact;
    } else {
        verdict.action = FirewallAction.Block;
    }

    return verdict;
}

/// Replaces all detected credential spans (`sk-...`, `ghp_...`, `AKIA...`, `-----BEGIN ...-----`)
/// in `text` with `[REDACTED_SECRET]` into caller-provided `out_buf`.
/// Returns the number of bytes written, or `null` if `out_buf` is too small (never writes OOB).
pub fn redactFirewallSecrets(text: string, out_buf: []u8): usize? {
    let out_pos: usize = 0;
    let i: usize = 0;

    while (i < text.len) {
        if (end := matchSecretSpanAt(text, i)) {
            if (out_pos + REDACTED_SECRET.len > out_buf.len) {
                return null;
            }
            let k: usize = 0;
            while (k < REDACTED_SECRET.len) : (k += 1) {
                out_buf[out_pos + k] = REDACTED_SECRET[k];
            }
            out_pos += REDACTED_SECRET.len;
            i = end;
            continue;
        }

        if (out_pos >= out_buf.len) {
            return null;
        }
        out_buf[out_pos] = text[i];
        out_pos += 1;
        i += 1;
    }

    return out_pos;
}
