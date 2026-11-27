[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# atoi

Zero-heap overflow-checked integer parsing directly from byte slices (base 10, 16, 2..36) matching Rust atoi (262M+ downloads) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:atoi` ([https://github.com/pacman82/atoi-rs](https://github.com/pacman82/atoi-rs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `atoi` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add atoi
```

```cp
import {
    ParseU64Result,
    ParseI64Result,
    digitVal,
    fromRadixU64,
    fromRadix10U64,
    fromRadix16U64,
    fromRadixI64,
    fromRadix10I64,
} from "atoi";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ParseU64Result` | `class` | 字段: `value: u64, used: usize, ok: bool` |
| `ParseI64Result` | `class` | 字段: `value: i64, used: usize, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `digitVal` | `(ch: u8, radix: u32): i32` | 调用 `digitVal` 并返回 `i32`。 |
| `fromRadixU64` | `(bytes: string, radix: u32): ParseU64Result` | 调用 `fromRadixU64` 并返回 `ParseU64Result`。 |
| `fromRadix10U64` | `(bytes: string): ParseU64Result` | 调用 `fromRadix10U64` 并返回 `ParseU64Result`。 |
| `fromRadix16U64` | `(bytes: string): ParseU64Result` | 调用 `fromRadix16U64` 并返回 `ParseU64Result`。 |
| `fromRadixI64` | `(bytes: string, radix: u32): ParseI64Result` | 调用 `fromRadixI64` 并返回 `ParseI64Result`。 |
| `fromRadix10I64` | `(bytes: string): ParseI64Result` | 调用 `fromRadix10I64` 并返回 `ParseI64Result`。 |
| `atoiU64` | `(s: string): ParseU64Result` | 调用 `atoiU64` 并返回 `ParseU64Result`。 |
| `atoiI64` | `(s: string): ParseI64Result` | 调用 `atoiI64` 并返回 `ParseI64Result`。 |

## 最小调用示例

```cp
import { fromRadixU64 } from "atoi";

pub fn main() {
    let result = fromRadixU64("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
