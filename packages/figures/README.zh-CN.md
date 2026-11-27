[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# figures

Zero-allocation Unicode terminal symbols with automatic ASCII/Windows fallbacks composing is-unicode-supported in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:figures` ([https://github.com/sindresorhus/figures](https://github.com/sindresorhus/figures), `MIT`)

| 元信息 | 值 |
| --- | --- |
| 包名 | `figures` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add figures
```

```cp
import {
    FigureKind,
    figStartsAt,
    figWriteStr,
    figureFor,
    figureForEnv,
    replaceSymbols,
} from "figures";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `FigureKind` | `Tick, Cross, Star, Square, Circle, Info, Warning, Pointer ...` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `figStartsAt` | `(input: string, pos: usize, needle: string): bool` | 调用 `figStartsAt` 并返回 `bool`。 |
| `figWriteStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `figWriteStr` 并返回 `usize`。 |
| `figureFor` | `(kind: FigureKind, unicode_ok: bool): string` | 调用 `figureFor` 并返回 `string`。 |
| `figureForEnv` | `(kind: FigureKind, env: EnvInfo): string` | 调用 `figureForEnv` 并返回 `string`。 |
| `replaceSymbols` | `(input: string, unicode_ok: bool, out: []u8): usize` | 调用 `replaceSymbols` 并返回 `usize`。 |

## 最小调用示例

```cp
import { figStartsAt } from "figures";

pub fn main() {
    let result = figStartsAt("example", 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
