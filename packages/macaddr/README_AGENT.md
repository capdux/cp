# macaddr — AI Agent Quick Ref

Zero-heap pure-CP IEEE 802 MAC-48/EUI-48 (MacAddr6) and EUI-64 (MacAddr8) hardware address parser, formatter, and RFC 4291 converter (crates.io macaddr equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import { MacAddr6, MacAddr8, macParseHexByte } from "macaddr";
```

## Core Types & Constants

```cp
class MacAddr6 { bytes: [6]u8 } // methods: new(...) +15
class MacAddr8 { bytes: [8]u8 } // methods: fromArray(...) +14
```

## API Signatures

```cp
fn macParseHexByte(hi_ch: u8, lo_ch: u8): u8?;
```

## Minimal Usage

```cp
import { macParseHexByte } from "macaddr";

pub fn main() {
    let result = macParseHexByte(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"macaddr"`; install via `cap add macaddr`.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
