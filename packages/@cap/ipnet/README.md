# `@cap/ipnet`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ipnet` ([https://github.com/krisprice/ipnet](https://github.com/krisprice/ipnet))

Zero-heap pure-CP IPv4 & CIDR subnet calculation engine — clean-room CP equivalent of crates.io `ipnet`.

## Features

- `parseIpv4(s: string): u32?` — strict IPv4 parser (rejects leading zeros, out-of-range octets `> 255`, wrong octet counts)
- `formatIpv4(ip: u32, out: []u8): usize` — zero-heap IPv4 formatter
- `Ipv4Net`:
  - `Ipv4Net.new(addr, prefix_len)` & `Ipv4Net.parse(cidr)`
  - `netmask()`, `hostmask()`, `network()`, `broadcast()`, `trunc()`
  - `containsIp(ip)`, `containsNet(other)`, `supernet()`
  - `hostCount()` (`/32` -> 1, `/31` -> 2, otherwise `2^(32-prefix) - 2`)
  - `formatCidr(out)`
