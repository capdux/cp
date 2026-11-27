# email_address — AI Agent Quick Ref

Zero-heap RFC 5321/5322 email address parser and validator (`v0.2.9`, `MIT`)

## Import

```cp
import {
    EmailAddress,
    emIsAtext,
    emIsLabelChar,
    emTrim,
    emValidLocal,
    emValidDomain,
    parseEmail,
    isValidEmail,
} from "email_address";
```

## Core Types & Constants

```cp
class EmailAddress { local_part: string, domain: string, display_name: string, valid: bool }
```

## API Signatures

```cp
fn emIsAtext(b: u8): bool;
fn emIsLabelChar(b: u8): bool;
fn emTrim(s: string): string;
fn emValidLocal(local: string): bool;
fn emValidDomain(domain: string): bool;
fn parseEmail(input: string): EmailAddress;
fn isValidEmail(input: string): bool;
```

## Minimal Usage

```cp
import { parseEmail } from "email_address";

pub fn main() {
    let result = parseEmail("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"email_address"`; install via `cap add email_address`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
