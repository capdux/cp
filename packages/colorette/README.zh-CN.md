[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# colorette

Zero-heap pure CP terminal ANSI color formatter with nested close-code recovery (clean-room MIT implementation of npm colorette v2)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:colorette` ([https://github.com/jorgebucaran/colorette](https://github.com/jorgebucaran/colorette))

| 元信息 | 值 |
| --- | --- |
| 包名 | `colorette` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add colorette
```

```cp
import {
    ColoretteOptions,
    Colorette,
    clrFormatPair,
    isColorSupported,
    createColors,
    createColorsWithOptions,
} from "colorette";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ColoretteOptions` | `class` | 字段: `use_color: bool` · 方法: `enabled, disabled, fromEnv` |
| `Colorette` | `class` | 字段: `options: ColoretteOptions` · 方法: `new, fromOptions, isEnabled, formatPair, reset, bold, +40 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `clrFormatPair` | `(use_color: bool, pair: StylePair, replace_with: string, text: string, out: []u8): usize` | 调用 `clrFormatPair` 并返回 `usize`。 |
| `isColorSupported` | `(no_color: string, force_color: string, term: string, is_tty: bool): bool` | 执行 `isColorSupported` 条件判定，返回 `bool`。 |
| `createColors` | `(use_color: bool): Colorette` | 创建或初始化实例，返回 `Colorette`。 |
| `createColorsWithOptions` | `(opts: ColoretteOptions): Colorette` | 创建或初始化实例，返回 `Colorette`。 |

## 最小调用示例

```cp
import { isColorSupported } from "colorette";

pub fn main() {
    let result = isColorSupported("example", "example", "example", false);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
