# proxy-from-env — AI Agent Quick Ref

Zero-allocation HTTP_PROXY / HTTPS_PROXY / ALL_PROXY / NO_PROXY URL routing and wildcard bypass matcher (clean-room MIT implementation of npm proxy-from-env, #303 / 169M wk) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ProxyEnv,
    PfeParsedUrl,
    pfeEndsWithIgnoreCase,
    pfeParseUrl,
    shouldBypassProxy,
    getProxyForUrl,
} from "proxy-from-env";
```

## Core Types & Constants

```cp
class ProxyEnv { http_proxy: string, https_proxy: string, all_proxy: string, no_proxy: string } // methods: empty(...)
class PfeParsedUrl { scheme: string, host: string, port: u16 }
```

## API Signatures

```cp
fn pfeEndsWithIgnoreCase(s: string, suffix: string): bool;
fn pfeParseUrl(url: string): PfeParsedUrl;
fn shouldBypassProxy(host: string, port: u16, no_proxy: string): bool;
fn getProxyForUrl(target_url: string, env: ProxyEnv): string;
```

## Minimal Usage

```cp
import { getProxyForUrl } from "proxy-from-env";

pub fn main() {
    let result = getProxyForUrl("example", /* env: ProxyEnv */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"proxy-from-env"`; install via `cap add proxy-from-env`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
