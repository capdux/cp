# is-potential-custom-element-name — AI Agent Quick Ref

Zero-allocation WHATWG HTML `PotentialCustomElementName` validator and sanitizer (npm `is-potential-custom-element-name` parity).

## Import

```cp
import {
    RESERVED_CUSTOM_ELEMENT_NAMES_COUNT, PcenError, PcenValidationResult,
    isValidCustomElementStart, isPcenChar, reservedCustomElementNameAt,
    isReservedCustomElementName, validateCustomElementName,
    isPotentialCustomElementName, sanitizeCustomElementName,
} from "is-potential-custom-element-name";
```

## API Signatures

```cp
export const RESERVED_CUSTOM_ELEMENT_NAMES_COUNT: usize; // 8
export enum PcenError {
    Ok, Empty, TooShort, InvalidStartChar, MissingHyphen,
    ReservedName, InvalidUtf8, InvalidChar,
}
export class PcenValidationResult {
    valid: bool, err: PcenError, error_offset: usize,
}
export fn isValidCustomElementStart(b: u8): bool;
export fn isPcenChar(cp: u32): bool;
export fn reservedCustomElementNameAt(idx: usize): string?;
export fn isReservedCustomElementName(name: string): bool;
export fn validateCustomElementName(name: string): PcenValidationResult;
export fn isPotentialCustomElementName(name: string): bool;
export fn sanitizeCustomElementName(input: string, out: []u8): usize;
```

## Minimal Example

```cp
import { isPotentialCustomElementName, validateCustomElementName, sanitizeCustomElementName } from "is-potential-custom-element-name";

fn demo(): void {
    const ok = isPotentialCustomElementName("my-element"); // true
    const res = validateCustomElementName("annotation-xml"); // res.valid == false, ReservedName
    let mut out: [64]u8 = [0; 64];
    const n = sanitizeCustomElementName("BadName", out[0..]);
}
```

## Caveats & Borrowing Rules

- **Strict lowercase ASCII**: Uppercase ASCII letters (`'A'..'Z'`) are invalid everywhere in a custom element name; it must start with `'a'..'z'` and contain at least one `'-'`.
- **Reserved names**: All 8 WHATWG reserved names (`annotation-xml`, `color-profile`, `font-face`, `font-face-src`, `font-face-uri`, `font-face-format`, `font-face-name`, `missing-glyph`) are rejected.
- **UTF-8 validation**: Invalid UTF-8 sequences return `PcenError.InvalidUtf8` with `error_offset` set to the offending byte.
- **Mutable output**: `sanitizeCustomElementName` writes into mutable `out: []u8` and returns bytes written; allow headroom for prefix/hyphen insertion if `input` starts with a non-letter or lacks `'-'`.
