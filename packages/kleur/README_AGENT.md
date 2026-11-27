# kleur — AI Agent Quick Ref

Zero-allocation composable chainable ANSI terminal style builder with nested close/reset recovery for CP (clean-room parity with npm kleur v4) (`v1.0.0`, `MIT`)

## Import

```cp
import { KleurChain } from "kleur";
```

## Core Types & Constants

```cp
class KleurChain { enabled: bool, stack: [8]StylePair, depth: usize } // methods: new(...) +30
```

## Minimal Usage

```cp
import { KleurChain } from "kleur";

pub fn main() {
    // Refer to src/index.d.cp for KleurChain fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"kleur"`; install via `cap add kleur`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add kleur
cpc check src/main.cp
```
