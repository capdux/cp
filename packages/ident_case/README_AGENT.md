# ident_case — AI Agent Quick Ref

Zero-heap identifier case conversion (`camelCase`, `PascalCase`, `snake_case`, `kebab-case`, `SCREAMING_*`) with smart acronym splitting (crates.io `ident_case` parity).

## Import

```cp
import {
    RenameRule,
    toLowerCase, toUpperCase,
    toCamelCase, toPascalCase,
    toSnakeCase, toKebabCase,
    toScreamingSnakeCase, toScreamingKebabCase,
    renameIdent, parseRenameRule,
} from "ident_case";
```

## API Signatures

```cp
enum RenameRule {
    LowerCase,
    UpperCase,
    CamelCase,
    PascalCase,
    SnakeCase,
    ScreamingSnakeCase,
    KebabCase,
    ScreamingKebabCase
}

fn toLowerCase(input: string, out: []u8): usize;
fn toUpperCase(input: string, out: []u8): usize;
fn toCamelCase(input: string, out: []u8): usize;
fn toPascalCase(input: string, out: []u8): usize;
fn toSnakeCase(input: string, out: []u8): usize;
fn toKebabCase(input: string, out: []u8): usize;
fn toScreamingSnakeCase(input: string, out: []u8): usize;
fn toScreamingKebabCase(input: string, out: []u8): usize;
fn renameIdent(rule: RenameRule, input: string, out: []u8): usize;
fn parseRenameRule(name: string): RenameRule?;
```

## Minimal Example

```cp
import { toSnakeCase, parseRenameRule, renameIdent } from "ident_case";

let out: [64]u8 = undefined;
let n = toSnakeCase("HTTPResponse", out[0..64]); // out[0..n] == "http_response"

let rule = parseRenameRule("kebab-case");
if (rule != null) {
    let m = renameIdent(rule!, "myFieldName", out[0..64]); // "my-field-name"
}
```

## Pitfalls & Borrowing Rules

- **Output buffer sizing**: Snake/kebab conversion can be longer than `input` due to inserted `_` or `-` separators; allocate `out.len >= input.len * 2` to prevent truncation.
- **Optional return**: `parseRenameRule` returns `RenameRule?` (`null` on unrecognized rule string); check `!= null` before unwrapping.
- **Accepted rule names**: `"lowercase"`, `"UPPERCASE"`, `"camelCase"`, `"PascalCase"`, `"snake_case"`, `"SCREAMING_SNAKE_CASE"`, `"kebab-case"`, `"SCREAMING-KEBAB-CASE"`.
- **ASCII identifiers**: Word splitting and case folding operate on ASCII alphanumeric bytes; input `string` is borrowed immutably.
