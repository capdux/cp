[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# zigzag

纯 CP 实现的零堆分配 8/16/32/64 位 ZigZag 有符号整数双射编解码与 ZigZag-Varint 流式编解码器（适用于 Protobuf / Avro / Parquet / CBOR）。

- **包名**：`zigzag`（`v1.0.0`）
- **上游对齐**：洁净室移植自 `crates.io/zigzag`（`MIT`），并扩展了批量切片编解码及 LEB128 ZigZag-Varint 流式接口。

## 安装与导入

```bash
cap add zigzag
```

```typescript
import {
    VarintResult,
    encodeI32, decodeI32,
    encodeI64, decodeI64,
    encodeZigzagVarint, decodeZigzagVarint,
    varintLen,
} from "zigzag";
```

## 核心 API

### 数据类型

| 类型 | 字段 | 说明 |
| --- | --- | --- |
| `VarintResult` | `value: i64`, `consumed: usize`, `valid: bool` | ZigZag-Varint 解码结果：解码值、消费字节数与合法性标志。 |

### 标量 ZigZag 双射

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `encodeI8` | `(n: i8): u8` | 将有符号 `i8` 映射为 `u8`（`0 -> 0, -1 -> 1, 1 -> 2, ...`）。 |
| `decodeU8` / `decodeI8` | `(u: u8): i8` | `u8` 逆映射还原为 `i8`。 |
| `encodeI16` | `(n: i16): u16` | 将有符号 `i16` 映射为 `u16`。 |
| `decodeU16` / `decodeI16` | `(u: u16): i16` | `u16` 逆映射还原为 `i16`。 |
| `encodeI32` | `(n: i32): u32` | 将有符号 `i32` 映射为 `u32`。 |
| `decodeI32` / `decodeU32` | `(u: u32): i32` | `u32` 逆映射还原为 `i32`。 |
| `encodeI64` | `(n: i64): u64` | 将有符号 `i64` 映射为 `u64`。 |
| `decodeI64` / `decodeU64` | `(u: u64): i64` | `u64` 逆映射还原为 `i64`。 |

### 批量切片编解码

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `encodeI8Slice` | `(src: &[]i8, dst: []u8): usize` | 批量编码 `i8` 切片到 `dst`，返回写入元素数（`min(src.len, dst.len)`）。 |
| `decodeU8Slice` | `(src: &[]u8, dst: []i8): usize` | 批量解码 `u8` 切片到 `i8` 切片。 |
| `encodeI16Slice` | `(src: &[]i16, dst: []u16): usize` | 批量编码 `i16` 切片到 `u16` 切片。 |
| `decodeU16Slice` | `(src: &[]u16, dst: []i16): usize` | 批量解码 `u16` 切片到 `i16` 切片。 |
| `encodeI32Slice` | `(src: &[]i32, dst: []u32): usize` | 批量编码 `i32` 切片到 `u32` 切片。 |
| `decodeU32Slice` | `(src: &[]u32, dst: []i32): usize` | 批量解码 `u32` 切片到 `i32` 切片。 |
| `encodeI64Slice` | `(src: &[]i64, dst: []u64): usize` | 批量编码 `i64` 切片到 `u64` 切片。 |
| `decodeU64Slice` | `(src: &[]u64, dst: []i64): usize` | 批量解码 `u64` 切片到 `i64` 切片。 |

### ZigZag-Varint 流式编解码

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `varintLen` | `(n: i64): usize` | 计算精确的 LEB128 ZigZag-Varint 编码字节长度（`1..=10`）。 |
| `encodeZigzagVarint` | `(n: i64, out: []u8): usize` | 将 `n` 编码写入 `out`，返回写入字节数（缓冲区不足返回 `0`）。 |
| `decodeZigzagVarint` | `(data: &[]u8, start: usize): VarintResult` | 从 `start` 偏移解码单个 ZigZag-Varint；使用前需检查 `.valid`。 |
| `encodeZigzagVarintSlice` | `(src: &[]i64, dst: []u8): usize` | 连续编码 `i64` 切片到 `dst`，返回总写入字节数。 |
| `decodeZigzagVarintSlice` | `(src: &[]u8, dst: []i64): usize` | 从 `src` 连续解码多个 ZigZag-Varint 到 `dst`，返回解码元素数。 |

## 使用示例

### 1. 标量双射编解码

```typescript
import { encodeI32, decodeI32, encodeI64, decodeI64 } from "zigzag";

fn main() {
    let u = encodeI32(-1);        // 1
    let s = decodeI32(u);         // -1
    let big = encodeI64(-100000); // 199999
    let orig = decodeI64(big);    // -100000
}
```

### 2. 流式 ZigZag-Varint 编解码

```typescript
import { encodeZigzagVarint, decodeZigzagVarint, varintLen } from "zigzag";

fn main() {
    let buf: [16]u8 = undefined;
    let needed = varintLen(-300);
    let written = encodeZigzagVarint(-300, buf[0..]);
    let res = decodeZigzagVarint(buf[0..written], 0);
    if (res.valid) {
        // res.value == -300, res.consumed == needed
    }
}
```

### 3. 批量切片往返

```typescript
import { encodeZigzagVarintSlice, decodeZigzagVarintSlice } from "zigzag";

fn main() {
    let nums: [4]i64 = [-1, 0, 1, -1000];
    let wire: [40]u8 = undefined;
    let bytes_written = encodeZigzagVarintSlice(nums[0..], wire[0..]);

    let decoded: [4]i64 = undefined;
    let count = decodeZigzagVarintSlice(wire[0..bytes_written], decoded[0..]);
    // count == 4, decoded == [-1, 0, 1, -1000]
}
```

## 与源包差异及零拷贝安全亮点

- **全程零堆分配**：所有批量和 Varint 接口均直接写入调用方提供的栈/目标切片（`dst` / `out`），无需分配器。
- **显式位宽函数**：用单态函数 `encodeI8/I16/I32/I64` 与 `decodeU*/decodeI*` 别名替代 Rust Trait 分发，零开销内联。
- **Varint 边界防护**：`decodeZigzagVarint` 对截断输入与超过 10 字节的溢出编码通过 `VarintResult.valid == false` 安全拒绝。
