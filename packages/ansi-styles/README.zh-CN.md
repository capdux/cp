[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ansi-styles

ANSI escape codes for styling strings in the terminal (clean-room CP implementation of npm ansi-styles)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`ansi-styles`](https://github.com/chalk/ansi-styles)（`MIT`）（包名 `ansi-styles`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `ansi-styles` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ansi-styles
```

```cp
import {
    StylePair,
    asPushStr,
    asPushU8,
    wrapStyle,
    formatAnsi256,
    formatAnsi16m,
    formatRgbToAnsi16,
    formatRgbToAnsi256,
} from "ansi-styles";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `StylePair` | `class` | 字段: `open: string, close: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `asPushStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `asPushStr` 并返回 `usize`。 |
| `asPushU8` | `(out: []u8, pos: usize, v: u8): usize` | 调用 `asPushU8` 并返回 `usize`。 |
| `wrapStyle` | `(style: StylePair, text: string, out: []u8): usize` | 调用 `wrapStyle` 并返回 `usize`。 |
| `formatAnsi256` | `(code: u8, is_bg: bool, out: []u8): usize` | 通过 `formatAnsi256` 编码或格式化输出，返回 `usize`。 |
| `formatAnsi16m` | `(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize` | 通过 `formatAnsi16m` 编码或格式化输出，返回 `usize`。 |
| `formatRgbToAnsi16` | `(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize` | 通过 `formatRgbToAnsi16` 编码或格式化输出，返回 `usize`。 |
| `formatRgbToAnsi256` | `(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize` | 通过 `formatRgbToAnsi256` 编码或格式化输出，返回 `usize`。 |
| `formatHexToAnsi16m` | `(hex: string, is_bg: bool, out: []u8): usize` | 通过 `formatHexToAnsi16m` 编码或格式化输出，返回 `usize`。 |
| `formatKeywordToAnsi16m` | `(name: string, is_bg: bool, out: []u8): usize` | 通过 `formatKeywordToAnsi16m` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { formatHexToAnsi16m } from "ansi-styles";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatHexToAnsi16m("example", false, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
