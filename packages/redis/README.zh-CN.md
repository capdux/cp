[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# redis

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Redis / Valkey / Sentinel connection URL parser (redis://, rediss://, unix://) and password-redacting formatter for CP.

## 安装与导入

```bash
cap add redis
```

```cp
import { parseRedisUrl } from "redis";
```

## 核心导出 API

- `export fn parseRedisUrl(url: string): RedisConfig;`
- `export fn formatRedisUrl(cfg: RedisConfig, redact_password: bool, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`redis@1.0.0`)，许可证：BSD-3-Clause
