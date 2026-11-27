# ipaddr.js@1.0.0 (Agent Quickref)

> Zero-allocation IPv4 & IPv6 parser, RFC 5952 formatter, CIDR matcher, IPv4-mapped IPv6 converter, and address range classifier (npm ipaddr.js parity) for CP | License: MIT
> Import: `import { ... } from "ipaddr.js";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseIpv4(s: string): Ipv4Addr;`
- `export fn parseIpv6(s: string): Ipv6Addr;`
- `export fn parseCidrV4(s: string): CidrV4;`
- `export fn parseCidrV6(s: string): CidrV6;`
- `export fn isValidIp(s: string): bool;`
- `export fn isValidIpv4(s: string): bool;`
- `export fn isValidIpv6(s: string): bool;`
- `export fn ipKind(s: string): string;`
