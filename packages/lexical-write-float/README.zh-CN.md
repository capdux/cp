[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# lexical-write-float

Zero-allocation pure CP integer and floating-point formatter (rust-lexical lexical-write-float / lexical-write-integer parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:lexical-write-float` ([https://github.com/Alexhuszagh/rust-lexical](https://github.com/Alexhuszagh/rust-lexical))

| 元信息 | 值 |
| --- | --- |
| 包名 | `lexical-write-float` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add lexical-write-float
```

```cp
import {
    lexwCopyStr,
    lexwWritePaddedU64,
    lexwWriteSpecialFloat,
    writeUintRadix,
    writeIntRadix,
    writeFloatFixed,
    writeFloatSci,
} from "lexical-write-float";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `lexwCopyStr` | `(out: []u8, pos: usize, src: string): usize` | 调用 `lexwCopyStr` 并返回 `usize`。 |
| `lexwWritePaddedU64` | `(out: []u8, pos: usize, val: u64, width: usize): usize` | 调用 `lexwWritePaddedU64` 并返回 `usize`。 |
| `lexwWriteSpecialFloat` | `(val: f64, out: []u8): usize` | 调用 `lexwWriteSpecialFloat` 并返回 `usize`。 |
| `writeUintRadix` | `(val: u64, radix: u32, uppercase: bool, out: []u8): usize` | 调用 `writeUintRadix` 并返回 `usize`。 |
| `writeIntRadix` | `(val: i64, radix: u32, uppercase: bool, out: []u8): usize` | 调用 `writeIntRadix` 并返回 `usize`。 |
| `writeFloatFixed` | `(val: f64, decimals: u8, out: []u8): usize` | 调用 `writeFloatFixed` 并返回 `usize`。 |
| `writeFloatSci` | `(val: f64, precision: u8, out: []u8): usize` | 调用 `writeFloatSci` 并返回 `usize`。 |

## 最小调用示例

```cp
import { lexwCopyStr } from "lexical-write-float";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = lexwCopyStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
