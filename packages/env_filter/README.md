[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# env_filter

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:env_filter` ([https://github.com/rust-cli/env_logger](https://github.com/rust-cli/env_logger))

Zero-allocation `RUST_LOG`-style hierarchical log level filter parser and module prefix matcher for CP — clean-room parity with Rust `env_filter` (257M+ downloads on crates.io).

纯 CP 零堆分配 `RUST_LOG` 风格分级模块日志过滤引擎（最多 32 条指令，按 `target` 长度降序执行最长模块前缀匹配与 `::` 边界对齐），适用于日志框架、可观测性管线与动态调试过滤。

## Install / 安装

```bash
cap add env_filter
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `LogLevel` | `enum: u8 { Off = 0, Error = 1, Warn = 2, Info = 3, Debug = 4, Trace = 5 }` | Standard 6-level log severity |
| `FilterDirective` | `struct { target: string, level: LogLevel }` | Single module-to-level directive |
| `parseLogLevel` | `(s: string) -> LogLevel?` | Case-insensitive level name parser |
| `EnvFilter.parse` | `(spec: string) -> EnvFilter` | Parses comma-separated `RUST_LOG` spec |
| `EnvFilter.empty` | `() -> EnvFilter` | Empty filter (all targets `Off`) |
| `EnvFilter.addDirective` | `(*_, target: string, level: LogLevel) -> bool` | Adds or updates a directive |
| `EnvFilter.enabled` | `(target: string, level: LogLevel) -> bool` | Checks if a target+level is enabled |
| `EnvFilter.maxLevelFor` | `(target: string) -> LogLevel` | Resolves highest enabled level for target |
| `EnvFilter.directiveCount` | `() -> usize` | Number of active directives |

## Usage / 使用示例

```cp
import { EnvFilter, LogLevel } from "env_filter";

const filter = EnvFilter.parse("warn,my_app=info,my_app::net=debug,quiet_mod=off");
const ok1 = filter.enabled("my_app::net::tcp", LogLevel.Debug); // true
const ok2 = filter.enabled("my_app::db", LogLevel.Debug);       // false (max is Info)
const ok3 = filter.enabled("quiet_mod::sub", LogLevel.Error);   // false (Off)
const ok4 = filter.enabled("other_crate", LogLevel.Warn);       // true (fallback Warn)
```
