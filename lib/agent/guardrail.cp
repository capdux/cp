// Origin: Pure-CP Stdlib (AI Agent Input/Output Guardrails, Secret Scanner & Policy Engine) | Created: 2026-11-22

fn satAddU32(a: u32, b: u32): u32 {
    if (a > 0xFFFFFFFF - b) {
        return 0xFFFFFFFF;
    }
    return a + b;
}

fn toLowerAscii(b: u8): u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn isKeyChar(b: u8): bool {
    if (b >= 48 && b <= 57) return true; // 0-9
    if (b >= 65 && b <= 90) return true; // A-Z
    if (b >= 97 && b <= 122) return true; // a-z
    return b == 45 || b == 95; // '-' or '_'
}

fn isUpperAlnum(b: u8): bool {
    if (b >= 48 && b <= 57) return true;
    if (b >= 65 && b <= 90) return true;
    return false;
}

fn matchAt(text: string, pos: usize, pattern: string, case_insensitive: bool): bool {
    if (pos + pattern.len > text.len) {
        return false;
    }
    let i: usize = 0;
    while (i < pattern.len) {
        const a = if (case_insensitive) toLowerAscii(text[pos + i]) else text[pos + i];
        const b = if (case_insensitive) toLowerAscii(pattern[i]) else pattern[i];
        if (a != b) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn findPattern(text: string, pattern: string, case_insensitive: bool): usize? {
    if (pattern.len == 0 || pattern.len > text.len) {
        return null;
    }
    const limit = text.len - pattern.len;
    let i: usize = 0;
    while (i <= limit) {
        if (matchAt(text, i, pattern, case_insensitive)) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn countKeyCharsFrom(text: string, start: usize): usize {
    let i = start;
    while (i < text.len && isKeyChar(text[i])) {
        i += 1;
    }
    return i - start;
}

fn countUpperAlnumFrom(text: string, start: usize): usize {
    let i = start;
    while (i < text.len && isUpperAlnum(text[i])) {
        i += 1;
    }
    return i - start;
}

fn findControlCharAnomaly(text: string): usize? {
    let i: usize = 0;
    while (i < text.len) {
        const b = text[i];
        if (b == 0 || b == 127 || (b < 32 && b != 9 && b != 10 && b != 13)) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn findSecretLeak(text: string): usize? {
    let i: usize = 0;
    while (i < text.len) {
        const b = text[i];
        // PEM private key headers
        if (b == 45) { // '-'
            if (
                matchAt(text, i, "-----BEGIN PRIVATE KEY-----", false) ||
                matchAt(text, i, "-----BEGIN RSA PRIVATE KEY-----", false) ||
                matchAt(text, i, "-----BEGIN EC PRIVATE KEY-----", false) ||
                matchAt(text, i, "-----BEGIN OPENSSH PRIVATE KEY-----", false)
            ) {
                return i;
            }
        }
        // OpenAI / Anthropic style sk-... keys
        if (b == 115 && matchAt(text, i, "sk-", false)) {
            if (countKeyCharsFrom(text, i + 3) >= 16) {
                return i;
            }
        }
        // GitHub personal access tokens
        if (b == 103) {
            if (matchAt(text, i, "ghp_", false) || matchAt(text, i, "gho_", false)) {
                if (countKeyCharsFrom(text, i + 4) >= 12) {
                    return i;
                }
            } else if (matchAt(text, i, "github_pat_", false)) {
                if (countKeyCharsFrom(text, i + 11) >= 12) {
                    return i;
                }
            }
        }
        // AWS Access Key IDs (AKIA + 16 uppercase alnum)
        if (b == 65 && matchAt(text, i, "AKIA", false)) {
            if (countUpperAlnumFrom(text, i + 4) >= 16) {
                return i;
            }
        }
        i += 1;
    }
    return null;
}

/// Enforcement decision returned by `evaluateGuardrails`.
pub enum GuardAction {
    Allow,
    Warn,
    Block,
}

/// Primary violation category reported in `GuardVerdict`.
pub enum GuardReason {
    None,
    RuleBlocked,
    SecretLeak,
    MaxLengthExceeded,
    NestingTooDeep,
    ControlCharAnomaly,
}

/// Custom pattern rule evaluated by `evaluateGuardrails`.
pub struct GuardRule {
    id: u32,
    pattern: string,
    case_insensitive: bool,
    action: GuardAction,
    weight: u16,
}

/// Result of evaluating input/output text against built-in and custom guardrails.
pub struct GuardVerdict {
    action: GuardAction,
    reason: GuardReason,
    matched_rule_id: u32,
    risk_score: u32,
    match_offset: usize,
}

/// Configuration for `evaluateGuardrails` (`0` disables `max_bytes` / `max_json_depth` / risk thresholds).
pub struct GuardConfig {
    max_bytes: usize,
    max_json_depth: u16,
    block_secret_leaks: bool,
    warn_risk_threshold: u32,
    block_risk_threshold: u32,

    /// Returns production-safe default guardrail limits.
    fn default(): GuardConfig {
        return GuardConfig {
            max_bytes: 65536,
            max_json_depth: 32,
            block_secret_leaks: true,
            warn_risk_threshold: 30,
            block_risk_threshold: 80,
        };
    }
}

/// Structural depth and balance report returned by `inspectJsonPayloadDepth`.
pub struct JsonDepthReport {
    valid_balance: bool,
    max_depth: u16,
    string_count: u32,
}

/// Zero-allocation structural pre-check for JSON tool payloads.
/// Skips string literals (respecting `\"` and `\\` escapes), tracks maximum `{}`/`[]` nesting depth,
/// and short-circuits immediately when `max_allowed_depth > 0` and depth exceeds `max_allowed_depth`.
pub fn inspectJsonPayloadDepth(payload: string, max_allowed_depth: u16): JsonDepthReport {
    let in_string: bool = false;
    let escaped: bool = false;
    let cur_depth: u16 = 0;
    let max_depth: u16 = 0;
    let string_count: u32 = 0;
    let valid_balance: bool = true;

    let i: usize = 0;
    while (i < payload.len) {
        const b = payload[i];
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (b == 92) { // '\\'
                escaped = true;
            } else if (b == 34) { // '"'
                in_string = false;
            }
        } else {
            if (b == 34) { // '"'
                in_string = true;
                string_count = satAddU32(string_count, 1);
            } else if (b == 123 || b == 91) { // '{' or '['
                if (cur_depth < 0xFFFF) {
                    cur_depth += 1;
                }
                if (cur_depth > max_depth) {
                    max_depth = cur_depth;
                }
                if (max_allowed_depth > 0 && cur_depth > max_allowed_depth) {
                    return JsonDepthReport {
                        valid_balance: false,
                        max_depth: max_depth,
                        string_count: string_count,
                    };
                }
            } else if (b == 125 || b == 93) { // '}' or ']'
                if (cur_depth == 0) {
                    valid_balance = false;
                } else {
                    cur_depth -= 1;
                }
            }
        }
        i += 1;
    }

    if (in_string || cur_depth != 0) {
        valid_balance = false;
    }

    return JsonDepthReport {
        valid_balance: valid_balance,
        max_depth: max_depth,
        string_count: string_count,
    };
}

fn firstNonWsByte(text: string): u8 {
    let i: usize = 0;
    while (i < text.len) {
        const b = text[i];
        if (b != 32 && b != 9 && b != 10 && b != 13) {
            return b;
        }
        i += 1;
    }
    return 0;
}

/// Evaluates `text` against length limits, control-character anomalies, JSON nesting limits,
/// built-in secret leak detectors, and caller-provided custom `rules` with cumulative risk scoring.
pub fn evaluateGuardrails(
    text: string,
    rules: []const GuardRule,
    cfg: GuardConfig,
): GuardVerdict {
    // 1. Max length check
    if (cfg.max_bytes > 0 && text.len > cfg.max_bytes) {
        return GuardVerdict {
            action: GuardAction.Block,
            reason: GuardReason.MaxLengthExceeded,
            matched_rule_id: 0,
            risk_score: 100,
            match_offset: cfg.max_bytes,
        };
    }

    // 2. Control character / NUL anomaly check
    if (ctrl_off := findControlCharAnomaly(text)) {
        return GuardVerdict {
            action: GuardAction.Block,
            reason: GuardReason.ControlCharAnomaly,
            matched_rule_id: 0,
            risk_score: 100,
            match_offset: ctrl_off,
        };
    }

    // 3. JSON payload nesting depth check when payload looks like a JSON container
    if (cfg.max_json_depth > 0) {
        const first = firstNonWsByte(text);
        if (first == 123 || first == 91) {
            const rep = inspectJsonPayloadDepth(text, cfg.max_json_depth);
            if (rep.max_depth > cfg.max_json_depth) {
                return GuardVerdict {
                    action: GuardAction.Block,
                    reason: GuardReason.NestingTooDeep,
                    matched_rule_id: 0,
                    risk_score: 100,
                    match_offset: 0,
                };
            }
        }
    }

    // 4. Built-in secret leak scanner
    if (cfg.block_secret_leaks) {
        if (sec_off := findSecretLeak(text)) {
            return GuardVerdict {
                action: GuardAction.Block,
                reason: GuardReason.SecretLeak,
                matched_rule_id: 0,
                risk_score: 100,
                match_offset: sec_off,
            };
        }
    }

    // 5. Custom rule table evaluation + cumulative risk score
    let total_risk: u32 = 0;
    let has_block: bool = false;
    let has_warn: bool = false;
    let matched_id: u32 = 0;
    let matched_off: usize = 0;
    let has_any_match: bool = false;

    let i: usize = 0;
    while (i < rules.len) {
        const rule = rules[i];
        if (off := findPattern(text, rule.pattern, rule.case_insensitive)) {
            total_risk = satAddU32(total_risk, rule.weight as u32);
            if (!has_any_match) {
                has_any_match = true;
                matched_id = rule.id;
                matched_off = off;
            }
            if (rule.action == GuardAction.Block) {
                if (!has_block) {
                    matched_id = rule.id;
                    matched_off = off;
                }
                has_block = true;
            } else if (rule.action == GuardAction.Warn) {
                has_warn = true;
            }
        }
        i += 1;
    }

    if (has_block || (cfg.block_risk_threshold > 0 && total_risk >= cfg.block_risk_threshold)) {
        return GuardVerdict {
            action: GuardAction.Block,
            reason: GuardReason.RuleBlocked,
            matched_rule_id: matched_id,
            risk_score: total_risk,
            match_offset: matched_off,
        };
    }

    if (has_warn || (cfg.warn_risk_threshold > 0 && total_risk >= cfg.warn_risk_threshold)) {
        return GuardVerdict {
            action: GuardAction.Warn,
            reason: GuardReason.RuleBlocked,
            matched_rule_id: matched_id,
            risk_score: total_risk,
            match_offset: matched_off,
        };
    }

    return GuardVerdict {
        action: GuardAction.Allow,
        reason: GuardReason.None,
        matched_rule_id: 0,
        risk_score: total_risk,
        match_offset: 0,
    };
}
