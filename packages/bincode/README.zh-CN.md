[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bincode

Zero-allocation bincode v2 compact binary serializer & zero-copy decoder (LE varint, ZigZag signed ints, length-prefixed strings, Option, fixed LE integers — Rust bincode parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bincode` ([https://github.com/bincode-org/bincode](https://github.com/bincode-org/bincode))

| 元信息 | 值 |
| --- | --- |
| 包名 | `bincode` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bincode
```

```cp
import {
    OptStrResult,
    BincodeEncoder,
    BincodeDecoder,
    zigzagEncode,
    zigzagDecode,
} from "bincode";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `OptStrResult` | `class` | 字段: `ok: bool, has_value: bool, value: string` |
| `BincodeEncoder` | `class` | 字段: `out: []u8, pos: usize, overflow: bool` · 方法: `init, asSlice, writeByte, encodeBool, encodeFixedU32, encodeFixedU64, +4 项` |
| `BincodeDecoder` | `class` | 字段: `data: string, pos: usize` · 方法: `init, remaining, atEnd, decodeBool, decodeFixedU32, decodeFixedU64, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `zigzagEncode` | `(v: i64): u64` | 调用 `zigzagEncode` 并返回 `u64`。 |
| `zigzagDecode` | `(u: u64): i64` | 调用 `zigzagDecode` 并返回 `i64`。 |

## 最小调用示例

```cp
import { zigzagEncode } from "bincode";

pub fn main() {
    let result = zigzagEncode(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
