[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bit_field

适用于 CP 语言的零堆分配单比特与位域区间操作库（覆盖 `u8`、`u16`、`u32`、`u64`、`usize` 及任意字节切片 / `BitArray`），对齐 Rust [`bit_field`](https://github.com/phil-opp/rust-bit_field) crate（MIT 协议，CP v1.0.0）。

支持单比特读写/翻转、半开区间 `[start, end)` 位域提取与写入、跨字节 LSB 优先位域读写（单次最多 64 位）、置位计数（`countOnesSlice`）以及面向对象的零堆分配 `BitArray` 封装。

## 安装与导入

```bash
cap add bit_field
```

```cp
import {
    getBitU64,
    setBitU64,
    getBitsRangeU64,
    setBitsRangeU64,
    getBitsU32,
    setBitsU32,
    getBitsSlice,
    setBitsSlice,
    BitArray,
} from "bit_field";
```

## 核心 API 参考

### 整数单比特与位域函数

所有位区间均为半开区间 `[start, end)`（第 `0` 位为最低有效位 LSB）：

| 类型 | 函数列表 | 索引宽度 |
| :--- | :--- | :--- |
| `u8` | `bitLengthU8()`, `getBitU8`, `setBitU8`, `getBitsU8`, `setBitsU8`, `bfldMaskU8` | `u3`（`0..7`） |
| `u16` | `bitLengthU16()`, `getBitU16`, `setBitU16`, `getBitsU16`, `setBitsU16`, `bfldMaskU16` | `u4`（`0..15`） |
| `u32` | `bitLengthU32()`, `getBitU32`, `setBitU32`, `getBitsU32`, `setBitsU32`, `bfldMaskU32` | `u5`（`0..31`） |
| `u64` | `bitLengthU64()`, `getBitU64`, `setBitU64`, `getBitsU64`, `setBitsU64`, `bfldMaskU64` | `u6`（`0..63`） |
| `u64`（全宽区间） | `getBitsRangeU64(val, start: u8, end: u8): u64`, `setBitsRangeU64(val, start: u8, end: u8, bits: u64): u64`, `bfldFullMaskU64(width: u8): u64` | `u8`（`end` 可达 `64`） |
| `usize` | `bitLengthUsize()`, `getBitUsize`, `setBitUsize`, `getBitsUsize`, `setBitsUsize` | `u8` |

### 字节切片与 `BitArray` API

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `bitLengthSlice` | `(bytes: &[]u8): usize` | 返回切片总比特数（`bytes.len * 8`）。 |
| `getBitSlice` | `(bytes: &[]u8, bit_index: usize): bool` | 读取指定位置比特（字节内 LSB 优先）。 |
| `setBitSlice` | `(bytes: []u8, bit_index: usize, on: bool)` | 原地设置或清除指定位置比特。 |
| `toggleBitSlice` | `(bytes: []u8, bit_index: usize)` | 原地翻转指定位置比特。 |
| `getBitsSlice` | `(bytes: &[]u8, start: usize, end: usize): u64` | 跨字节提取 `[start, end)` 位域（最多 64 位）。 |
| `setBitsSlice` | `(bytes: []u8, start: usize, end: usize, value: u64)` | 原地向字节切片写入 `[start, end)` 位域。 |
| `countOnesSlice` | `(bytes: &[]u8): usize` | 统计字节切片中值为 `1` 的比特总数。 |
| `BitArray` | `fromSlice(buf: []u8)`, `bitLength()`, `getBit(i)`, `setBit(i, on)`, `toggleBit(i)`, `getBits(s, e)`, `setBits(s, e, val)`, `countOnes()`, `clearAll()`, `setAll()` | 包装调用方 `[]u8` 缓冲区的零堆分配位数组对象。 |

## 代码示例

### 1. 硬件寄存器 / 页表项位域操作（`u64`）

```cp
import { setBitU64, setBitsRangeU64, getBitsRangeU64, getBitU64 } from "bit_field";

fn buildPageEntry(phys_frame: u64): u64 {
    let mut entry: u64 = 0;
    entry = setBitU64(entry, 0, true);                  // Present 位
    entry = setBitU64(entry, 1, true);                  // Writable 位
    entry = setBitsRangeU64(entry, 12, 52, phys_frame); // 40 位物理页帧号
    let present = getBitU64(entry, 0);                  // true
    let frame = getBitsRangeU64(entry, 12, 52);         // phys_frame
    return if (present) { frame } else { 0 };
}
```

### 2. 跨字节紧凑协议位域（`BitArray`）

```cp
import { BitArray } from "bit_field";

fn bitArrayExample(): usize {
    let buf: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0];
    let mut bits = BitArray.fromSlice(buf[0..8]);
    bits.setBit(3, true);
    bits.setBits(5, 17, 0xABC); // 跨字节写入 12 位字段
    let v = bits.getBits(5, 17); // 0xABC
    bits.toggleBit(3);
    return bits.countOnes();
}
```

## 与源包差异与安全亮点

- **精确移位宽度类型**：定宽函数（`getBitU8`..`getBitU64`）采用 CP 窄无符号移位索引（`u3`/`u4`/`u5`/`u6`），在编译期即消除移位越界；当 `u64` 区间上界包含第 64 位时请使用 `getBitsRangeU64` / `setBitsRangeU64`。
- **无副作用整数接口 + 原地切片操作**：整数函数返回新值不修改原变量；`BitArray` 与 `*Slice` 直接原地操作调用方 `[]u8`，全程零堆分配。
- **越界安全**：字节切片位操作对越界索引自动钳位或忽略，不触发运行时崩溃。
