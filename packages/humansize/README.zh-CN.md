[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# humansize

Zero-heap human-readable byte size formatter (Decimal, Binary, Windows) in pure CP (modeled after Rust humansize v2)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:humansize` ([https://github.com/LeopoldArkham/humansize](https://github.com/LeopoldArkham/humansize))（原库名 `humansize` → CP 包名 `humansize`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `humansize` |
| 版本 | `1.0.0` |
| 许可证 | `MIT OR Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add humansize
```

```cp
import {
    BaseUnit,
    FormatSizeOptions,
    hszWriteSlice,
    hszWriteU64,
    hszWriteFixedFrac,
    hszPow10,
    hszUnitStr,
    formatSize,
} from "humansize";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `BaseUnit` | `Decimal, Binary, Windows` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FormatSizeOptions` | `class` | 字段: `base: BaseUnit, decimal_places: u8, space_after_value: bool, long_units: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hszWriteSlice` | `(out: []u8, pos: usize, s: string): usize` | 调用 `hszWriteSlice` 并返回 `usize`。 |
| `hszWriteU64` | `(out: []u8, pos: usize, val: u64): usize` | 调用 `hszWriteU64` 并返回 `usize`。 |
| `hszWriteFixedFrac` | `(out: []u8, pos: usize, frac: u64, places: u8): usize` | 调用 `hszWriteFixedFrac` 并返回 `usize`。 |
| `hszPow10` | `(places: u8): u64` | 调用 `hszPow10` 并返回 `u64`。 |
| `hszUnitStr` | `(base: BaseUnit, idx: usize, long_units: bool, is_one: bool): string` | 调用 `hszUnitStr` 并返回 `string`。 |
| `formatSize` | `(bytes: u64, opts: FormatSizeOptions, out: []u8): usize` | 通过 `formatSize` 编码或格式化输出，返回 `usize`。 |
| `formatDecimal` | `(bytes: u64, out: []u8): usize` | 通过 `formatDecimal` 编码或格式化输出，返回 `usize`。 |
| `formatBinary` | `(bytes: u64, out: []u8): usize` | 通过 `formatBinary` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { hszWriteSlice } from "humansize";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = hszWriteSlice(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
