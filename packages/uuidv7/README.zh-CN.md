[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# uuidv7

Zero-allocation RFC 9562 UUIDv7 constructor, 36-byte formatter, parser, timestamp extractor, and comparator in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:uuidv7` ([https://github.com/LiosK/uuidv7](https://github.com/LiosK/uuidv7)) / `crates.io:uuid7` ([https://github.com/LiosK/uuid7-rs](https://github.com/LiosK/uuid7-rs)) / `crates.io:uuid` ([https://github.com/uuid-rs/uuid](https://github.com/uuid-rs/uuid))（原库名 `uuidv7` → CP 包名 `uuidv7`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `uuidv7` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add uuidv7
```

```cp
import {
    UuidV7,
    fromParts,
    timestampMs,
    version,
    isRfcVariant,
    formatHyphenated,
    parse,
    isValidUuidV7,
} from "uuidv7";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `UuidV7` | `class` | 字段: `bytes: [16]u8` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `fromParts` | `(timestamp_ms: u64, rand_bytes: [10]u8): UuidV7` | 调用 `fromParts` 并返回 `UuidV7`。 |
| `timestampMs` | `(u: UuidV7): u64` | 调用 `timestampMs` 并返回 `u64`。 |
| `version` | `(u: UuidV7): u8` | 调用 `version` 并返回 `u8`。 |
| `isRfcVariant` | `(u: UuidV7): bool` | 执行 `isRfcVariant` 条件判定，返回 `bool`。 |
| `formatHyphenated` | `(u: UuidV7, out: []u8): usize` | 通过 `formatHyphenated` 编码或格式化输出，返回 `usize`。 |
| `parse` | `(s: string, out: &mut UuidV7): bool` | 通过 `parse` 解析输入数据，返回 `bool`。 |
| `isValidUuidV7` | `(s: string): bool` | 执行 `isValidUuidV7` 条件判定，返回 `bool`。 |
| `compare` | `(a: UuidV7, b: UuidV7): i32` | 调用 `compare` 并返回 `i32`。 |

## 最小调用示例

```cp
import { parse } from "uuidv7";

pub fn main() {
    let result = parse("example", /* out: &mut UuidV7 */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
