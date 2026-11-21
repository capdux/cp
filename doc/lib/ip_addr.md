# `cap:ip_addr` — RFC 5952 IPv4 & IPv6 Address Parser & Canonical Formatter

Zero-allocation IPv4 & IPv6 address parser, RFC 5952 canonical zero-compression formatter, IPv4-mapped IPv6 conversion, address classification, and CIDR subnet matching.

## Exports

- `Ipv4Addr` — `fromOctets`, `toU32`, `fromU32`, `parse` (strict dotted-quad, rejects leading zeros), `isUnspecified`, `isLoopback`, `isPrivate`, `isLinkLocal`, `isMulticast`, `isBroadcast`, `inSubnet`, `format`, `toIpv6Mapped`.
- `Ipv6Addr` — `parse` (RFC 4291 with `::` compression and embedded IPv4 tail), `isUnspecified`, `isLoopback`, `isMulticast`, `isLinkLocal`, `isUniqueLocal`, `toIpv4Mapped`, `inSubnet`, `format` (RFC 5952 canonical shortest lowercase form).
- `IpVersion` (`v4` / `v6`) & `IpAddr` — unified `parse`, `isLoopback`, `format`.
