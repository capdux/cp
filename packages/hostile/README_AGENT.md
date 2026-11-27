# hostile — AI Agent Quick Ref

Zero-allocation POSIX/Windows /etc/hosts parser, case-insensitive forward/family/reverse lookup, and canonical formatter (hostile / Rust hostsfile parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    HostEntry,
    HostsTable,
    isBlank,
    parseHosts,
    lookupIp,
    lookupIpByFamily,
    reverseLookup,
    pushByte,
} from "hostile";
```

## Core Types & Constants

```cp
class HostEntry { ip: string, hostname: string, is_ipv6: bool } // methods: empty(...)
class HostsTable { entries: [MAX_HOST_ENTRIES]HostEntry, count: usize } // methods: empty(...) +1
```

## API Signatures

```cp
fn isBlank(c: u8): bool;
fn parseHosts(content: string): HostsTable;
fn lookupIp(table: HostsTable, hostname: string): string?;
fn lookupIpByFamily(table: HostsTable, hostname: string, ipv6: bool): string?;
fn reverseLookup(table: HostsTable, ip: string): string?;
fn pushByte(out: []u8, pos: usize, b: u8): usize;
fn formatHosts(table: HostsTable, out: []u8): string;
```

## Minimal Usage

```cp
import { parseHosts } from "hostile";

pub fn main() {
    let result = parseHosts("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"hostile"`; install via `cap add hostile`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
