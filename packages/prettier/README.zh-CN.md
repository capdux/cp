[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# prettier

Zero-heap JSON and CSS pretty-printer formatter (clean-room CP implementation inspired by npm prettier)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:prettier` ([https://github.com/prettier/prettier](https://github.com/prettier/prettier))（原库名 `prettier` → CP 包名 `prettier`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `prettier` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add prettier
```

```cp
import {
    FormatOptions,
    pfIsSpace,
    pfPushByte,
    pfPushIndent,
    pfPeekNextNonSpace,
    formatJson,
    formatCss,
} from "prettier";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FormatOptions` | `class` | 字段: `tab_width: usize, use_tabs: bool, trailing_newline: bool` · 方法: `defaults` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pfIsSpace` | `(c: u8): bool` | 调用 `pfIsSpace` 并返回 `bool`。 |
| `pfPushByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `pfPushByte` 并返回 `usize`。 |
| `pfPushIndent` | `(out: []u8, pos: usize, depth: usize, opts: FormatOptions): usize` | 调用 `pfPushIndent` 并返回 `usize`。 |
| `pfPeekNextNonSpace` | `(s: string, start: usize): u8` | 调用 `pfPeekNextNonSpace` 并返回 `u8`。 |
| `formatJson` | `(input: string, opts: FormatOptions, out: []u8): usize` | 通过 `formatJson` 编码或格式化输出，返回 `usize`。 |
| `formatCss` | `(input: string, opts: FormatOptions, out: []u8): usize` | 通过 `formatCss` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { formatJson } from "prettier";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatJson("example", /* opts: FormatOptions */, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
