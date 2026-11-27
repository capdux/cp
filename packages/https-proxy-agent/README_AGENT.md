# https-proxy-agent@7.0.6 (Agent Quickref)

> Zero-heap HTTP/HTTPS proxy URL parser and RFC 9110 CONNECT tunnel request builder (covers http-proxy-agent) | License: MIT
> Import: `import { ... } from "https-proxy-agent";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseProxyUrl(url: string): ProxyEndpoint;`
- `export fn buildConnectRequest(target_host: string, target_port: u16, proxy: ProxyEndpoint, out: []u8): usize;`
