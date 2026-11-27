[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`redis`](https://github.com/redis-rs/redis-rs)（`BSD-3-Clause`）（原库名 `redis`，CP 包名 `@cap/redis-url`）。

# @cap/redis-url

Zero-allocation Redis / Valkey / Sentinel connection URL parser (`redis://`, `rediss://`, `redis+unix://`, `unix://`) and password-redacting formatter for CP.

## Features

- **Zero heap allocation**: zero-copy `RedisConfig` slicing and caller-supplied `[]u8` formatting buffer.
- **Full connection syntax**: TLS (`rediss://`), Redis 6+ ACL (`user:pass@`), password-only (`:pass@`), bracketed IPv6 (`[::1]:6380`), Unix domain sockets (`redis+unix://`, `unix://`), `/db` path, and `?db=N` query override.
- **Safe log redaction**: `formatRedisUrl(cfg, true, out)` replaces credentials with `***` to prevent secret leakage.

## Usage

```cp
import { parseRedisUrl, formatRedisUrl } from "@cap/redis-url";

const cfg = parseRedisUrl("rediss://alice:s3cr3t@cache.internal:6380/3");
let buf: [128]u8 = undefined;
const n = formatRedisUrl(cfg, true, buf[0..128]);
// "rediss://alice:***@cache.internal:6380/3"
```
