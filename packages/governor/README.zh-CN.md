[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# governor

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation IETF HTTP RateLimit headers (RateLimit-Policy / RateLimit / X-RateLimit-*) parser, formatter, and reset delay calculator for CP.

## 安装与导入

```bash
cap add governor
```

```cp
import { parseUintHeader } from "governor";
```

## 核心导出 API

- `export fn parseUintHeader(value: string): i64;`
- `export fn parsePolicy(header: string): RateLimitPolicy;`
- `export fn formatPolicy(quota: u64, window_secs: u64, out: []u8): usize;`
- `export fn parseStructuredRateLimit(header: string): RateLimitState;`
- `export fn formatStructuredRateLimit(state: RateLimitState, out: []u8): usize;`
- `export fn isExhausted(state: RateLimitState): bool;`
- `export fn computeResetDelaySecs(reset_val: u64, now_unix_secs: u64): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`governor@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`governor@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`express-rate-limit@1.0.0`)，许可证：MIT
