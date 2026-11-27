# credit-card-type — AI Agent Quick Ref

Zero-allocation payment card BIN network detector, Luhn validator, formatter, and PCI-DSS masker for CP (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
enum CardNetwork { Visa, Mastercard, Amex, UnionPay, Jcb, Discover, ... }
class CardInfo { network: CardNetwork, digit_count: usize, valid_length: bool, luhn_ok: bool, /* +2 fields */ }
class ExtractResult { count: usize, ok: bool }
```

## API Signatures

```cp
fn extractDigits(raw: string, out: &mut [MAX_CARD_DIGITS]u8): ExtractResult;
fn prefixNum(digits: &mut [MAX_CARD_DIGITS]u8, count: usize, len: usize): u32;
fn detectNetwork(digits: &mut [MAX_CARD_DIGITS]u8, count: usize): CardNetwork;
fn checkValidLength(net: CardNetwork, len: usize): bool;
fn networkCvvLen(net: CardNetwork): u8;
fn checkLuhnDigits(digits: &mut [MAX_CARD_DIGITS]u8, count: usize): bool;
fn luhnValid(raw: string): bool;
fn inspectCard(raw: string): CardInfo;
fn networkName(n: CardNetwork): string;
fn formatCardNumber(raw: string, out: []u8): string;
fn maskCardPci(raw: string, out: []u8): string;
```

## Minimal Usage

```cp
import { formatCardNumber } from "credit-card-type";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatCardNumber("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"credit-card-type"`; install via `cap add credit-card-type`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
