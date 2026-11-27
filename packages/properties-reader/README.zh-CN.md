[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# properties-reader

Zero-heap Java / Spring Boot / i18n .properties parser and formatter with =/:/space separators, backslash line continuations, comments, and \uXXXX Unicode escapes.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:properties-reader` ([https://github.com/steveukx/properties](https://github.com/steveukx/properties))（原库名 `properties-reader` → CP 包名 `properties-reader`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `properties-reader` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add properties-reader
```

```cp
import {
    PropEntry,
    EscDecodeResult,
    Properties,
    isLineWs,
    decodeEscapeInto,
    pushByte,
    pushEscaped,
    formatEntry,
} from "properties-reader";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PropEntry` | `class` | 字段: `key_start: usize, key_len: usize, val_start: usize, val_len: usize` |
| `EscDecodeResult` | `class` | 字段: `next_pos: usize, written: usize` |
| `Properties` | `class` | 字段: `entries: [128]PropEntry, count: usize, val_buf: [2048]u8, buf_used: usize` · 方法: `parse, len, findIndex, has, get` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isLineWs` | `(c: u8): bool` | 执行 `isLineWs` 条件判定，返回 `bool`。 |
| `decodeEscapeInto` | `(src: &[]u8, esc_pos: usize, dst: []u8, dst_pos: usize): EscDecodeResult` | 通过 `decodeEscapeInto` 解析输入数据，返回 `EscDecodeResult`。 |
| `pushByte` | `(dst: []u8, pos: usize, b: u8): i64` | 写入或更新状态数据。 |
| `pushEscaped` | `(dst: []u8, pos: usize, ch: u8): i64` | 写入或更新状态数据。 |
| `formatEntry` | `(key: &[]u8, value: &[]u8, dst: []u8): i64` | 通过 `formatEntry` 编码或格式化输出，返回 `i64`。 |

## 最小调用示例

```cp
import { isLineWs } from "properties-reader";

pub fn main() {
    let result = isLineWs(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
