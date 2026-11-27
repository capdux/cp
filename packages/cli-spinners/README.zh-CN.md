[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cli-spinners

Zero-allocation terminal spinner frame presets and Unicode/ASCII status symbols (cli-spinners / ora / log-symbols parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cli-spinners` ([https://github.com/sindresorhus/cli-spinners](https://github.com/sindresorhus/cli-spinners))

| 元信息 | 值 |
| --- | --- |
| 包名 | `cli-spinners` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cli-spinners
```

```cp
import {
    SpinnerKind,
    SymbolKind,
    intervalMs,
    frameCount,
    frameAt,
    frameAtTime,
    findSpinnerByName,
    logSymbol,
} from "cli-spinners";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `SpinnerKind` | `Dots, Dots2, Line, Pipe, Arc, Circle, SquareCorners, Bounce ...` |
| `enum` | `SymbolKind` | `Success, Error, Warning, Info` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `intervalMs` | `(kind: SpinnerKind): u32` | 调用 `intervalMs` 并返回 `u32`。 |
| `frameCount` | `(kind: SpinnerKind): usize` | 调用 `frameCount` 并返回 `usize`。 |
| `frameAt` | `(kind: SpinnerKind, tick: usize): string` | 调用 `frameAt` 并返回 `string`。 |
| `frameAtTime` | `(kind: SpinnerKind, elapsed_ms: u64): string` | 调用 `frameAtTime` 并返回 `string`。 |
| `findSpinnerByName` | `(name: string): SpinnerKind?` | 查询或读取目标数据，返回 `SpinnerKind?`。 |
| `logSymbol` | `(kind: SymbolKind, ascii_only: bool): string` | 调用 `logSymbol` 并返回 `string`。 |
| `renderSpinnerLine` | `(kind: SpinnerKind, tick: usize, text: string, out: []u8): string` | 调用 `renderSpinnerLine` 并返回 `string`。 |
| `renderStatusLine` | `(sym: SymbolKind, ascii_only: bool, text: string, out: []u8): string` | 调用 `renderStatusLine` 并返回 `string`。 |

## 最小调用示例

```cp
import { findSpinnerByName } from "cli-spinners";

pub fn main() {
    let result = findSpinnerByName("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
