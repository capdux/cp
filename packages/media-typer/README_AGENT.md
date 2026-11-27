# media-typer — AI Agent Quick Ref

Zero-heap RFC 6838 HTTP media type parser, validator, tree classifier, and canonical lowercase formatter.

## Import
```typescript
import {
    MediaType, parse, parseMediaType, format, formatMediaType,
    testMediaType, testType, essence, mediaTypeEq, hasSuffix,
    isValidType, isValidSubtype, isValidSuffix,
    isStandardType, isVendorSubtype, isPersonalSubtype, isUnregisteredSubtype
} from "media-typer";
```

## API Signatures
```typescript
export class MediaType {
    valid: bool, main_type: string, subtype: string, suffix: string,
    fn validate(raw: string): bool;
    fn isValid(): bool;
    fn hasSuffix(): bool;
    fn formatTo(out: []u8): usize;
}

export fn isValidType(s: string): bool;
export fn isValidSubtype(s: string): bool;
export fn isValidSuffix(s: string): bool;
export fn testMediaType(raw: string): bool;
export fn testType(raw: string): bool;
export fn parseMediaType(raw: string): MediaType;
export fn parse(raw: string): MediaType;
export fn formatMediaType(main_type: string, subtype: string, suffix: string, out: []u8): usize;
export fn format(mt: MediaType, out: []u8): usize;
export fn essence(raw: string, out: []u8): usize;
export fn mediaTypeEq(a: string, b: string): bool;
export fn hasSuffix(raw: string, suffix: string): bool;
export fn isStandardType(t: string): bool;
export fn isVendorSubtype(subtype: string): bool;
export fn isPersonalSubtype(subtype: string): bool;
export fn isUnregisteredSubtype(subtype: string): bool;
```

## Minimal Example
```typescript
import { parse, format, mediaTypeEq } from "media-typer";

fn normalize(raw: string, out: []u8): usize {
    let mt = parse(raw);
    if (!mt.isValid()) { return 0; }
    let _ = mediaTypeEq(raw, "application/problem+json");
    return format(mt, out);
}
```

## Pitfalls & Rules
- Does NOT accept Content-Type parameters (e.g. `; charset=utf-8`); strip parameters before calling `parse` or `testMediaType`.
- `parse` never throws; always check `mt.isValid()` before using `main_type`/`subtype`/`suffix`.
- `MediaType` string fields borrow from `raw` (zero-copy).
- `format` / `formatMediaType` / `essence` / `formatTo` write lowercase ASCII into `out: []u8` and return `0` on invalid input.
- `parse`/`parseMediaType` and `testType`/`testMediaType` are exact aliases.
