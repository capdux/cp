[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ajv-formats

Zero-allocation JSON Schema Draft 2019-09/2020-12 string format validators: date, time, date-time, email, hostname, ipv4, ipv6, uuid, uri, json-pointer (clean-room MIT implementation of npm ajv-formats, #205 / 212M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ajv-formats` ([https://github.com/ajv-validator/ajv-formats](https://github.com/ajv-validator/ajv-formats)) | 许可：`MIT`

| Metadata | Value |
| --- | --- |
| Package | `ajv-formats` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ajv-formats
```

```cp
import {
    ajvfIsDigit,
    ajvfIsAlpha,
    ajvfIsAlphaNum,
    ajvfIsHex,
    ajvfParse2Digits,
    ajvfParse4Digits,
    ajvfIsLeapYear,
    ajvfDaysInMonth,
} from "ajv-formats";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ajvfIsDigit` | `(b: u8): bool` | Invokes `ajvfIsDigit` and returns `bool`. |
| `ajvfIsAlpha` | `(b: u8): bool` | Invokes `ajvfIsAlpha` and returns `bool`. |
| `ajvfIsAlphaNum` | `(b: u8): bool` | Invokes `ajvfIsAlphaNum` and returns `bool`. |
| `ajvfIsHex` | `(b: u8): bool` | Invokes `ajvfIsHex` and returns `bool`. |
| `ajvfParse2Digits` | `(s: string, offset: usize): i32` | Invokes `ajvfParse2Digits` and returns `i32`. |
| `ajvfParse4Digits` | `(s: string, offset: usize): i32` | Invokes `ajvfParse4Digits` and returns `i32`. |
| `ajvfIsLeapYear` | `(year: u32): bool` | Invokes `ajvfIsLeapYear` and returns `bool`. |
| `ajvfDaysInMonth` | `(year: u32, month: u32): u32` | Invokes `ajvfDaysInMonth` and returns `u32`. |
| `isDate` | `(s: string): bool` | Checks predicate `isDate` and returns `bool`. |
| `isTime` | `(s: string): bool` | Checks predicate `isTime` and returns `bool`. |
| `isDateTime` | `(s: string): bool` | Checks predicate `isDateTime` and returns `bool`. |
| `isHostname` | `(s: string): bool` | Checks predicate `isHostname` and returns `bool`. |
| `ajvfIsEmailLocalChar` | `(b: u8): bool` | Invokes `ajvfIsEmailLocalChar` and returns `bool`. |
| `isEmail` | `(s: string): bool` | Checks predicate `isEmail` and returns `bool`. |
| `isIpv4` | `(s: string): bool` | Checks predicate `isIpv4` and returns `bool`. |
| `isIpv6` | `(s: string): bool` | Checks predicate `isIpv6` and returns `bool`. |
| `isUuid` | `(s: string): bool` | Checks predicate `isUuid` and returns `bool`. |
| `ajvfIsSchemeChar` | `(b: u8): bool` | Invokes `ajvfIsSchemeChar` and returns `bool`. |
| `isUri` | `(s: string): bool` | Checks predicate `isUri` and returns `bool`. |
| `isJsonPointer` | `(s: string): bool` | Checks predicate `isJsonPointer` and returns `bool`. |
| `ajvfStrEq` | `(a: string, b: string): bool` | Invokes `ajvfStrEq` and returns `bool`. |
| `validateFormat` | `(format_name: string, value: string): bool` | Validates input with `validateFormat` returning `bool`. |

## Minimal Example

```cp
import { isDate } from "ajv-formats";

pub fn main() {
    let result = isDate("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
