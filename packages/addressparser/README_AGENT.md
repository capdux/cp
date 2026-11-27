# addressparser — AI Agent Quick Ref

Zero-allocation RFC 5322 email address and recipient list parser and formatter for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MailAddress,
    AddressList,
    invalidMail,
    stripEdgeComments,
    isValidLocal,
    isValidDomain,
    makeAddrFromSpec,
    unwrapDisplayName,
} from "addressparser";
```

## Core Types & Constants

```cp
class MailAddress { valid: bool, display_name: string, address: string, local: string, /* +1 fields */ }
class AddressList { items: [MAX_ADDRESSES]MailAddress, count: usize } // methods: get(...)
```

## API Signatures

```cp
fn invalidMail(): MailAddress;
fn stripEdgeComments(raw: string): string;
fn isValidLocal(local: string): bool;
fn isValidDomain(domain: string): bool;
fn makeAddrFromSpec(displayName: string, addrSpec: string): MailAddress;
fn unwrapDisplayName(rawName: string): string;
fn parseMailbox(raw: string): MailAddress;
fn parseAddressList(raw: string): AddressList;
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn needsQuoting(name: string): bool;
fn formatMailbox(m: MailAddress, out: []u8): string;
```

## Minimal Usage

```cp
import { isValidLocal } from "addressparser";

pub fn main() {
    let result = isValidLocal("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"addressparser"`; install via `cap add addressparser`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
