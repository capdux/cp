[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @sapphire/snowflake

Zero-heap 64-bit Twitter/Discord Snowflake distributed ID generator, decomposer, clock-rollback guard, and decimal formatter.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@sapphire/snowflake` ([https://github.com/sapphiredev/utilities](https://github.com/sapphiredev/utilities)) / `crates.io:sonyflake` ([https://github.com/sony/sonyflake](https://github.com/sony/sonyflake))（原库名 `@sapphire/snowflake` → CP 包名 `@sapphire/snowflake`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `@sapphire/snowflake` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @sapphire/snowflake
```

```cp
import {
    SnowflakeParts,
    SnowflakeGenerator,
    compose,
    decompose,
    initGenerator,
    nextId,
    formatId,
    parseId,
} from "@sapphire/snowflake";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SnowflakeParts` | `class` | 字段: `timestamp_ms: u64, datacenter_id: u8, worker_id: u8, sequence: u16` |
| `SnowflakeGenerator` | `class` | 字段: `epoch_ms: u64, datacenter_id: u8, worker_id: u8, last_ts_ms: u64, sequence: u16` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `compose` | `(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u8, worker_id: u8, sequence: u16): u64` | 调用 `compose` 并返回 `u64`。 |
| `decompose` | `(id: u64, epoch_ms: u64): SnowflakeParts` | 调用 `decompose` 并返回 `SnowflakeParts`。 |
| `initGenerator` | `(epoch_ms: u64, datacenter_id: u8, worker_id: u8): SnowflakeGenerator` | 创建或初始化实例，返回 `SnowflakeGenerator`。 |
| `nextId` | `(gen: &mut SnowflakeGenerator, now_ms: u64): i64` | 调用 `nextId` 并返回 `i64`。 |
| `formatId` | `(id: u64, out: []u8): usize` | 通过 `formatId` 编码或格式化输出，返回 `usize`。 |
| `parseId` | `(s: string): i64` | 通过 `parseId` 解析输入数据，返回 `i64`。 |

## 最小调用示例

```cp
import { parseId } from "@sapphire/snowflake";

pub fn main() {
    let result = parseId("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
