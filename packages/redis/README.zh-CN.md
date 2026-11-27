[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# redis

Zero-allocation Redis / Valkey / Sentinel connection URL parser (redis://, rediss://, unix://) and password-redacting formatter for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`redis`](https://github.com/redis-rs/redis-rs)（`BSD-3-Clause`）（原库名 `redis`，CP 包名 `redis`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `redis` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add redis
```

```cp
import {
    RedisConfig,
    ruStartsWithIgnoreCase,
    ruParseStrictU16,
    ruWrite,
    ruWriteU16,
    parseRedisUrl,
    formatRedisUrl,
} from "redis";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RedisConfig` | `class` | 字段: `tls: bool, is_unix: bool, host: string, port: u16, username: string, password: string, +3 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ruStartsWithIgnoreCase` | `(s: string, prefix: string): bool` | 调用 `ruStartsWithIgnoreCase` 并返回 `bool`。 |
| `ruParseStrictU16` | `(s: string): u16?` | 调用 `ruParseStrictU16` 并返回 `u16?`。 |
| `ruWrite` | `(out: []u8, pos: &mut usize, src: string): bool` | 调用 `ruWrite` 并返回 `bool`。 |
| `ruWriteU16` | `(out: []u8, pos: &mut usize, val: u16): bool` | 调用 `ruWriteU16` 并返回 `bool`。 |
| `parseRedisUrl` | `(url: string): RedisConfig` | 通过 `parseRedisUrl` 解析输入数据，返回 `RedisConfig`。 |
| `formatRedisUrl` | `(cfg: RedisConfig, redact_password: bool, out: []u8): usize` | 通过 `formatRedisUrl` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseRedisUrl } from "redis";

pub fn main() {
    let result = parseRedisUrl("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
