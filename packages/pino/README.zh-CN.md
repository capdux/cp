[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# pino

Zero-heap structured NDJSON logger with numeric levels (10..60), safe JSON escaping, and child fields (clean-room port of npm pino)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pino` ([https://github.com/pinojs/pino](https://github.com/pinojs/pino))（原库名 `pino` → CP 包名 `pino`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `pino` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add pino
```

```cp
import {
    PinoField,
    PinoLogger,
    pinoPush,
    pinoPushJsonStr,
    pinoPushU64,
    pinoPushI64,
    levelLabel,
} from "pino";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PinoField` | `class` | 字段: `key: string, kind: u8, str_val: string, int_val: i64, bool_val: bool` |
| `PinoLogger` | `class` | 字段: `min_level: u8, name: string, pid: u32, hostname: string, bindings: [16]PinoField, binding_count: usize` · 方法: `new, isLevelEnabled, bindStr, bindInt, bindBool, formatLine` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pinoPush` | `(out: []u8, pos: usize, s: string): usize` | 调用 `pinoPush` 并返回 `usize`。 |
| `pinoPushJsonStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `pinoPushJsonStr` 并返回 `usize`。 |
| `pinoPushU64` | `(out: []u8, pos: usize, v: u64): usize` | 调用 `pinoPushU64` 并返回 `usize`。 |
| `pinoPushI64` | `(out: []u8, pos: usize, v: i64): usize` | 调用 `pinoPushI64` 并返回 `usize`。 |
| `levelLabel` | `(level: u8): string` | 调用 `levelLabel` 并返回 `string`。 |

## 最小调用示例

```cp
import { pinoPush } from "pino";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pinoPush(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
