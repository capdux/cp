# requires-port — AI Agent Quick Ref

Zero-heap checker determining whether a port number is non-default for a given URL protocol (`v1.0.0`, `MIT`)

## Import

```cp
import { rptNormalizeProtocol, defaultPortForProtocol, requiresPort } from "requires-port";
```

## API Signatures

```cp
fn rptNormalizeProtocol(proto: string): string;
fn defaultPortForProtocol(protocol: string): u16;
fn requiresPort(port: u16, protocol: string): bool;
```

## Minimal Usage

```cp
import { rptNormalizeProtocol } from "requires-port";

pub fn main() {
    let result = rptNormalizeProtocol("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"requires-port"`; install via `cap add requires-port`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add requires-port
cpc check src/main.cp
```
