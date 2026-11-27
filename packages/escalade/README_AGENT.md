# escalade — AI Agent Quick Ref

Zero-heap ancestor directory path walker (clean-room CP implementation of npm escalade) (`v1.0.0`, `MIT`)

## Import

```cp
import { AncestorWalker, joinCandidate } from "escalade";
```

## Core Types & Constants

```cp
class AncestorWalker { buf: [512]u8, len: usize, done: bool } // methods: new(...) +3
```

## API Signatures

```cp
fn joinCandidate(dir: string, name: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { joinCandidate } from "escalade";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = joinCandidate("example", "example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"escalade"`; install via `cap add escalade`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
