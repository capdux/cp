[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# convert_case

Zero-allocation 14-style string case converter (crates.io convert_case v0.6 parity) composing heck-case for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:convert_case` ([https://github.com/rutrum/convert-case](https://github.com/rutrum/convert-case))（原库名 `convert_case` → CP 包名 `convert_case`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `convert_case` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add convert_case
```

```cp
import {
    ccvReplaceUnderscoreWithSpace,
    ccvStripUnderscores,
    toFlatCase,
    toUpperFlatCase,
    toLowerWords,
    toUpperWords,
    toAlternatingCase,
    convertCase,
} from "convert_case";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ccvReplaceUnderscoreWithSpace` | `(out: []u8, len: usize)` | 就地执行 `ccvReplaceUnderscoreWithSpace` 操作。 |
| `ccvStripUnderscores` | `(out: []u8, len: usize): usize` | 调用 `ccvStripUnderscores` 并返回 `usize`。 |
| `toFlatCase` | `(src: string, out: []u8): usize` | 调用 `toFlatCase` 并返回 `usize`。 |
| `toUpperFlatCase` | `(src: string, out: []u8): usize` | 调用 `toUpperFlatCase` 并返回 `usize`。 |
| `toLowerWords` | `(src: string, out: []u8): usize` | 调用 `toLowerWords` 并返回 `usize`。 |
| `toUpperWords` | `(src: string, out: []u8): usize` | 调用 `toUpperWords` 并返回 `usize`。 |
| `toAlternatingCase` | `(src: string, out: []u8): usize` | 调用 `toAlternatingCase` 并返回 `usize`。 |
| `convertCase` | `(src: string, style: u8, out: []u8): usize` | 调用 `convertCase` 并返回 `usize`。 |

## 最小调用示例

```cp
import { toFlatCase } from "convert_case";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = toFlatCase("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
