# zod-validation-error@3.4.0 (Agent Quickref)

> Zero-heap human-readable validation error aggregator and formatter (npm zod-validation-error equivalent) | License: MIT
> Import: `import { ... } from "zod-validation-error";` | Full types: `src/index.d.cp`

## Functions
- `export fn defaultFormatErrorOptions(): FormatErrorOptions;`
- `export fn defaultMessageBuilderOptions(): MessageBuilderOptions;`
- `export fn zverWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn zverWriteByte(out: []u8, pos: usize, ch: u8): usize;`
- `export fn zverIsIdentChar(ch: u8): bool;`
- `export fn zverIsDigitsOnly(s: string): bool;`
- `export fn zverIsSimpleIdentifier(s: string): bool;`
- `export fn formatIssuePath(segments: &[]string, out: []u8): usize;`
- `export fn formatValidationIssue(issue: ValidationIssue, include_path: bool, include_code: bool, out: []u8): usize;`
- `export fn formatValidationIssues(issues: &[]ValidationIssue, opts: FormatErrorOptions, out: []u8): usize;`
- `export fn createMessage(issues: &[]ValidationIssue, opts: MessageBuilderOptions, out: []u8): usize;`
- `export fn fromZodErrorInto(issues: &[]ValidationIssue, opts: MessageBuilderOptions, out: []u8): ValidationError;`
- `export fn fromZodError(issues: &[]ValidationIssue, opts: MessageBuilderOptions): ValidationError;`
- `export fn fromError(issues: &[]ValidationIssue, opts: MessageBuilderOptions): ValidationError;`
- `export fn fromErrorMessage(msg: string, opts: MessageBuilderOptions): ValidationError;`
- `export fn isValidationError(error_name: string): bool;`
- `export fn isValidationErrorLike(error_name: string, has_details: bool): bool;`
