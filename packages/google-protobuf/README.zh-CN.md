[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# google-protobuf

Zero-allocation Protocol Buffers Wire Format encoder/decoder reusing varint (ULEB128 + ZigZag) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:google-protobuf` ([https://github.com/protocolbuffers/protobuf-javascript](https://github.com/protocolbuffers/protobuf-javascript)) / `crates.io:prost` ([https://github.com/tokio-rs/prost](https://github.com/tokio-rs/prost)) / `npm:pbf` ([https://github.com/mapbox/pbf](https://github.com/mapbox/pbf))（原库名 `google-protobuf` → CP 包名 `google-protobuf`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `google-protobuf` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add google-protobuf
```

```cp
import {
    FieldTag,
    BytesField,
    Sint64Decode,
    isValidWireType,
    encodeTag,
    decodeTag,
    encodeVarintField,
    encodeSint64Field,
} from "google-protobuf";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FieldTag` | `class` | 字段: `field_number: u32, wire_type: u8, bytes_read: usize, ok: bool` |
| `BytesField` | `class` | 字段: `start: usize, len: usize, next_pos: usize, ok: bool` |
| `Sint64Decode` | `class` | 字段: `value: i64, bytes_read: usize, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isValidWireType` | `(wt: u8): bool` | 执行 `isValidWireType` 条件判定，返回 `bool`。 |
| `encodeTag` | `(field_number: u32, wire_type: u8, out: []u8): usize` | 通过 `encodeTag` 编码或格式化输出，返回 `usize`。 |
| `decodeTag` | `(buf: &[]u8): FieldTag` | 通过 `decodeTag` 解析输入数据，返回 `FieldTag`。 |
| `encodeVarintField` | `(field_number: u32, val: u64, out: []u8): usize` | 通过 `encodeVarintField` 编码或格式化输出，返回 `usize`。 |
| `encodeSint64Field` | `(field_number: u32, val: i64, out: []u8): usize` | 通过 `encodeSint64Field` 编码或格式化输出，返回 `usize`。 |
| `decodeVarintValue` | `(buf: &[]u8, pos: usize): U64Decode` | 通过 `decodeVarintValue` 解析输入数据，返回 `U64Decode`。 |
| `decodeSint64Value` | `(buf: &[]u8, pos: usize): Sint64Decode` | 通过 `decodeSint64Value` 解析输入数据，返回 `Sint64Decode`。 |
| `encodeFixed32Field` | `(field_number: u32, val: u32, out: []u8): usize` | 通过 `encodeFixed32Field` 编码或格式化输出，返回 `usize`。 |
| `decodeFixed32` | `(buf: &[]u8, pos: usize): u32` | 通过 `decodeFixed32` 解析输入数据，返回 `u32`。 |
| `encodeFixed64Field` | `(field_number: u32, val: u64, out: []u8): usize` | 通过 `encodeFixed64Field` 编码或格式化输出，返回 `usize`。 |
| `decodeFixed64` | `(buf: &[]u8, pos: usize): u64` | 通过 `decodeFixed64` 解析输入数据，返回 `u64`。 |
| `encodeBytesField` | `(field_number: u32, payload: string, out: []u8): usize` | 通过 `encodeBytesField` 编码或格式化输出，返回 `usize`。 |
| `decodeBytesField` | `(buf: &[]u8, pos: usize): BytesField` | 通过 `decodeBytesField` 解析输入数据，返回 `BytesField`。 |
| `skipField` | `(buf: &[]u8, pos: usize, wire_type: u8): i64` | 调用 `skipField` 并返回 `i64`。 |

## 最小调用示例

```cp
import { encodeBytesField } from "google-protobuf";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeBytesField(0, "example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
