# `cap:etc_services` — Linux `/etc/services` Zero-Allocation Parser

Zero-allocation, pure-CP streaming parser and bidirectional service-name / port / protocol lookup library for Linux `/etc/services` files (`service-name port/protocol [aliases...] [# comment]`).

## Exports

### Types

- `ServiceEntry`
  - Fields: `name: []const u8`, `port: u16`, `proto: []const u8`, `aliases: []const u8`
  - `ServiceEntry.parseLine(raw: []const u8): ServiceEntry?` — Parses a single `/etc/services` line, stripping trailing `\r` and `#` inline comments, and rejecting port `0`, ports `> 65535`, missing or malformed `/protocol` tokens.
  - `hasNameOrAlias(query: []const u8): bool` — Exact match against the primary service name or any whitespace-separated alias (no substring false positives).
  - `aliasCount(): usize` — Counts whitespace-separated aliases.
- `EtcServicesIter`
  - `EtcServicesIter.init(content: []const u8): EtcServicesIter`
  - `next(*_): ServiceEntry?` — Streams valid `ServiceEntry` records across `\n` and `\r\n` lines, skipping blank lines, `#` comments, and malformed entries.

### Lookup Functions

- `findServicePort(content: []const u8, name: []const u8, proto: []const u8): u16?` — Finds port number by primary name or alias and case-insensitive protocol (`"tcp"`, `"TCP"`, `"udp"`, etc.).
- `findServiceByPort(content: []const u8, port: u16, proto: []const u8): ServiceEntry?` — Reverse lookup of a `ServiceEntry` by port number and case-insensitive protocol.

## Example

```typescript
import {
    EtcServicesIter,
    findServicePort,
    findServiceByPort,
} from "cap:etc_services";

const content =
    "# Network services\n" ++
    "http            80/tcp          www www-http    # WorldWideWeb HTTP\n" ++
    "https           443/tcp                         # HTTP over TLS\n" ++
    "domain          53/udp          dns             # Domain Name Server\n";

const http_port = findServicePort(content, "www", "TCP"); // 80
const dns_entry = findServiceByPort(content, 53, "udp");  // name == "domain"
```
