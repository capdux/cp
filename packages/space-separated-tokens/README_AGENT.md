# space-separated-tokens — AI Agent Quick Ref

Zero-allocation HTML/SVG space-separated and comma-separated token parser, inspector, and serializer (npm `space-separated-tokens` 2.0.2 parity).

## Import

```cp
import {
    parseSpaceTokens, parse,
    countTokens, tokenAt,
    hasSpaceToken, containsToken,
    stringifySpaceTokens, stringify,
    normalizeSpaceTokens,
    parseCommaTokens, stringifyCommaTokens,
} from "space-separated-tokens";
```

## API Signatures

```cp
fn parseSpaceTokens(input: string, out: []string): usize;
fn parse(input: string, out: []string): usize;
fn countTokens(input: string): usize;
fn tokenAt(input: string, index: usize): string?;
fn hasSpaceToken(input: string, target: string): bool;
fn containsToken(input: string, target: string): bool;
fn stringifySpaceTokens(tokens: []const string, out: []u8): usize;
fn stringify(tokens: []const string, out: []u8): usize;
fn normalizeSpaceTokens(input: string, out: []u8): usize;
fn parseCommaTokens(input: string, out: []string): usize;
fn stringifyCommaTokens(tokens: []const string, out: []u8): usize;
```

## Minimal Example

```cp
import { parse, hasSpaceToken, stringify } from "space-separated-tokens";

let tokens: [8]string = undefined;
let n = parse("  a   b\tc ", tokens[0..8]); // n == 3
let ok = hasSpaceToken("btn active", "active"); // true

let buf: [64]u8 = undefined;
let m = stringify(tokens[0..n], buf[0..64]); // buf[0..m] == "a b c"
```

## Pitfalls & Borrowing Rules

- **Lifetime of parsed slices**: `parse*` and `tokenAt` return zero-copy sub-slices borrowing from `input`; do not let `input` be freed or overwritten while using the returned slices.
- **Output cap**: `parse` / `parseCommaTokens` stop writing once `out.len` is reached; call `countTokens(input)` first if exact capacity is needed.
- **Aliases**: `parse` = `parseSpaceTokens`, `containsToken` = `hasSpaceToken`, `stringify` = `stringifySpaceTokens`.
- **Optional return**: `tokenAt` returns `string?` (`null` if `index >= countTokens(input)`).
