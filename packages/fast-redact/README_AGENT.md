# fast-redact — AI Agent Quick Ref

Zero-allocation PII & secret redaction/masking engine (fast-redact / maskdata / secrecy parity: emails, Luhn credit cards, JWT/API tokens, phones, IPv4, and sensitive JSON fields) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RedactOptions,
    isTokenChar,
    isEmailLocalChar,
    isDomainChar,
    appendStr,
    appendByte,
    startsWithAt,
    maskEmail,
} from "fast-redact";
```

## Core Types & Constants

```cp
class RedactOptions { redact_emails: bool, redact_tokens: bool, redact_cards: bool, redact_phones: bool, /* +1 fields */ } // methods: defaultAll(...) +1
```

## API Signatures

```cp
fn isTokenChar(c: u8): bool;
fn isEmailLocalChar(c: u8): bool;
fn isDomainChar(c: u8): bool;
fn appendStr(out: []u8, pos: usize, s: string): usize;
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn startsWithAt(s: string, idx: usize, prefix: string): bool;
fn maskEmail(email: string, out: []u8): string;
fn maskPhone(phone: string, out: []u8): string;
fn maskCard(card: string, out: []u8): string;
fn maskSecret(secret: string, out: []u8): string;
fn maskIp(ip: string, out: []u8): string;
fn checkLuhnRange(s: string, start: usize, end: usize): bool;
fn matchTokenEnd(s: string, idx: usize): usize;
fn matchEmailEnd(s: string, idx: usize): usize;
fn matchCardEnd(s: string, idx: usize): usize;
fn matchPhoneEnd(s: string, idx: usize): usize;
fn matchIpv4End(s: string, idx: usize): usize;
fn redactText(input: string, opts: RedactOptions, out: []u8): string;
fn containsSensitive(input: string): bool;
fn isSensitiveKey(key: string): bool;
fn findJsonStringEnd(s: string, open_quote: usize): usize;
fn redactJsonFields(json_str: string, out: []u8): string;
```

## Minimal Usage

```cp
import { isSensitiveKey } from "fast-redact";

pub fn main() {
    let result = isSensitiveKey("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"fast-redact"`; install via `cap add fast-redact`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
