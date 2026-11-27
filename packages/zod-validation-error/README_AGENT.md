# zod-validation-error — AI Agent Quick Ref

- **Import**: `import { ValidationIssue, ValidationError, defaultMessageBuilderOptions, fromZodErrorInto, ... } from "zod-validation-error";`
- **Origin**: npm `zod-validation-error` v3.4.0 (MIT, zero-heap)
- **Full Types**: `src/index.d.cp`

## Core Types
```cp
class ValidationIssue { path: string, message: string, code: string }
class FormatErrorOptions {
    prefix: string, prefix_sep: string, issue_sep: string,
    max_issues: usize, include_path: bool,
}
class MessageBuilderOptions {
    prefix: string, prefix_separator: string, issue_separator: string,
    union_separator: string, max_issues_in_message: usize,
    include_path: bool, include_code: bool,
}
class ValidationError {
    name: string, message_text: string,
    details_count: usize, total_issues: usize, truncated: bool,
    fn message(): string;
    fn toString(): string;
    fn copyMessage(out: []u8): usize;
    fn issueCount(): usize;
    fn detailsCount(): usize;
    fn isTruncated(): bool;
    fn isValidationError(): bool;
}
```

## Functions
- `defaultFormatErrorOptions(): FormatErrorOptions`
- `defaultMessageBuilderOptions(): MessageBuilderOptions`
- `formatIssuePath(segments: &[]string, out: []u8): usize`
- `formatValidationIssue(issue: ValidationIssue, include_path: bool, include_code: bool, out: []u8): usize`
- `formatValidationIssues(issues: &[]ValidationIssue, opts: FormatErrorOptions, out: []u8): usize`
- `createMessage(issues: &[]ValidationIssue, opts: MessageBuilderOptions, out: []u8): usize`
- `fromZodErrorInto(issues: &[]ValidationIssue, opts: MessageBuilderOptions, out: []u8): ValidationError`
- `fromZodError(issues: &[]ValidationIssue, opts: MessageBuilderOptions): ValidationError`
- `fromError(issues: &[]ValidationIssue, opts: MessageBuilderOptions): ValidationError`
- `fromErrorMessage(msg: string, opts: MessageBuilderOptions): ValidationError`
- `isValidationError(error_name: string): bool`
- `isValidationErrorLike(error_name: string, has_details: bool): bool`

## Gotchas
1. Prefer `fromZodErrorInto(&issues, opts, &mut buf)` when you need the full formatted multi-issue string in `err.message()`, because `out` backs `err.message_text` without heap allocation.
2. Keep `buf` alive as long as the returned `ValidationError` from `fromZodErrorInto` is used.
3. `formatIssuePath` / `createMessage` return the exact byte count written into `out`; slice `out[0..written]`.
