[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查手册](README_AGENT.md)

# bit_field

面向 CP 的零堆内存位与位域操作工具库，支持 `u8`/`u16`/`u32`/`u64`/`usize`、字节切片以及 `BitArray` 位数组。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| **上游源平台** | [crates.io/crates/bit_field](https://crates.io/crates/bit_field) |
| **上游原包名** | `bit_field`（CP 版本 `1.0.0`） |
| **上游仓库** | https://github.com/phil-opp/rust-bit_field |
| **许可证** | MIT |

从 Rust 迁移到 CP 保持原包名不变：

```rust
// Rust
use bit_field::{BitField, BitArray};
```

```cp
// CP —— 原包名直接导入，零堆分配原生位运算
import {
    getBitU64, setBitU64, getBitsU64, setBitsU64,
    getBitsRangeU64, setBitsRangeU64, BitArray,
} from "bit_field";
```

## 安装

```bash
cap add bit_field
```

## 实战示例

### 1. 整数单比特与位域读写（`u8` / `u16` / `u32` / `u64` / `usize`）

```cp
import {
    getBitU8, setBitU8, getBitsU8, setBitsU8,
    getBitU16, setBitU16, getBitsU16, setBitsU16,
    getBitU32, setBitU32, getBitsU32, setBitsU32,
    getBitU64, setBitU64, getBitsU64, setBitsU64,
    getBitsRangeU64, setBitsRangeU64,
    getBitUsize, setBitUsize, getBitsUsize, setBitsUsize,
} from "bit_field";

fn demoIntegerBits() {
    // 窄位宽索引类型（u3/u4/u5/u6）在编译期杜绝移位越界
    mut reg: u32 = 0;
    reg = setBitU32(reg, 3, true);          // 置位第 3 位 -> 0b1000
    let is_set = getBitU32(reg, 3);         // true

    // 读写半开区间 [start, end) 位域
    reg = setBitsU32(reg, 8, 16, 0xAB);     // 向 [8..16) 写入 0xAB
    let field = getBitsU32(reg, 8, 16);     // 0xAB

    // 支持完整 64 位端点（end == 64 使用 u8 索引避免 u6 溢出）
    mut flags: u64 = 0;
    flags = setBitsRangeU64(flags, 56, 64, 0xFF);
    let top_byte = getBitsRangeU64(flags, 56, 64); // 0xFF
}
```

### 2. 字节切片位运算与 `BitArray`

```cp
import {
    BitArray, bitLengthSlice,
    getBitSlice, setBitSlice, toggleBitSlice,
    getBitsSlice, setBitsSlice, countOnesSlice,
} from "bit_field";

fn demoBitArray() {
    mut storage: [4]u8 = [0; 4]; // 栈上 32 位位数组
    mut bits = BitArray.fromSlice(&mut storage);

    let total_bits = bits.bitLength(); // 32
    bits.setBit(0, true);
    bits.setBit(15, true);
    bits.toggleBit(0);                 // 将第 0 位翻转为 false

    // 跨字节边界位域 [6..18) 读写
    bits.setBits(6, 18, 0xABC);
    let extracted = bits.getBits(6, 18); // 0xABC
    let popcount = bits.countOnes();

    bits.setAll();
    bits.clearAll();

    // 也可直接对切片调用自由函数
    setBitSlice(&mut storage, 10, true);
    let b10 = getBitSlice(&storage, 10); // true
    let ones = countOnesSlice(&storage); // 1
}
```

### 3. 位宽查询与掩码生成

```cp
import {
    bitLengthU8, bitLengthU16, bitLengthU32, bitLengthU64, bitLengthUsize,
    bfldMaskU8, bfldMaskU16, bfldMaskU32, bfldMaskU64, bfldFullMaskU64,
} from "bit_field";

fn demoMasks() {
    let w64 = bitLengthU64();      // 64
    let m4 = bfldMaskU8(4);        // 0x0F
    let m64 = bfldFullMaskU64(64); // 0xFFFF_FFFF_FFFF_FFFF
}
```

## 性能与安全亮点

- **零堆内存分配**：全部函数直接操作标量寄存器与调用方栈/切片缓冲区（`BitArray` 仅借用 `[]u8`）。
- **类型级移位安全**：采用精确窄位宽索引（`u3`/`u4`/`u5`/`u6`），编译期消除越界移位风险。
- **完整 64 位区间支持**：`getBitsRangeU64`、`setBitsRangeU64` 与 `bfldFullMaskU64` 安全支持 `end == 64` 边界，无移位溢出。
- **跨字节位域访问**：`BitArray` 及 `*Slice` 函数支持任意跨字节 `[start, end)` 位域读写（最宽 64 位）与置位计数。

## API 参考

完整类型定义见 [`src/index.d.cp`](src/index.d.cp)，AI 智能体极简速查见 [`README_AGENT.md`](README_AGENT.md)。
