[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# zigzag

零堆分配的纯 CP ZigZag 有符号↔无符号整数双射编解码库（支持 `i8`/`i16`/`i32`/`i64` 标量与切片批量编解码）及兼容 Protobuf / Avro / Parquet 的 ZigZag-Varint 流式编解码器。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源平台** | crates.io |
| **上游原包** | `zigzag` |
| **CP 包名** | `zigzag` (`v1.0.0`) |
| **许可证** | MIT |

CP 生态沿用原包名 `zigzag`，无需 `@cap/*` 前缀或改名：

```rust
// Rust (crates.io: zigzag)
use zigzag::ZigZag;
let enc: u64 = ZigZag::encode(-1_i64); // 1
let dec: i64 = ZigZag::decode(enc);    // -1
```

```typescript
// CP（保持原包名 "zigzag"）
import { encodeI64, decodeU64 } from "zigzag";
let enc = encodeI64(-1);  // 1
let dec = decodeU64(enc); // -1
```

## 安装

```bash
cap add zigzag
```

## 快速上手与实战示例

### 1. 标量双射编解码（`i8`、`i16`、`i32`、`i64`）

将绝对值较小的有符号整数（`0, -1, 1, -2, 2, ...`）映射为较小的无符号整数（`0, 1, 2, 3, 4, ...`），大幅提升 Varint/LEB128 对负数的压缩率。

```typescript
import {
    encodeI8, decodeU8,
    encodeI16, decodeU16,
    encodeI32, decodeU32,
    encodeI64, decodeU64,
} from "zigzag";

fn scalarCodec(): void {
    let e8 = encodeI8(-1);       // 1
    let d8 = decodeU8(e8);       // -1（别名：decodeI8）

    let e16 = encodeI16(-2);     // 3
    let d16 = decodeU16(e16);    // -2（别名：decodeI16）

    let e32 = encodeI32(-100);   // 199
    let d32 = decodeU32(e32);    // -100（别名：decodeI32）

    let e64 = encodeI64(-42);    // 83
    let d64 = decodeU64(e64);    // -42（别名：decodeI64）
}
```

### 2. 切片批量编解码

```typescript
import { encodeI32Slice, decodeU32Slice } from "zigzag";

fn sliceCodec(): void {
    let src: [4]i32 = [0, -1, 1, -1000];
    let mut enc: [4]u32 = undefined;
    let mut dec: [4]i32 = undefined;

    let n_enc = encodeI32Slice(src[0..4], enc[0..4]); // 4
    let n_dec = decodeU32Slice(enc[0..4], dec[0..4]); // 4
}
```

### 3. ZigZag-Varint 流式与批量编解码（Protobuf `sint64` / Avro）

```typescript
import {
    varintLen,
    encodeZigzagVarint,
    decodeZigzagVarint,
    encodeZigzagVarintSlice,
    decodeZigzagVarintSlice,
} from "zigzag";

fn varintCodec(): void {
    // 单值编解码
    let needed = varintLen(-150); // 2 字节
    let mut buf: [16]u8 = undefined;
    let written = encodeZigzagVarint(-150, buf[0..16]);
    let res = decodeZigzagVarint(buf[0..written], 0);
    if (res.valid) {
        // res.value == -150, res.consumed == 2
    }

    // i64 序列批量打包 / 解包
    let vals: [4]i64 = [0, -1, 150, -100000];
    let mut packed: [64]u8 = undefined;
    let total_bytes = encodeZigzagVarintSlice(vals[0..4], packed[0..64]);

    let mut unpacked: [4]i64 = undefined;
    let count = decodeZigzagVarintSlice(packed[0..total_bytes], unpacked[0..4]); // 4
}
```

## API 速览

### 标量双射
- `encodeI8` / `decodeU8` / `decodeI8`
- `encodeI16` / `decodeU16` / `decodeI16`
- `encodeI32` / `decodeU32` / `decodeI32`
- `encodeI64` / `decodeU64` / `decodeI64`

### 切片批量双射
- `encodeI8Slice` / `decodeU8Slice`、`encodeI16Slice` / `decodeU16Slice`
- `encodeI32Slice` / `decodeU32Slice`、`encodeI64Slice` / `decodeU64Slice`

### ZigZag-Varint 编解码
- `VarintResult { value: i64, consumed: usize, valid: bool }`
- `varintLen(n: i64): usize`
- `encodeZigzagVarint(n: i64, out: []u8): usize`
- `decodeZigzagVarint(data: &[]u8, start: usize): VarintResult`
- `encodeZigzagVarintSlice(src: &[]i64, dst: []u8): usize`
- `decodeZigzagVarintSlice(src: &[]u8, dst: []i64): usize`
