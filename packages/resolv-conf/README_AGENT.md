# resolv-conf — AI Agent Quick Ref

Zero-heap POSIX /etc/resolv.conf DNS resolver configuration parser (`v0.7.0`, `MIT`)

## Import

```cp
import { ResolvConfig, defaultResolvConfig, parseResolvConf } from "resolv-conf";
```

## Core Types & Constants

```cp
class ResolvConfig { nameservers: [8]string, nameserver_count: usize, search_domains: [8]string, search_count: usize, /* +8 fields */ }
```

## API Signatures

```cp
fn defaultResolvConfig(): ResolvConfig;
fn parseResolvConf(content: string): ResolvConfig;
```

## Minimal Usage

```cp
import { parseResolvConf } from "resolv-conf";

pub fn main() {
    let result = parseResolvConf("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"resolv-conf"`; install via `cap add resolv-conf`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
