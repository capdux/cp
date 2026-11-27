# forwarded@1.0.0 (Agent Quickref)

> Zero-allocation RFC 7239 Forwarded & X-Forwarded-For proxy chain parser with anti-spoofing trusted-hop client IP extraction and private IP detection (forwarded / proxy-addr parity) for CP | License: MIT
> Import: `import { ... } from "forwarded";` | Full types: `src/index.d.cp`

## Functions
- `export fn unquote(raw: string): string;`
- `export fn stripAddr(raw: string): string;`
- `export fn parseForwardedElement(elem: string): ForwardedHop;`
- `export fn parseForwarded(header: string): ForwardedChain;`
- `export fn parseXForwardedFor(xff: string, remote_addr: string): ForwardedChain;`
- `export fn extractClientIp(xff: string, remote_addr: string, trusted_hops: usize): string;`
- `export fn isPrivateIpv4(s: string): bool;`
- `export fn isPrivateIp(ip: string): bool;`
