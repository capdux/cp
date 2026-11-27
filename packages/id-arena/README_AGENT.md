# id-arena — AI Agent Quick Ref

Zero-allocation typed handle arena allocator with arena_id cross-arena misuse protection in pure CP (`v1.0.0`, `MIT OR Apache-2.0`)

## Import

```cp
import { ArenaId, IdArenaI64, IdArenaU64, idaIsValid } from "id-arena";
```

## Core Types & Constants

```cp
class ArenaId { index: usize, arena_id: u32 }
class IdArenaI64 { arena_id: u32, items: [128]i64, count: usize } // methods: new(...) +8
class IdArenaU64 { arena_id: u32, items: [128]u64, count: usize } // methods: new(...) +8
```

## API Signatures

```cp
fn idaIsValid(id: ArenaId, expected_arena_id: u32, count: usize): bool;
```

## Minimal Usage

```cp
import { idaIsValid } from "id-arena";

pub fn main() {
    let result = idaIsValid(/* id: ArenaId */, 0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"id-arena"`; install via `cap add id-arena`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
