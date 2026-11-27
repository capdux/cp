[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# env_filter

> 版本：`1.0.0` | 许可证：`MIT OR Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RUST_LOG-style hierarchical log filter parser and longest-prefix module matcher (Rust env_filter parity) for CP

## 安装与导入

```bash
cap add env_filter
```

```cp
import { efltLevelRank } from "env_filter";
```

## 核心导出 API

- `export enum LogLevel`
- `export fn efltLevelRank(lvl: LogLevel): u8;`
- `export fn parseLogLevel(s: string): LogLevel?;`
- `export fn efltMatchesTarget(target: string, directive_target: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`env_filter@1.0.0`)，许可证：MIT OR Apache-2.0
