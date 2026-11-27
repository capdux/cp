# content-security-policy-builder — AI Agent Quick Ref

Zero-allocation W3C CSP Level 3 HTTP header builder and zero-copy policy parser in pure CP (npm `content-security-policy-builder` parity).

## Import

```cp
import {
    CspBuilder,
    CspDirectiveSpan,
    CspPolicy,
} from "content-security-policy-builder";
```

## API Signatures

```cp
class CspBuilder {
    buf: []u8, pos: usize, count: usize, overflow: bool,
    static fn init(buf: []u8): CspBuilder;
    fn addDirective(*self, name: &[]u8, sources: &[]&[]u8): bool;
    fn written(): &[]u8;
    fn len(): usize;
    fn isOverflow(): bool;
}

class CspDirectiveSpan {
    name_start: usize, name_len: usize,
    val_start: usize, val_len: usize,
}

class CspPolicy {
    directives: [32]CspDirectiveSpan, count: usize,
    static fn parse(header: &[]u8): CspPolicy;
    fn len(): usize;
    fn getDirective(header: &[]u8, name: &[]u8): &[]u8;
    fn hasDirective(header: &[]u8, name: &[]u8): bool;
    fn allowsUnsafeInline(header: &[]u8, directive: &[]u8): bool;
}
```

## Minimal Example

```cp
import { CspBuilder, CspPolicy } from "content-security-policy-builder";

let buf: [256]u8 = undefined;
let b = CspBuilder.init(buf[0..256]);
let srcs: [1][]const u8 = ["'self'"];
let _ = b.addDirective("default-src", srcs[0..1]);

let p = CspPolicy.parse(b.written());
let has_def = p.hasDirective(b.written(), "default-src");
```

## Pitfalls & Borrowing Rules

- **Pass original `header` to query methods**: `CspPolicy` stores index spans only; `getDirective`, `hasDirective`, and `allowsUnsafeInline` require passing the exact same `header` slice that was passed to `CspPolicy.parse`.
- **Directive cap**: `CspPolicy` stores up to 32 directives on the stack (`directives: [32]CspDirectiveSpan`).
- **Validation failures**: `addDirective` returns `false` if `name` has non-`[A-Za-z0-9-]` chars, any source contains `;`, `,`, or control bytes (`< 0x20` / `0x7F`), or `buf` overflows (`isOverflow() == true`).
- **Buffer lifetime**: `CspBuilder.written()` returns a read-only slice into `buf`; keep `buf` alive while using the slice.
