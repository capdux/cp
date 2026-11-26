# `@cap/forwarded-http`

Zero-heap RFC 7239 `Forwarded` & `X-Forwarded-For` proxy chain parser, trusted-hop client IP extractor, and loopback IP detector in pure CP (clean-room MIT counterpart to npm `forwarded` + `proxy-addr`).

## Features

- **100% Zero Heap Allocation**: Parses proxy chains and RFC 7239 entries directly into caller-provided slices.
- **Cross-Package Reuse**: Reuses `eqAscii` from `@cap/unicase` for case-insensitive RFC 7239 parameter keys (`for`, `by`, `host`, `proto`).
- **RFC 7239 `Forwarded` Parser**: Handles multiple comma-separated hops, quoted values, and bracketed IPv6 addresses with ports (`for="[2001:db8:cafe::17]:4711"`).
- **Proxy-Aware Client IP Extraction**: `parseXForwardedFor` returns `[remote_addr, closest_proxy, ..., client_ip]`; `extractClientIp` respects `trust_hops` to prevent header spoofing.
- **Loopback Detection**: `isLoopbackIp` recognizes `127.0.0.0/8`, `::1`, `0:0:0:0:0:0:0:1`, `::ffff:127.x.y.z`, and optional `:port` / `[ipv6]:port` forms.

## Dependencies

- `@cap/unicase` (`1.0.0`)

## Quick Start

```cp
import { parseXForwardedFor, extractClientIp, parseForwardedHeader, ForwardedEntry, isLoopbackIp } from "@cap/forwarded-http";

let addrs: [8]string = undefined;
const n = parseXForwardedFor("127.0.0.1", "203.0.113.5, 10.0.0.2", &addrs);
// addrs[0] == "127.0.0.1", addrs[1] == "10.0.0.2", addrs[2] == "203.0.113.5"

const client = extractClientIp("127.0.0.1", "203.0.113.5, 10.0.0.2", 2);
// "203.0.113.5"
```

## API

- `ForwardedEntry { for_val: string, by_val: string, host: string, proto: string }`
- `parseXForwardedFor(remote_addr: string, xff_header: string, out_addrs: []string): usize`
- `extractClientIp(remote_addr: string, xff_header: string, trust_hops: usize): string`
- `parseForwardedHeader(header: string, out_entries: []ForwardedEntry): usize`
- `isLoopbackIp(ip: string): bool`

## License

MIT
