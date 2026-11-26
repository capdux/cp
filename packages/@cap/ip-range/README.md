# @cap/ip-range

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ipnet` ([https://github.com/krisprice/ipnet](https://github.com/krisprice/ipnet)) / `npm:ip-range-check` ([https://github.com/danielcompton/ip-range-check](https://github.com/danielcompton/ip-range-check))（原库名 `ipnet` → CP 包名 `@cap/ip-range`）

Zero-heap IPv4 address parser/formatter, CIDR subnet containment & overlap engine, and RFC special-use address classifier in pure CP.

## Features

- `parseIpv4(s)` & `formatIpv4(ip, out)`: strict dotted-decimal parsing rejecting leading zeros (`010.0.0.1`)
- `parseCidr(s)`: parses `a.b.c.d/0..32` and computes `mask`, `network`, and `broadcast`
- `cidrContains(cidr, ip)` & `cidrOverlaps(a, b)`: O(1) range containment & overlap checks
- `isPrivateIpv4`, `isLoopbackIpv4`, `isLinkLocalIpv4` (`169.254.0.0/16`), and `isPublicIpv4`

## Usage

```cp
import { parseCidr, parseIpv4, cidrContains, isPublicIpv4 } from "@cap/ip-range";

const net = parseCidr("10.0.0.0/8");
const ip = parseIpv4("10.20.30.40");
const inside = cidrContains(net, ip.ip); // true
const pub_ip = isPublicIpv4(ip.ip);      // false
```
