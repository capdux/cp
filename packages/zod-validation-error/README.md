[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# zod-validation-error

Human-readable validation error aggregator and formatter for CP, modeled on npm [`zod-validation-error`](https://github.com/causaly/zod-validation-error) v3.4.0 (MIT, CP v3.4.0).

Formats structured validation issues into clean, readable error summaries with JS-style path notation (`users[0].email`, `config["api-key"]`), configurable prefixes/separators, issue truncation (`max_issues_in_message`), optional error code display, and both zero-heap (`fromZodErrorInto` / `createMessage`) and convenience (`fromZodError` / `fromError`) APIs.

## Installation & Import

```bash
cap add zod-validation-error
```

```cp
import {
    ValidationIssue,
    FormatErrorOptions,
    MessageBuilderOptions,
    ValidationError,
    defaultMessageBuilderOptions,
    createMessage,
    fromZodErrorInto,
    fromZodError,
    formatIssuePath,
} from "zod-validation-error";
```

## Core API Reference

### Constants & Data Structures

| Symbol | Fields / Methods | Description |
| :--- | :--- | :--- |
| `VALIDATION_ERROR_NAME` | `"ZodValidationError"` | Standard `ValidationError.name` identifier. |
| `ValidationIssue` | `path: string`, `message: string`, `code: string` | Single validation issue descriptor. |
| `FormatErrorOptions` | `prefix`, `prefix_sep`, `issue_sep`, `max_issues`, `include_path` | Compact formatting options for `formatValidationIssues`. |
| `MessageBuilderOptions` | `prefix`, `prefix_separator`, `issue_separator`, `union_separator`, `max_issues_in_message`, `include_path`, `include_code` | Full npm `zod-validation-error` message builder options. |
| `ValidationError` | `name`, `message_text`, `details_count`, `total_issues`, `truncated` | Structured error object with `message()`, `toString()`, `copyMessage(out: []u8): usize`, `issueCount()`, `detailsCount()`, `isTruncated()`, `isValidationError()`. |

### Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `defaultFormatErrorOptions` | `(): FormatErrorOptions` | Default compact options (`prefix: "Validation error"`, `prefix_sep: ": "`, `issue_sep: "; "`). |
| `defaultMessageBuilderOptions` | `(): MessageBuilderOptions` | Default full options matching npm `zod-validation-error`. |
| `formatIssuePath` | `(segments: &[]string, out: []u8): usize` | Formats path segments using dot, numeric `[0]`, or quoted `["a-b"]` syntax into `out`. |
| `formatValidationIssue` | `(issue: ValidationIssue, include_path: bool, include_code: bool, out: []u8): usize` | Formats a single issue into `out`, returning bytes written. |
| `formatValidationIssues` | `(issues: &[]ValidationIssue, opts: FormatErrorOptions, out: []u8): usize` | Zero-heap formatting of an issue slice into `out`. |
| `createMessage` | `(issues: &[]ValidationIssue, opts: MessageBuilderOptions, out: []u8): usize` | Zero-heap full message builder writing into `out`. |
| `fromZodErrorInto` | `(issues: &[]ValidationIssue, opts: MessageBuilderOptions, out: []u8): ValidationError` | Zero-heap constructor; `ValidationError.message_text` borrows `out[0..n]`. |
| `fromZodError` / `fromError` | `(issues: &[]ValidationIssue, opts: MessageBuilderOptions): ValidationError` | Convenience constructors returning a self-contained `ValidationError` (up to 512-byte message). |
| `fromErrorMessage` | `(msg: string, opts: MessageBuilderOptions): ValidationError` | Wraps a single plain error message string into a `ValidationError`. |
| `isValidationError` | `(error_name: string): bool` | Checks whether `error_name == "ZodValidationError"`. |
| `isValidationErrorLike` | `(error_name: string, has_details: bool): bool` | Duck-type check for validation error objects. |

## Usage Examples

### 1. Zero-Heap Formatting with `fromZodErrorInto`

```cp
import {
    ValidationIssue,
    defaultMessageBuilderOptions,
    fromZodErrorInto,
} from "zod-validation-error";

fn formatUserErrors() {
    let issues = [
        ValidationIssue { path: "email", message: "Invalid email", code: "invalid_string" },
        ValidationIssue { path: "age", message: "Must be >= 18", code: "too_small" },
    ];
    let opts = defaultMessageBuilderOptions();
    let buf: [256]u8 = undefined;
    let err = fromZodErrorInto(&issues, opts, buf[0..256]);
    // err.message() -> "Validation error: Invalid email at \"email\"; Must be >= 18 at \"age\""
    // err.issueCount() == 2, err.isTruncated() == false
}
```

### 2. JS-Style Issue Path Formatting

```cp
import { formatIssuePath } from "zod-validation-error";

fn buildPath(): usize {
    let segs = ["users", "0", "api-key"];
    let out: [64]u8 = undefined;
    return formatIssuePath(&segs, out[0..64]);
    // writes: users[0]["api-key"]
}
```

### 3. Truncated Summaries with Custom Options

```cp
import {
    ValidationIssue,
    defaultMessageBuilderOptions,
    createMessage,
} from "zod-validation-error";

fn truncatedSummary(): usize {
    let issues = [
        ValidationIssue { path: "a", message: "Required", code: "invalid_type" },
        ValidationIssue { path: "b", message: "Required", code: "invalid_type" },
    ];
    let mut opts = defaultMessageBuilderOptions();
    opts.max_issues_in_message = 1;
    opts.include_code = true;
    let out: [256]u8 = undefined;
    return createMessage(&issues, opts, out[0..256]);
}
```

## Differences from Upstream & Zero-Copy Highlights

- **Zero-Heap Fast Path**: `createMessage`, `formatValidationIssues`, `formatIssuePath`, and `fromZodErrorInto` perform zero heap allocations by writing directly into caller-provided `[]u8` buffers.
- **Explicit Issue Slice**: Replaces runtime JS `ZodError` inspection with a flat `&[]ValidationIssue` slice, decoupling formatting from any single schema library.
- **Safe Truncation**: Respects both `max_issues_in_message` and destination buffer capacity without ever overflowing `out`.
