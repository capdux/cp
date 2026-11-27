[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# credit-card-type

Zero-allocation payment card BIN network detector, Luhn validator, formatter, and PCI-DSS masker for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:credit-card-type` ([https://github.com/braintree/credit-card-type](https://github.com/braintree/credit-card-type))（原库名 `credit-card-type` → CP 包名 `credit-card-type`）

| Metadata | Value |
| --- | --- |
| Package | `credit-card-type` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add credit-card-type
```

```cp
import {
    CardNetwork,
    CardInfo,
    ExtractResult,
    extractDigits,
    prefixNum,
    detectNetwork,
    checkValidLength,
    networkCvvLen,
} from "credit-card-type";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `CardNetwork` | `Visa, Mastercard, Amex, UnionPay, Jcb, Discover, DinersClub, Maestro ...` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CardInfo` | `class` | Fields: `network: CardNetwork, digit_count: usize, valid_length: bool, luhn_ok: bool, is_valid: bool, cvv_len: u8` |
| `ExtractResult` | `class` | Fields: `count: usize, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `extractDigits` | `(raw: string, out: &mut [MAX_CARD_DIGITS]u8): ExtractResult` | Invokes `extractDigits` and returns `ExtractResult`. |
| `prefixNum` | `(digits: &mut [MAX_CARD_DIGITS]u8, count: usize, len: usize): u32` | Invokes `prefixNum` and returns `u32`. |
| `detectNetwork` | `(digits: &mut [MAX_CARD_DIGITS]u8, count: usize): CardNetwork` | Invokes `detectNetwork` and returns `CardNetwork`. |
| `checkValidLength` | `(net: CardNetwork, len: usize): bool` | Validates input with `checkValidLength` returning `bool`. |
| `networkCvvLen` | `(net: CardNetwork): u8` | Invokes `networkCvvLen` and returns `u8`. |
| `checkLuhnDigits` | `(digits: &mut [MAX_CARD_DIGITS]u8, count: usize): bool` | Validates input with `checkLuhnDigits` returning `bool`. |
| `luhnValid` | `(raw: string): bool` | Invokes `luhnValid` and returns `bool`. |
| `inspectCard` | `(raw: string): CardInfo` | Invokes `inspectCard` and returns `CardInfo`. |
| `networkName` | `(n: CardNetwork): string` | Invokes `networkName` and returns `string`. |
| `formatCardNumber` | `(raw: string, out: []u8): string` | Serializes/formats data via `formatCardNumber` returning `string`. |
| `maskCardPci` | `(raw: string, out: []u8): string` | Invokes `maskCardPci` and returns `string`. |

## Minimal Example

```cp
import { formatCardNumber } from "credit-card-type";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatCardNumber("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
