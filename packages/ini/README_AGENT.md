# ini — AI Agent Quick Ref

Zero-heap INI / .gitconfig / .npmrc / .editorconfig parser and writer with sections, comments, quoted values, and typed getters. (`v1.0.0`, `MIT`)

## Import

```cp
import { IniEntry, IniDoc, IniWriter, isWs } from "ini";
```

## Core Types & Constants

```cp
class IniEntry { section_start: usize, section_len: usize, key_start: usize, key_len: usize, /* +2 fields */ }
class IniDoc { entries: [128]IniEntry, count: usize } // methods: parse(...) +6
class IniWriter { buf: []u8, pos: usize, overflow: bool } // methods: init(...) +6
```

## API Signatures

```cp
fn isWs(c: u8): bool;
```

## Minimal Usage

```cp
import { isWs } from "ini";

pub fn main() {
    let result = isWs(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ini"`; install via `cap add ini`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
