# `cap:hosts_file` — `/etc/hosts` Parser & Lookup

Zero-allocation `/etc/hosts` file parser and resolver lookup supporting UTF-8 BOM, CRLF/LF line endings, `#` comments, IPv4/IPv6 separation, multiple aliases per line, and reverse IP lookup.

## Exports

- `HostsLine` — Parsed line (`ip`, `canonical_host`, `aliases_rest`, `is_ipv6`) with `matchesHost(hostname): bool` and `hostCount(): usize`.
- `HostsIter` — Zero-copy streaming iterator (`init(content)`, `next(*_): HostsLine?`).
- `lookupHost(content, hostname): ([]const u8)?` — First matching IP (IPv4 or IPv6).
- `lookupHostV4(content, hostname): ([]const u8)?` — First matching IPv4 address.
- `lookupHostV6(content, hostname): ([]const u8)?` — First matching IPv6 address.
- `reverseLookup(content, ip): ([]const u8)?` — Canonical hostname for an IP string.
