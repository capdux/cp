# proxy-from-env@1.0.0 (Agent Quickref)

> Zero-allocation HTTP_PROXY / HTTPS_PROXY / ALL_PROXY / NO_PROXY URL routing and wildcard bypass matcher (clean-room MIT implementation of npm proxy-from-env, #303 / 169M wk) | License: MIT
> Import: `import { ... } from "proxy-from-env";` | Full types: `src/index.d.cp`

## Functions
- `export fn shouldBypassProxy(host: string, port: u16, no_proxy: string): bool;`
- `export fn getProxyForUrl(target_url: string, env: ProxyEnv): string;`
