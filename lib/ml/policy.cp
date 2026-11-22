// Source: cp/lib/guardrail/policy.cp (Guardrail Policy & Inspection Pipeline)
// Configurable multi-stage input/output guardrail policy with budget limits,
// boundary-escape signature checks, blocked-phrase filtering, and PII redaction.

const std = @import("std");
import {
    GuardrailError,
    PiiKind,
    PiiFinding,
    RedactionResult,
    PiiScanner,
} from "./pii.cp";

/// Final enforcement action produced by `GuardrailPolicy`.
pub enum GuardrailAction: u8 {
    Allow = 0,
    Redact = 1,
    Block = 2,
}

/// Structured verdict returned by `GuardrailPolicy.inspectInput` and `inspectOutput`.
pub struct GuardrailVerdict {
    action: GuardrailAction = .Allow;
    blocked: bool = false;
    sanitized_text: string = "";
    reasons: []string = [];
    finding_count: usize = 0;

    /// Releases all heap allocations owned by this verdict.
    fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.sanitized_text.len > 0) {
            alloc.free(_.sanitized_text);
            _.sanitized_text = "";
        }
        for (r of _.reasons) {
            if (r.len > 0) alloc.free(r);
        }
        if (_.reasons.len > 0) {
            alloc.free(_.reasons);
            _.reasons = [];
        }
        _.finding_count = 0;
    }
}

fn dupSlice(alloc: std.mem.Allocator, s: string): string ! GuardrailError {
    if (s.len == 0) return "";
    return alloc.dupe(u8, s) catch return GuardrailError.OutOfMemory;
}

fn freeStringList(alloc: std.mem.Allocator, list: *std.ArrayList(string)) {
    for (item of list.items) {
        if (item.len > 0) alloc.free(item);
    }
    list.deinit(alloc);
}

fn containsIgnoreCase(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    const limit = haystack.len - needle.len;
    while (i <= limit) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(haystack[i .. i + needle.len], needle)) {
            return true;
        }
    }
    return false;
}

fn appendReason(reasons: *std.ArrayList(string), alloc: std.mem.Allocator, msg: string): void ! GuardrailError {
    const owned = dupSlice(alloc, msg)?;
    reasons.append(alloc, owned) catch {
        if (owned.len > 0) alloc.free(owned);
        return GuardrailError.OutOfMemory;
    };
}

fn freeReasonsList(alloc: std.mem.Allocator, reasons: *std.ArrayList(string)) {
    for (r of reasons.items) {
        if (r.len > 0) alloc.free(r);
    }
    reasons.deinit(alloc);
}

/// Configurable multi-stage AI Agent safety guardrail policy.
pub class GuardrailPolicy {
    alloc: std.mem.Allocator;
    max_chars: usize = 0;
    redact_pii: bool = true;
    mask_token: string = "";
    blocked_phrases: std.ArrayList(string) = .empty;
    injection_signatures: std.ArrayList(string) = .empty;

    /// Initializes a policy with PII redaction enabled and no character limit.
    pub static fn init(alloc: std.mem.Allocator): GuardrailPolicy {
        return new GuardrailPolicy(alloc, 0, true, "", .empty, .empty);
    }

    /// Releases all owned rule strings and internal lists.
    pub fn deinit(*_) {
        if (_.mask_token.len > 0) {
            _.alloc.free(_.mask_token);
            _.mask_token = "";
        }
        freeStringList(_.alloc, *_.blocked_phrases);
        freeStringList(_.alloc, *_.injection_signatures);
    }

    /// Sets the maximum allowed character budget (`0` disables the limit).
    pub fn setMaxChars(*_, n: usize) {
        _.max_chars = n;
    }

    /// Enables or disables automatic PII/credential redaction.
    pub fn setRedactPii(*_, enabled: bool) {
        _.redact_pii = enabled;
    }

    /// Sets a custom fixed mask token; passing empty resets to typed placeholders (`[REDACTED:KIND]`).
    pub fn setMaskToken(*_, token: string): void ! GuardrailError {
        const next_tok = dupSlice(_.alloc, token)?;
        if (_.mask_token.len > 0) {
            _.alloc.free(_.mask_token);
        }
        _.mask_token = next_tok;
    }

    /// Registers a case-insensitive blocked phrase rule.
    pub fn addBlockedPhrase(*_, phrase: string): void ! GuardrailError {
        if (phrase.len == 0) return GuardrailError.InvalidRule;
        const owned = dupSlice(_.alloc, phrase)?;
        _.blocked_phrases.append(_.alloc, owned) catch {
            _.alloc.free(owned);
            return GuardrailError.OutOfMemory;
        };
    }

    /// Registers a case-insensitive prompt boundary-escape or override signature.
    pub fn addInjectionSignature(*_, sig: string): void ! GuardrailError {
        if (sig.len == 0) return GuardrailError.InvalidRule;
        const owned = dupSlice(_.alloc, sig)?;
        _.injection_signatures.append(_.alloc, owned) catch {
            _.alloc.free(owned);
            return GuardrailError.OutOfMemory;
        };
    }

    /// Preloads standard boundary-escape and role-override detection signatures.
    pub fn addDefaultInjectionSignatures(*_): void ! GuardrailError {
        _.addInjectionSignature("ignore previous instructions")?;
        _.addInjectionSignature("</system>")?;
        _.addInjectionSignature("system prompt:")?;
        _.addInjectionSignature("disregard prior instructions")?;
        _.addInjectionSignature("<|im_start|>system")?;
    }

    /// Evaluates user/agent input through: budget -> boundary-escape -> blocked phrases -> PII redaction.
    pub fn inspectInput(alloc: std.mem.Allocator, text: string): GuardrailVerdict ! GuardrailError {
        return _.evaluatePipeline(alloc, text, true);
    }

    /// Evaluates model output through: budget -> blocked phrases -> secret/PII redaction.
    pub fn inspectOutput(alloc: std.mem.Allocator, text: string): GuardrailVerdict ! GuardrailError {
        return _.evaluatePipeline(alloc, text, false);
    }

    fn evaluatePipeline(alloc: std.mem.Allocator, text: string, check_injection: bool): GuardrailVerdict ! GuardrailError {
        let reasons: std.ArrayList(string) = .empty;
        errdefer freeReasonsList(alloc, *reasons);

        // Stage 1: Length budget check
        if (_.max_chars > 0 && text.len > _.max_chars) {
            appendReason(*reasons, alloc, "length_limit_exceeded")?;
            const owned_reasons = reasons.toOwnedSlice(alloc) catch return GuardrailError.OutOfMemory;
            return GuardrailVerdict{
                action: .Block,
                blocked: true,
                sanitized_text: "",
                reasons: owned_reasons,
                finding_count: 0,
            };
        }

        // Stage 2: Boundary-escape / override signature check (input pipeline)
        if (check_injection) {
            for (sig of _.injection_signatures.items) {
                if (containsIgnoreCase(text, sig)) {
                    appendReason(*reasons, alloc, "boundary_escape_signature")?;
                    const owned_reasons = reasons.toOwnedSlice(alloc) catch return GuardrailError.OutOfMemory;
                    return GuardrailVerdict{
                        action: .Block,
                        blocked: true,
                        sanitized_text: "",
                        reasons: owned_reasons,
                        finding_count: 0,
                    };
                }
            }
        }

        // Stage 3: Blocked phrase check (case-insensitive)
        for (phrase of _.blocked_phrases.items) {
            if (containsIgnoreCase(text, phrase)) {
                appendReason(*reasons, alloc, "blocked_phrase")?;
                const owned_reasons = reasons.toOwnedSlice(alloc) catch return GuardrailError.OutOfMemory;
                return GuardrailVerdict{
                    action: .Block,
                    blocked: true,
                    sanitized_text: "",
                    reasons: owned_reasons,
                    finding_count: 0,
                };
            }
        }

        // Stage 4: PII & secret scan / redaction
        if (_.redact_pii) {
            let red = if (_.mask_token.len > 0)
                PiiScanner.redact(alloc, text, _.mask_token)?
            else
                PiiScanner.redactTyped(alloc, text)?;

            if (red.finding_count > 0) {
                const count = red.finding_count;
                const sanitized = red.text;
                red.text = "";
                red.deinit(alloc);

                appendReason(*reasons, alloc, "pii_redacted") catch e => {
                    if (sanitized.len > 0) alloc.free(sanitized);
                    return e;
                };
                const owned_reasons = reasons.toOwnedSlice(alloc) catch {
                    if (sanitized.len > 0) alloc.free(sanitized);
                    return GuardrailError.OutOfMemory;
                };
                return GuardrailVerdict{
                    action: .Redact,
                    blocked: false,
                    sanitized_text: sanitized,
                    reasons: owned_reasons,
                    finding_count: count,
                };
            }
            red.deinit(alloc);
        }

        // Clean pass
        const clean_copy = dupSlice(alloc, text)?;
        const owned_reasons = reasons.toOwnedSlice(alloc) catch {
            if (clean_copy.len > 0) alloc.free(clean_copy);
            return GuardrailError.OutOfMemory;
        };
        return GuardrailVerdict{
            action: .Allow,
            blocked: false,
            sanitized_text: clean_copy,
            reasons: owned_reasons,
            finding_count: 0,
        };
    }
}
