# zod-validation-error — AI Agent Quick Ref

Human-readable validation error aggregator and formatter with zero-heap buffer APIs (npm `zod-validation-error` v3.4.0 parity).

## Import

```cp
import {
    ValidationIssue, FormatErrorOptions, MessageBuilderOptions, ValidationError,
    defaultFormatErrorOptions, defaultMessageBuilderOptions,
    formatIssuePath, formatValidationIssue, formatValidationIssues,
    createMessage, fromZodErrorInto, fromZodError, fromError, fromErrorMessage,
    isValidationError, isValidationErrorLike,
} from "zod-validation-error";
```

## Core API Signatures

```cp
// Constant: VALIDATION_ERROR_NAME = "ZodValidationError"
class ValidationIssue { path: string, message: string, code: string }
class FormatErrorOptions { prefix, prefix_sep, issue_sep: string, max_issues: usize, include_path: bool }
class MessageBuilderOptions {
    prefix, prefix_separator, issue_separator, union_separator: string,
    max_issues_in_message: usize, include_path: bool, include_code: bool,
}
class ValidationError {
    name, message_text: string, details_count, total_issues: usize, truncated: bool,
    fn message(): string; fn toString(): string; fn copyMessage(out: []u8): usize;
    fn issueCount(): usize; fn detailsCount(): usize; fn isTruncated(): bool; fn isValidationError(): bool;
}

defaultFormatErrorOptions(): FormatErrorOptions
defaultMessageBuilderOptions(): MessageBuilderOptions
formatIssuePath(segments: &[]string, out: []u8): usize
formatValidationIssue(issue: ValidationIssue, include_path: bool, include_code: bool, out: []u8): usize
formatValidationIssues(issues: &[]ValidationIssue, opts: FormatErrorOptions, out: []u8): usize
createMessage(issues: &[]ValidationIssue, opts: MessageBuilderOptions, out: []u8): usize
fromZodErrorInto(issues: &[]ValidationIssue, opts: MessageBuilderOptions, out: []u8): ValidationError
fromZodError(issues: &[]ValidationIssue, opts: MessageBuilderOptions): ValidationError
fromError(issues: &[]ValidationIssue, opts: MessageBuilderOptions): ValidationError
fromErrorMessage(msg: string, opts: MessageBuilderOptions): ValidationError
isValidationError(error_name: string): bool
isValidationErrorLike(error_name: string, has_details: bool): bool
```

## Minimal Example

```cp
import { ValidationIssue, defaultMessageBuilderOptions, fromZodErrorInto } from "zod-validation-error";

fn demo(): string {
    let issues = [ValidationIssue { path: "user.email", message: "Invalid", code: "invalid_string" }];
    let buf: [256]u8 = undefined;
    let err = fromZodErrorInto(&issues, defaultMessageBuilderOptions(), buf[0..256]);
    return err.message();
}
```

## Gotchas & Borrow Rules

- **Zero-heap vs heap**: Prefer `fromZodErrorInto` / `createMessage` (zero-heap, writes into `out: []u8`); `fromZodError` / `fromError` / `fromErrorMessage` duplicate up to 512 bytes via `page_allocator`.
- **Lifetime of `fromZodErrorInto`**: The returned `ValidationError.message_text` slices `out`; keep `out` alive while using `err.message()`.
- **Read-only slice inputs**: Pass `&issues` (`&[]ValidationIssue`) and `&segments` (`&[]string`).
- **Truncation**: Set `max_issues_in_message = 0` for unlimited issues, or `> 0` to cap formatted entries (`err.isTruncated()` reflects truncation).
