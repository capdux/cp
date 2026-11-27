# https-proxy-agent — AI Agent Quick Ref

Zero-heap HTTP/HTTPS proxy URL parser and RFC 9110 CONNECT tunnel request builder (covers http-proxy-agent) (`v7.0.6`, `MIT`)

## Import

```cp
import {
    ProxyEndpoint,
    hpaParsePort,
    parseProxyUrl,
    hpaWriteBase64,
    buildConnectRequest,
} from "https-proxy-agent";
```

## Core Types & Constants

```cp
class ProxyEndpoint { scheme: string, host: string, port: u16, username: string, /* +4 fields */ }
```

## API Signatures

```cp
fn hpaParsePort(s: string): u16?;
fn parseProxyUrl(url: string): ProxyEndpoint;
fn hpaWriteBase64(input: &[]u8, out: []u8, pos: usize): usize;
fn buildConnectRequest(target_host: string, target_port: u16, proxy: ProxyEndpoint, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseProxyUrl } from "https-proxy-agent";

pub fn main() {
    let result = parseProxyUrl("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"https-proxy-agent"`; install via `cap add https-proxy-agent`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
