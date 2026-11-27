[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @jridgewell/sourcemap-codec

Zero-heap Source Map v3 Base64 VLQ (Variable-Length Quantity) signed integer sequence encoder and decoder.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@jridgewell/sourcemap-codec` ([https://github.com/jridgewell/sourcemap-codec](https://github.com/jridgewell/sourcemap-codec)) / `crates.io:vlq` ([https://github.com/fitzgen/vlq](https://github.com/fitzgen/vlq)) / `npm:vlq` ([https://github.com/Rich-Harris/vlq](https://github.com/Rich-Harris/vlq))（原库名 `@jridgewell/sourcemap-codec` → CP 包名 `@jridgewell/sourcemap-codec`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `@jridgewell/sourcemap-codec` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @jridgewell/sourcemap-codec
```

```cp
import {
    DecodeResult,
    b64Char,
    b64Val,
    encodeOne,
    decodeOne,
    encodeSlice,
    decodeSlice,
} from "@jridgewell/sourcemap-codec";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DecodeResult` | `class` | 字段: `value: i64, consumed: usize, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `b64Char` | `(v: u8): u8` | 调用 `b64Char` 并返回 `u8`。 |
| `b64Val` | `(c: u8): i32` | 调用 `b64Val` 并返回 `i32`。 |
| `encodeOne` | `(val: i64, out: []u8): usize` | 通过 `encodeOne` 编码或格式化输出，返回 `usize`。 |
| `decodeOne` | `(input: string, start: usize): DecodeResult` | 通过 `decodeOne` 解析输入数据，返回 `DecodeResult`。 |
| `encodeSlice` | `(values: &[]i64, out: []u8): usize` | 通过 `encodeSlice` 编码或格式化输出，返回 `usize`。 |
| `decodeSlice` | `(input: string, out: []i64): i64` | 通过 `decodeSlice` 解析输入数据，返回 `i64`。 |

## 最小调用示例

```cp
import { decodeOne } from "@jridgewell/sourcemap-codec";

pub fn main() {
    let result = decodeOne("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
