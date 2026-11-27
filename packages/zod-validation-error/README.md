[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# zod-validation-error

Zero-heap human-readable validation error aggregator and formatter (`fromZodError` / `fromZodErrorInto` / `fromError` / `createMessage` / `formatIssuePath` / `isValidationError`) in pure CP.

## Source & Zero-Cost Migration

| Field | Value |
| --- | --- |
| **Upstream Registry** | [npmjs.com/package/zod-validation-error](https://www.npmjs.com/package/zod-validation-error) |
| **Upstream Package** | `zod-validation-error` v3.4.0 |
| **Upstream Repository** | https://github.com/causaly/zod-validation-error |
| **License** | MIT |

Keep the exact package name when migrating from TypeScript/npm to CP:

```ts
// TypeScript (npm)
import { fromZodError, isValidationError } from "zod-validation-error";
const err = fromZodError(zodError, { prefix: "Validation error", issueSeparator: "; " });
```

```cp
// CP — same package name, zero-heap validation error aggregation
import {
    ValidationIssue, ValidationError,
    defaultMessageBuilderOptions, fromZodErrorInto,
    fromZodError, fromError, isValidationError,
} from "zod-validation-error";
```

## Installation

```bash
cap add zod-validation-error
```

## Quick Start & Practical Examples

### 1. Building a `ValidationError` into a Stack Buffer (`fromZodErrorInto` & `createMessage`)

```cp
import {
    ValidationIssue, ValidationError,
    defaultMessageBuilderOptions, fromZodErrorInto,
    createMessage, isValidationError, isValidationErrorLike,
} from "zod-validation-error";

fn demoValidationError() {
    let issues: [2]ValidationIssue = [
        ValidationIssue { path: "user.email", message: "Invalid email", code: "invalid_string" },
        ValidationIssue { path: "items[0].qty", message: "Expected >= 1", code: "too_small" },
    ];

    mut opts = defaultMessageBuilderOptions();
    opts.prefix = "Validation error";
    opts.prefix_separator = ": ";
    opts.issue_separator = "; ";
    opts.include_path = true;
    opts.include_code = false;

    // Zero-heap: write formatted message into stack buffer and get ValidationError wrapper
    mut msg_buf: [256]u8 = [0; 256];
    let err: ValidationError = fromZodErrorInto(&issues, opts, &mut msg_buf);

    let text = err.toString();          // "Validation error: Invalid email at \"user.email\"; Expected >= 1 at \"items[0].qty\""
    let total = err.issueCount();       // 2
    let truncated = err.isTruncated();  // false
    let ok_type = err.isValidationError(); // true
    let by_name = isValidationError(err.name); // true
    let like = isValidationErrorLike(err.name, err.detailsCount() > 0); // true

    // Or format directly with createMessage
    mut out2: [256]u8 = [0; 256];
    let written = createMessage(&issues, opts, &mut out2);
}
```

### 2. Formatting Issue Paths & Custom Error Wrapping

```cp
import {
    ValidationIssue, defaultFormatErrorOptions, defaultMessageBuilderOptions,
    formatIssuePath, formatValidationIssue, formatValidationIssues,
    fromErrorMessage, fromZodError,
} from "zod-validation-error";

fn demoFormatting() {
    // JS-style dot / bracket path formatting: ["users", "0", "first-name"] -> users[0]["first-name"]
    let segs: [3]string = ["users", "0", "first-name"];
    mut path_buf: [128]u8 = [0; 128];
    let plen = formatIssuePath(&segs, &mut path_buf);

    // Format a single issue or list with FormatErrorOptions
    let issue = ValidationIssue { path: "port", message: "Must be integer", code: "invalid_type" };
    mut line_buf: [128]u8 = [0; 128];
    let l1 = formatValidationIssue(issue, true, true, &mut line_buf);

    let list: [1]ValidationIssue = [issue];
    let fopts = defaultFormatErrorOptions();
    let l2 = formatValidationIssues(&list, fopts, &mut line_buf);

    // Wrap a plain error message string
    let wrapped = fromErrorMessage("Unexpected token", defaultMessageBuilderOptions());
    let msg = wrapped.message();
}
```

## Performance & Safety Highlights

- **Zero Heap Allocations**: All message formatting (`fromZodErrorInto`, `createMessage`, `formatValidationIssues`, `formatIssuePath`) writes directly into caller-provided `[]u8` buffers without heap allocation.
- **Buffer Overflow Safe**: Every write helper stops cleanly at `out.len` and returns the exact byte count written.
- **Truncation Control**: `max_issues_in_message` / `max_issues` caps overly large error lists and exposes `err.isTruncated()` and `err.detailsCount()` vs `err.issueCount()`.
- **Smart JS/TS Path Formatting**: `formatIssuePath` formats numeric segments as `[0]`, identifiers as `.foo`, and special keys as `["a-b"]`.

## API Reference

See [`src/index.d.cp`](src/index.d.cp) for complete type signatures, or [`README_AGENT.md`](README_AGENT.md) for the compact AI-agent reference.
