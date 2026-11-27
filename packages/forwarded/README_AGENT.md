# forwarded@1.0.0 (Agent Quickref)

> Zero-allocation RFC 7239 Forwarded & X-Forwarded-For proxy chain parser with anti-spoofing trusted-hop client IP extraction and private IP detection (forwarded / proxy-addr parity) for CP | License: MIT
> Import: `import { ... } from "forwarded";` | Full types: `src/index.d.cp`

## Functions
- `export fn isSpace(c: u8): bool;`
- `export fn isDigit(c: u8): bool;`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn trim(s: string): string;`
- `export fn unquote(raw: string): string;`
- `export fn eqIgnoreCase(a: string, lower_lit: string): bool;`
- `export fn stripAddr(raw: string): string;`
- `export fn parseForwardedElement(elem: string): ForwardedHop;`
- `export fn parseForwarded(header: string): ForwardedChain;`
- `export fn parseXForwardedFor(xff: string, remote_addr: string): ForwardedChain;`
- `export fn extractClientIp(xff: string, remote_addr: string, trusted_hops: usize): string;`
- `export fn hexVal(c: u8): i8;`
- `export fn isPrivateIpv4(s: string): bool;`
- `export fn isPrivateIp(ip: string): bool;`
