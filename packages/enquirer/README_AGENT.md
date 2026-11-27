# enquirer — AI Agent Quick Ref

Zero-heap pure CP interactive CLI prompt state machines for Confirm, Select, and MultiSelect (clean-room MIT implementation of npm enquirer v2) (`v1.0.0`, `MIT`)

## Import

```cp
import { ConfirmPrompt, SelectChoice, SelectPrompt, MultiSelectPrompt } from "enquirer";
```

## Core Types & Constants

```cp
class ConfirmPrompt { message: string, initial: bool } // methods: new(...) +2
class SelectChoice { label: string, enabled: bool }
class SelectPrompt { message: string, choices: [16]SelectChoice, choice_count: usize, cursor: usize, /* +1 fields */ } // methods: new(...) +5
class MultiSelectPrompt { message: string, choices: [16]string, selected: [16]bool, choice_count: usize } // methods: new(...) +5
```

## Minimal Usage

```cp
import { ConfirmPrompt } from "enquirer";

pub fn main() {
    // Refer to src/index.d.cp for ConfirmPrompt fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"enquirer"`; install via `cap add enquirer`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add enquirer
cpc check src/main.cp
```
