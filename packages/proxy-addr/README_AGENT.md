# proxy-addr@1.0.0 (Agent Quickref)

> Pure CP zero-allocation reverse-proxy client IP resolver and trusted proxy chain validator (npm proxy-addr parity) | License: MIT
> Import: `import { ... } from "proxy-addr";` | Full types: `src/index.d.cp`

## Functions
- `export fn isLoopback(ip: string): bool;`
- `export fn isLinkLocal(ip: string): bool;`
- `export fn isUniqueLocal(ip: string): bool;`
- `export fn isTrustedBuiltin(ip: string, preset: u8): bool;`
- `export fn parseXffAddresses(remote_addr: string, xff_header: string, out: []IpSpan): usize;`
- `export fn resolveClientIp(remote_addr: string, xff_header: string, trust_preset: u8, max_hops: usize, out: []u8): usize;`
