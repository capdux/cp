[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# is-potential-custom-element-name

Zero-allocation WHATWG HTML Custom Elements `PotentialCustomElementName` validator and sanitizer in pure CP.

- **Upstream Alignment**: `npm` `is-potential-custom-element-name` (MIT, mathiasbynens/is-potential-custom-element-name)
- **Package Version**: `1.0.0`
- **Memory Model**: Zero heap allocation — pure UTF-8 code point decoding and stack-buffer sanitization.

## Installation & Import

```bash
cap add is-potential-custom-element-name
```

```cp
import {
    RESERVED_CUSTOM_ELEMENT_NAMES_COUNT,
    PcenError,
    PcenValidationResult,
    isValidCustomElementStart,
    isPcenChar,
    reservedCustomElementNameAt,
    isReservedCustomElementName,
    validateCustomElementName,
    isPotentialCustomElementName,
    sanitizeCustomElementName,
} from "is-potential-custom-element-name";
```

## Core API

| Symbol | Signature | Description |
| --- | --- | --- |
| `RESERVED_CUSTOM_ELEMENT_NAMES_COUNT` | `const RESERVED_CUSTOM_ELEMENT_NAMES_COUNT: usize` | Count of WHATWG reserved custom element names (`8`, e.g. `annotation-xml`, `color-profile`, `font-face`, etc.). |
| `PcenError` | `enum PcenError { Ok, Empty, TooShort, InvalidStartChar, MissingHyphen, ReservedName, InvalidUtf8, InvalidChar }` | Structured diagnostic reason why a candidate name failed validation. |
| `PcenValidationResult` | `class PcenValidationResult { valid: bool, err: PcenError, error_offset: usize }` | Detailed validation result including the byte offset where an error occurred. |
| `isValidCustomElementStart` | `fn isValidCustomElementStart(b: u8): bool` | Checks if `b` is a valid ASCII lowercase start byte (`'a'..'z'`). |
| `isPcenChar` | `fn isPcenChar(cp: u32): bool` | Checks if Unicode scalar `cp` is allowed by the WHATWG `PCENChar` production. |
| `reservedCustomElementNameAt` | `fn reservedCustomElementNameAt(idx: usize): string?` | Returns the `idx`-th WHATWG reserved name, or `null` if out of range. |
| `isReservedCustomElementName` | `fn isReservedCustomElementName(name: string): bool` | Returns `true` if `name` is one of the 8 WHATWG reserved hyphenated names. |
| `validateCustomElementName` | `fn validateCustomElementName(name: string): PcenValidationResult` | Validates `name` with full error code and failing byte offset. |
| `isPotentialCustomElementName` | `fn isPotentialCustomElementName(name: string): bool` | Boolean WHATWG `PotentialCustomElementName` check matching npm `is-potential-custom-element-name`. |
| `sanitizeCustomElementName` | `fn sanitizeCustomElementName(input: string, out: []u8): usize` | Sanitizes arbitrary `input` into a valid custom element name written into `out`; returns bytes written. |

## Examples

### 1. Boolean & Diagnostic Validation

```cp
import { isPotentialCustomElementName, validateCustomElementName, PcenError } from "is-potential-custom-element-name";

fn validateDemo(): void {
    const ok = isPotentialCustomElementName("x-card");       // true
    const bad = isPotentialCustomElementName("font-face");   // false (reserved)

    const res = validateCustomElementName("My-Widget");
    // res.valid == false, res.err == PcenError.InvalidStartChar, res.error_offset == 0
}
```

### 2. Sanitize Arbitrary Input into a Valid Custom Element Name

```cp
import { sanitizeCustomElementName } from "is-potential-custom-element-name";

fn sanitizeDemo(): void {
    let mut out: [64]u8 = [0; 64];
    const n = sanitizeCustomElementName("123_Widget!", out[0..]);
    // out[0..n] is guaranteed to be a valid PotentialCustomElementName
}
```

## Differences from Upstream & Highlights

- **Structured Diagnostics**: `validateCustomElementName` pinpoints the exact `PcenError` (`Empty`, `TooShort`, `InvalidStartChar`, `MissingHyphen`, `ReservedName`, `InvalidUtf8`, `InvalidChar`) and `error_offset`.
- **Zero-Allocation Sanitizer**: `sanitizeCustomElementName` transforms arbitrary strings into spec-compliant custom element names directly in a caller-owned stack buffer.
- **No Regex Engine**: Direct UTF-8 scalar decoding and range comparisons eliminate regex compilation and heap allocations.
