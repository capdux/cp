[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# potential_utf

Zero-allocation potentially-ill-formed UTF-8, UTF-16, and Unicode scalar value wrappers (ICU4X potential_utf parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:potential_utf` ([https://github.com/unicode-org/icu4x](https://github.com/unicode-org/icu4x))

| 元信息 | 值 |
| --- | --- |
| 包名 | `potential_utf` |
| 版本 | `1.0.0` |
| 许可证 | `Unicode-3.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add potential_utf
```

```cp
import {
    PotentialUtf8,
    PotentialUtf16,
    PotentialCodePoint,
    putfIsCont,
    putfValidSeqLen,
    putfMaximalSubpart,
    putfEncodeScalar,
    putfWriteReplacement,
} from "potential_utf";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PotentialUtf8` | `class` | 字段: `bytes: &[]u8` · 方法: `fromBytes, fromStr, asBytes, len, isEmpty, isValidUtf8, +2 项` |
| `PotentialUtf16` | `class` | 字段: `units: &[]u16` · 方法: `fromSlice, asSlice, len, isEmpty, isValidUtf16, toUtf8Lossy` |
| `PotentialCodePoint` | `class` | 字段: `raw: u32` · 方法: `fromU32, toU32, isValidScalar, toCharOrReplacement, encodeUtf8Lossy` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `putfIsCont` | `(b: u8): bool` | 写入或更新状态数据。 |
| `putfValidSeqLen` | `(bytes: &[]u8, pos: usize): usize` | 写入或更新状态数据。 |
| `putfMaximalSubpart` | `(bytes: &[]u8, pos: usize): usize` | 写入或更新状态数据。 |
| `putfEncodeScalar` | `(scalar: u32, out: []u8, w: usize): usize` | 写入或更新状态数据。 |
| `putfWriteReplacement` | `(out: []u8, w: usize): usize` | 写入或更新状态数据。 |

## 最小调用示例

```cp
import { putfIsCont } from "potential_utf";

pub fn main() {
    let result = putfIsCont(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
