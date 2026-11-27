[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# leb128

Zero-heap ULEB128 (unsigned) and SLEB128 (signed) variable-length integer encoder/decoder for WebAssembly, DWARF & binary formats (compatible with Rust leb128)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:leb128` ([https://github.com/gimli-rs/leb128](https://github.com/gimli-rs/leb128)) / `npm:leb` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| 元信息 | 值 |
| --- | --- |
| 包名 | `leb128` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add leb128
```

```cp
import {
    UlebResult,
    SlebResult,
    encodeUnsigned,
    decodeUnsigned,
    encodeSigned,
    decodeSigned,
    unsignedEncodedLen,
} from "leb128";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `UlebResult` | `class` | 字段: `value: u64, consumed: usize, valid: bool` |
| `SlebResult` | `class` | 字段: `value: i64, consumed: usize, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `encodeUnsigned` | `(val: u64, out: []u8): usize` | 通过 `encodeUnsigned` 编码或格式化输出，返回 `usize`。 |
| `decodeUnsigned` | `(data: &[]u8, start: usize): UlebResult` | 通过 `decodeUnsigned` 解析输入数据，返回 `UlebResult`。 |
| `encodeSigned` | `(val: i64, out: []u8): usize` | 通过 `encodeSigned` 编码或格式化输出，返回 `usize`。 |
| `decodeSigned` | `(data: &[]u8, start: usize): SlebResult` | 通过 `decodeSigned` 解析输入数据，返回 `SlebResult`。 |
| `unsignedEncodedLen` | `(val: u64): usize` | 调用 `unsignedEncodedLen` 并返回 `usize`。 |

## 最小调用示例

```cp
import { encodeUnsigned } from "leb128";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeUnsigned(0, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
