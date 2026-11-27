// @cap/zod-validation-error — Zero-heap human-readable validation error formatter
// Clean-room CP implementation inspired by npm:zod-validation-error (causaly/zod-validation-error)

import std from "std";

pub const VALIDATION_ERROR_NAME: string = "ZodValidationError";

pub struct ValidationIssue {
    path: string,
    message: string,
    code: string,
}

pub struct FormatErrorOptions {
    prefix: string,
    prefix_sep: string,
    issue_sep: string,
    max_issues: usize,
    include_path: bool,
}

/// Full npm `zod-validation-error` message builder options.
pub struct MessageBuilderOptions {
    pub prefix: string,
    pub prefix_separator: string,
    pub issue_separator: string,
    pub union_separator: string,
    pub max_issues_in_message: usize,
    pub include_path: bool,
    pub include_code: bool,
}

/// Structured validation error modeled on `zod-validation-error`'s `ValidationError`.
pub struct ValidationError {
    pub name: string,
    pub message_text: string,
    pub details_count: usize,
    pub total_issues: usize,
    pub truncated: bool,

    pub fn message(): string {
        return _.message_text;
    }

    pub fn toString(): string {
        return _.message_text;
    }

    pub fn copyMessage(out: []u8): usize {
        return zverWriteStr(out, 0, _.message_text);
    }

    pub fn issueCount(): usize {
        return _.total_issues;
    }

    pub fn detailsCount(): usize {
        return _.details_count;
    }

    pub fn isTruncated(): bool {
        return _.truncated;
    }

    pub fn isValidationError(): bool {
        if (_.name.len != VALIDATION_ERROR_NAME.len) { return false; }
        let i: usize = 0;
        while (i < _.name.len) : (i += 1) {
            if (_.name[i] != VALIDATION_ERROR_NAME[i]) { return false; }
        }
        return true;
    }
}

pub fn defaultFormatErrorOptions(): FormatErrorOptions {
    return FormatErrorOptions{
        prefix: "Validation error",
        prefix_sep: ": ",
        issue_sep: "; ",
        max_issues: 99,
        include_path: true,
    };
}

pub fn defaultMessageBuilderOptions(): MessageBuilderOptions {
    return MessageBuilderOptions{
        prefix: "Validation error",
        prefix_separator: ": ",
        issue_separator: "; ",
        union_separator: ", or ",
        max_issues_in_message: 99,
        include_path: true,
        include_code: false,
    };
}

fn zverWriteStr(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn zverWriteByte(out: []u8, pos: usize, ch: u8): usize {
    if (pos < out.len) {
        out[pos] = ch;
        return pos + 1;
    }
    return pos;
}

fn zverIsIdentChar(ch: u8): bool {
    return (ch >= 'a' && ch <= 'z')
        || (ch >= 'A' && ch <= 'Z')
        || (ch >= '0' && ch <= '9')
        || ch == '_'
        || ch == '$';
}

fn zverIsDigitsOnly(s: string): bool {
    if (s.len == 0) { return false; }
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] < '0' || s[i] > '9') { return false; }
    }
    return true;
}

fn zverIsSimpleIdentifier(s: string): bool {
    if (s.len == 0) { return false; }
    if (s[0] >= '0' && s[0] <= '9') { return false; }
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (!zverIsIdentChar(s[i])) { return false; }
    }
    return true;
}

/// Joins path segments following `zod-validation-error` conventions:
/// - Simple identifiers use dot notation (`user.email`)
/// - Numeric array indices use bracket notation (`items[0].name`)
/// - Segments with spaces/dots/quotes use quoted brackets (`data["a.b"]`)
pub fn formatIssuePath(segments: []const string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < segments.len) : (i += 1) {
        const seg = segments[i];
        if (seg.len == 0) { continue; }
        if (zverIsDigitsOnly(seg)) {
            pos = zverWriteByte(out, pos, '[');
            pos = zverWriteStr(out, pos, seg);
            pos = zverWriteByte(out, pos, ']');
        } else if (zverIsSimpleIdentifier(seg)) {
            if (pos > 0) {
                pos = zverWriteByte(out, pos, '.');
            }
            pos = zverWriteStr(out, pos, seg);
        } else {
            pos = zverWriteStr(out, pos, "[\"");
            pos = zverWriteStr(out, pos, seg);
            pos = zverWriteStr(out, pos, "\"]");
        }
    }
    return pos;
}

/// Formats a single `ValidationIssue` into `out`.
pub fn formatValidationIssue(
    issue: ValidationIssue,
    include_path: bool,
    include_code: bool,
    out: []u8,
): usize {
    let pos: usize = 0;
    pos = zverWriteStr(out, pos, issue.message);
    if (include_code && issue.code.len > 0) {
        pos = zverWriteStr(out, pos, " [");
        pos = zverWriteStr(out, pos, issue.code);
        pos = zverWriteByte(out, pos, ']');
    }
    if (include_path && issue.path.len > 0) {
        pos = zverWriteStr(out, pos, " at \"");
        pos = zverWriteStr(out, pos, issue.path);
        pos = zverWriteByte(out, pos, '"');
    }
    return pos;
}

/// Format a list of validation issues into a human-readable string in `out`.
/// Returns the number of bytes written.
pub fn formatValidationIssues(
    issues: []const ValidationIssue,
    opts: FormatErrorOptions,
    out: []u8,
): usize {
    if (out.len == 0) {
        return 0;
    }

    let pos: usize = 0;

    if (issues.len == 0) {
        if (opts.prefix.len > 0) {
            pos = zverWriteStr(out, pos, opts.prefix);
        }
        return pos;
    }

    if (opts.prefix.len > 0) {
        pos = zverWriteStr(out, pos, opts.prefix);
        pos = zverWriteStr(out, pos, opts.prefix_sep);
    }

    let limit = issues.len;
    if (opts.max_issues > 0 && opts.max_issues < limit) {
        limit = opts.max_issues;
    }

    let i: usize = 0;
    while (i < limit) : (i += 1) {
        if (i > 0) {
            pos = zverWriteStr(out, pos, opts.issue_sep);
        }
        const issue = issues[i];
        pos = zverWriteStr(out, pos, issue.message);
        if (opts.include_path && issue.path.len > 0) {
            pos = zverWriteStr(out, pos, " at \"");
            pos = zverWriteStr(out, pos, issue.path);
            pos = zverWriteByte(out, pos, '"');
        }
    }

    return pos;
}

/// Full `createMessage` builder using `MessageBuilderOptions`.
pub fn createMessage(
    issues: []const ValidationIssue,
    opts: MessageBuilderOptions,
    out: []u8,
): usize {
    if (out.len == 0) { return 0; }
    let pos: usize = 0;

    if (issues.len == 0) {
        if (opts.prefix.len > 0) {
            pos = zverWriteStr(out, pos, opts.prefix);
        }
        return pos;
    }

    if (opts.prefix.len > 0) {
        pos = zverWriteStr(out, pos, opts.prefix);
        pos = zverWriteStr(out, pos, opts.prefix_separator);
    }

    let limit = issues.len;
    if (opts.max_issues_in_message > 0 && opts.max_issues_in_message < limit) {
        limit = opts.max_issues_in_message;
    }

    let i: usize = 0;
    while (i < limit) : (i += 1) {
        if (i > 0) {
            pos = zverWriteStr(out, pos, opts.issue_separator);
        }
        const issue = issues[i];
        pos = zverWriteStr(out, pos, issue.message);
        if (opts.include_code && issue.code.len > 0) {
            pos = zverWriteStr(out, pos, " [");
            pos = zverWriteStr(out, pos, issue.code);
            pos = zverWriteByte(out, pos, ']');
        }
        if (opts.include_path && issue.path.len > 0) {
            pos = zverWriteStr(out, pos, " at \"");
            pos = zverWriteStr(out, pos, issue.path);
            pos = zverWriteByte(out, pos, '"');
        }
    }

    return pos;
}

/// Zero-heap variant: formats issues into caller-provided `out` and wraps in `ValidationError`.
pub fn fromZodErrorInto(
    issues: []const ValidationIssue,
    opts: MessageBuilderOptions,
    out: []u8,
): ValidationError {
    let details_count = issues.len;
    let truncated = false;
    if (opts.max_issues_in_message > 0 && issues.len > opts.max_issues_in_message) {
        details_count = opts.max_issues_in_message;
        truncated = true;
    }
    const n = createMessage(issues, opts, out);
    return ValidationError{
        name: VALIDATION_ERROR_NAME,
        message_text: out[0..n] as string,
        details_count: details_count,
        total_issues: issues.len,
        truncated: truncated,
    };
}

/// Converts a slice of Zod-like validation issues into a structured `ValidationError`.
/// Modeled on `fromZodError(zodError, options)`.
pub fn fromZodError(
    issues: []const ValidationIssue,
    opts: MessageBuilderOptions,
): ValidationError {
    let tmp: [512]u8 = [0; 512];
    let details_count = issues.len;
    let truncated = false;
    if (opts.max_issues_in_message > 0 && issues.len > opts.max_issues_in_message) {
        details_count = opts.max_issues_in_message;
        truncated = true;
    }
    const n = createMessage(issues, opts, tmp[0..512]);
    const owned = std.heap.page_allocator.dupe(u8, tmp[0..n]) catch "";
    return ValidationError{
        name: VALIDATION_ERROR_NAME,
        message_text: owned,
        details_count: details_count,
        total_issues: issues.len,
        truncated: truncated,
    };
}

/// Alias matching `zod-validation-error` v3+ `fromError(err, options)`.
pub fn fromError(
    issues: []const ValidationIssue,
    opts: MessageBuilderOptions,
): ValidationError {
    return fromZodError(issues, opts);
}

/// Wraps a plain non-Zod error message string into a `ValidationError`.
pub fn fromErrorMessage(msg: string, opts: MessageBuilderOptions): ValidationError {
    let tmp: [512]u8 = [0; 512];
    let pos: usize = 0;
    if (opts.prefix.len > 0) {
        pos = zverWriteStr(tmp[0..512], pos, opts.prefix);
        if (msg.len > 0) {
            pos = zverWriteStr(tmp[0..512], pos, opts.prefix_separator);
        }
    }
    pos = zverWriteStr(tmp[0..512], pos, msg);
    const owned = std.heap.page_allocator.dupe(u8, tmp[0..pos]) catch "";
    return ValidationError{
        name: VALIDATION_ERROR_NAME,
        message_text: owned,
        details_count: 0,
        total_issues: 0,
        truncated: false,
    };
}

/// Type-guard helper matching `isValidationError(err)`.
pub fn isValidationError(error_name: string): bool {
    if (error_name.len != VALIDATION_ERROR_NAME.len) { return false; }
    let i: usize = 0;
    while (i < error_name.len) : (i += 1) {
        if (error_name[i] != VALIDATION_ERROR_NAME[i]) { return false; }
    }
    return true;
}

/// Structural type-guard helper matching `isValidationErrorLike(err)`.
pub fn isValidationErrorLike(error_name: string, has_details: bool): bool {
    return isValidationError(error_name) && has_details;
}
