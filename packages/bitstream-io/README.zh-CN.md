[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bitstream-io

Zero-allocation MSB-first & LSB-first BitReader and BitWriter supporting 0..64-bit reads/writes, peek, byte alignment, and H.264/HEVC Exp-Golomb coding (Rust bitstream-io parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bitstream-io` ([https://github.com/tuffy/bitstream-io](https://github.com/tuffy/bitstream-io))（原库名 `bitstream-io` → CP 包名 `bitstream-io`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `bitstream-io` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bitstream-io
```

```cp
import { BitReadResult, BitReader, BitWriter } from "bitstream-io";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BitReadResult` | `class` | 字段: `value: u64, ok: bool` |
| `BitReader` | `class` | 字段: `buf: &[]u8, bit_pos: usize, msb_first: bool` · 方法: `initMsb, initLsb, bitPosition, bitsRemaining, readBitAt, readBit, +5 项` |
| `BitWriter` | `class` | 字段: `buf: []u8, bit_pos: usize, msb_first: bool` · 方法: `initMsb, initLsb, bitsWritten, bytesUsed, writeBit, writeBits, +2 项` |

## 最小调用示例

```cp
import { BitReadResult } from "bitstream-io";

pub fn main() {
    // Refer to src/index.d.cp for BitReadResult fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
