[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/cache-control

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cache-control-parser` ([https://github.com/etienne-martin/cache-control-parser](https://github.com/etienne-martin/cache-control-parser))（原库名 `cache-control-parser` → CP 包名 `@cap/cache-control`）

Zero-allocation RFC 9111 HTTP `Cache-Control` directive parser, serializer, and shared/private TTL calculator for CP — clean-room MIT parity with npm `cache-control-parser` and Rust `http-cache-semantics`.

纯 CP 零堆分配 RFC 9111 HTTP `Cache-Control` 指令解析、序列化与共享/私有缓存有效 TTL 计算引擎。

## Install / 安装

```bash
cap add @cap/cache-control
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `CacheControl.parse` | `(header: []const u8) -> CacheControl` | Case-insensitive directive parser (supports quoted numbers & ignores extensions) |
| `isCacheableShared` | `() -> bool` | Whether response is eligible for shared CDN/proxy caching |
| `effectiveTtl` | `(is_shared: bool) -> i64` | Effective TTL in seconds (`0` for `no-store`/`no-cache`, `-1` if unspecified) |
| `format` | `(dst: []u8) -> i64` | Canonical `, `-separated serialization (`-1` if buffer too small) |

## Usage / 使用示例

```cp
import { CacheControl } from "@cap/cache-control";

const cc = CacheControl.parse("public, max-age=60, s-maxage=3600, immutable");
const cdnTtl = cc.effectiveTtl(true);  // 3600
const brwTtl = cc.effectiveTtl(false); // 60
```
