# hostile@1.0.0 (Agent Quickref)

> Zero-allocation POSIX/Windows /etc/hosts parser, case-insensitive forward/family/reverse lookup, and canonical formatter (hostile / Rust hostsfile parity) for CP | License: MIT
> Import: `import { ... } from "hostile";` | Full types: `src/index.d.cp`

## Functions
- `export fn isBlank(c: u8): bool;`
- `export fn parseHosts(content: string): HostsTable;`
- `export fn lookupIp(table: HostsTable, hostname: string): string?;`
- `export fn lookupIpByFamily(table: HostsTable, hostname: string, ipv6: bool): string?;`
- `export fn reverseLookup(table: HostsTable, ip: string): string?;`
- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn formatHosts(table: HostsTable, out: []u8): string;`
