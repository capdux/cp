# toidentifier (AI Agent Quick Ref)

Zero-heap converter from word phrases to PascalCase/camelCase/CONST_CASE/snake_case/safe identifiers + validators.

## Import
```typescript
import {
    toIdentifierLen, toIdentifier, toIdentifierChecked,
    toSafeIdentifier, toCamelIdentifier, toConstIdentifier, toSnakeIdentifier,
    isValidIdentifier, isValidJsIdentifier,
} from "toidentifier";
```

## API Signatures
- `toIdentifierLen(input: string): usize`
- `toIdentifier(input: string, out: []u8): usize` (PascalCase, matches npm `toidentifier`)
- `toIdentifierChecked(input: string, out: []u8): usize?` (`null` if `out.len < toIdentifierLen(input)`)
- `toSafeIdentifier(input: string, out: []u8): usize` (prepends `_` if leading digit or empty)
- `toCamelIdentifier(input: string, out: []u8): usize` (`camelCase`)
- `toConstIdentifier(input: string, out: []u8): usize` (`UPPER_SNAKE_CASE`)
- `toSnakeIdentifier(input: string, out: []u8): usize` (`lower_snake_case`)
- `isValidIdentifier(s: string): bool` (`[A-Za-z_][A-Za-z0-9_]*`)
- `isValidJsIdentifier(s: string): bool` (`[A-Za-z_$][A-Za-z0-9_$]*`)

## Minimal Example
```typescript
import { toIdentifierChecked, isValidIdentifier } from "toidentifier";

fn makeName(phrase: string, out: []u8): usize {
    if (let n = toIdentifierChecked(phrase, out)) {
        return n;
    }
    return 0;
}
```

## Pitfalls & Borrowing Rules
- `toIdentifier` mirrors npm `toidentifier`: inputs starting with digits (e.g. `"404 Not Found"`) produce `"404NotFound"` which fails `isValidIdentifier`; use `toSafeIdentifier` when a guaranteed valid identifier is needed.
- `toIdentifierChecked` returns optional `usize?`; unwrap with `if (let n = toIdentifierChecked(s, out))`.
- Unchecked variants (`toIdentifier`, `toCamelIdentifier`, `toConstIdentifier`, `toSnakeIdentifier`, `toSafeIdentifier`) stop writing if `out` fills up; allocate `out.len >= input.len + 1` (or `2 * input.len` for snake/const on camelCase input).
- `out` must be a mutable byte slice (`[]u8`) and not alias `input`.
- Only ASCII letters/digits are classified via `cap:ascii`; non-ASCII bytes are stripped.
