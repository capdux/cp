[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# amqplib

Zero-allocation AMQP 0-9-1 / RabbitMQ URI parser (amqp/amqps, IPv6, %2F vhost, heartbeat) and password-redacting formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:amqplib` ([https://github.com/amqp-node/amqplib](https://github.com/amqp-node/amqplib)) / `crates.io:lapin` ([https://github.com/amqp-rs/lapin](https://github.com/amqp-rs/lapin))（原库名 `amqplib` → CP 包名 `amqplib`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `amqplib` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add amqplib
```

```cp
import {
    AmqpConfig,
    invalidAmqp,
    amqpIsSpace,
    parseU16Strict,
    parseAmqpUrl,
    pushSlice,
    pushU16,
    formatAmqpUrl,
} from "amqplib";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `AmqpConfig` | `class` | 字段: `tls: bool, host: string, port: u16, username: string, password: string, vhost: string, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `invalidAmqp` | `(): AmqpConfig` | 调用 `invalidAmqp` 并返回 `AmqpConfig`。 |
| `amqpIsSpace` | `(b: u8): bool` | 调用 `amqpIsSpace` 并返回 `bool`。 |
| `parseU16Strict` | `(s: string): i32` | 通过 `parseU16Strict` 解析输入数据，返回 `i32`。 |
| `parseAmqpUrl` | `(url: string): AmqpConfig` | 通过 `parseAmqpUrl` 解析输入数据，返回 `AmqpConfig`。 |
| `pushSlice` | `(dst: []u8, pos: usize, src: string): i64` | 写入或更新状态数据。 |
| `pushU16` | `(dst: []u8, pos: usize, val: u16): i64` | 写入或更新状态数据。 |
| `formatAmqpUrl` | `(cfg: AmqpConfig, redact_password: bool, out: []u8): usize` | 通过 `formatAmqpUrl` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseU16Strict } from "amqplib";

pub fn main() {
    let result = parseU16Strict("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
