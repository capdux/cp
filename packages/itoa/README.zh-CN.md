[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# itoa

Fast zero-heap two-digits-per-step integer-to-ASCII formatter (clean-room CP implementation of crates.io itoa)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:itoa` ([https://github.com/dtolnay/itoa](https://github.com/dtolnay/itoa))

| 元信息 | 值 |
| --- | --- |
| 包名 | `itoa` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add itoa
```

```cp
import {
    formatU64,
    formatI64,
    formatU32,
    formatI32,
    formatUsize,
    formatHexU64,
    formatRadixU64,
} from "itoa";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `formatU64` | `(val: u64, out: []u8): usize` | 通过 `formatU64` 编码或格式化输出，返回 `usize`。 |
| `formatI64` | `(val: i64, out: []u8): usize` | 通过 `formatI64` 编码或格式化输出，返回 `usize`。 |
| `formatU32` | `(val: u32, out: []u8): usize` | 通过 `formatU32` 编码或格式化输出，返回 `usize`。 |
| `formatI32` | `(val: i32, out: []u8): usize` | 通过 `formatI32` 编码或格式化输出，返回 `usize`。 |
| `formatUsize` | `(val: usize, out: []u8): usize` | 通过 `formatUsize` 编码或格式化输出，返回 `usize`。 |
| `formatHexU64` | `(val: u64, uppercase: bool, out: []u8): usize` | 通过 `formatHexU64` 编码或格式化输出，返回 `usize`。 |
| `formatRadixU64` | `(val: u64, radix: u8, out: []u8): usize` | 通过 `formatRadixU64` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { formatU64 } from "itoa";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatU64(0, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
