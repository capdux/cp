[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# anstyle

Zero-heap ANSI terminal styling model (16-color, 256-color, truecolor RGB, SGR effects) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:anstyle` ([https://github.com/rust-cli/anstyle](https://github.com/rust-cli/anstyle)) / `crates.io:console` ([https://github.com/console-rs/console](https://github.com/console-rs/console)) / `crates.io:colored` ([https://github.com/colored-rs/colored](https://github.com/colored-rs/colored))

| 元信息 | 值 |
| --- | --- |
| 包名 | `anstyle` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add anstyle
```

```cp
import {
    Color,
    Effects,
    Style,
    writeByte,
    writeU8Dec,
    writeParam,
} from "anstyle";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Color` | `class` | 字段: `kind: u8, r: u8, g: u8, b: u8` · 方法: `none, ansi, ansi256, rgb` |
| `Effects` | `class` | 字段: `bits: u16` · 方法: `new, isEmpty` |
| `Style` | `class` | 字段: `fg: Color, bg: Color, effects: Effects` · 方法: `new, isPlain, fgColor, bgColor, bold, dimmed, +9 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `writeByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `writeByte` 并返回 `usize`。 |
| `writeU8Dec` | `(out: []u8, pos: usize, val: u8): usize` | 调用 `writeU8Dec` 并返回 `usize`。 |
| `writeParam` | `(out: []u8, pos: usize, has_prev: bool, val: u8): usize` | 调用 `writeParam` 并返回 `usize`。 |

## 最小调用示例

```cp
import { writeByte } from "anstyle";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = writeByte(out_buf[0..256], 0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
