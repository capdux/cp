# @cap/proxy-addr

Pure CP, zero-allocation reverse-proxy client IP resolver and trusted proxy chain validator (parity with npm `proxy-addr`).

## Features

- **Built-in Trust Presets**:
  - `isLoopback`: `127.0.0.0/8`, `::1`, `::ffff:127.0.0.1`
  - `isLinkLocal`: `169.254.0.0/16`, `fe80::/10`
  - `isUniqueLocal`: `10.0.0.0/8`, `172.16.0.0/12`, `192.168.0.0/16`, `fc00::/7`
  - `isTrustedBuiltin(ip, preset)`: `0=none`, `1=loopback`, `2=linklocal`, `3=uniquelocal`, `4=all_private`
- **Zero-Copy XFF Parsing**: `parseXffAddresses` returns nearest-to-farthest `IpSpan` entries into `X-Forwarded-For`
- **Reverse Hop Resolution**: `resolveClientIp` walks backward from `remote_addr` through trusted proxies (with optional `max_hops` cap) to prevent `X-Forwarded-For` spoofing

## Usage

```cp
import { resolveClientIp, TRUST_ALL_PRIVATE } from "@cap/proxy-addr";

let out: [64]u8 = undefined;
let n = resolveClientIp("10.0.0.1", "203.0.113.5, 172.16.0.2", TRUST_ALL_PRIVATE, 0, out[0..64]);
// out[0..n] == "203.0.113.5"
```

## License

MIT
