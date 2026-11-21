# `cap:socket_addr` — `host:port` / `[ipv6]:port` Socket Address Parser & Formatter

Zero-allocation socket address parser supporting strict remote endpoints (`host:port`, `[ipv6]:port`) and server bind addresses (`:8080`, `127.0.0.1:0`), with IPv4/IPv6/hostname classification and canonical formatting.

## Exports

- `HostKind` (`ipv4` | `ipv6` | `hostname`)
- `SocketAddr` — `parse(raw)`, `parseBind(raw)`, `parseWithOptions(raw, allow_port_zero, allow_empty_host)`, `format(out)`.
