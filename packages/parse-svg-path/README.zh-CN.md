[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# parse-svg-path

Zero-allocation SVG path `d` attribute parser, bounding box calculator, and canonical formatter for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:parse-svg-path` ([https://github.com/jkroso/parse-svg-path](https://github.com/jkroso/parse-svg-path))（原库名 `parse-svg-path` → CP 包名 `parse-svg-path`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `parse-svg-path` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add parse-svg-path
```

```cp
import {
    PathCmd,
    ParsedPath,
    PathBounds,
    ParsedNum,
    BoundsAcc,
    skipWs,
    skipWsComma,
    expectedArgCount,
} from "parse-svg-path";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PathCmd` | `class` | 字段: `code: u8, relative: bool, args: [7]f64, arg_count: u8` · 方法: `empty` |
| `ParsedPath` | `class` | 字段: `cmds: [64]PathCmd, count: usize, valid: bool` · 方法: `invalid, cmdAt` |
| `PathBounds` | `class` | 字段: `valid: bool, min_x: f64, min_y: f64, max_x: f64, max_y: f64, width: f64, +1 项` · 方法: `empty` |
| `ParsedNum` | `class` | 字段: `ok: bool, val: f64, next_pos: usize` |
| `BoundsAcc` | `class` | 字段: `has_point: bool, min_x: f64, min_y: f64, max_x: f64, max_y: f64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `skipWs` | `(s: string, start: usize): usize` | 调用 `skipWs` 并返回 `usize`。 |
| `skipWsComma` | `(s: string, start: usize): usize` | 调用 `skipWsComma` 并返回 `usize`。 |
| `expectedArgCount` | `(code: u8): u8?` | 调用 `expectedArgCount` 并返回 `u8?`。 |
| `isLowerCmd` | `(code: u8): bool` | 执行 `isLowerCmd` 条件判定，返回 `bool`。 |
| `isNumberStart` | `(c: u8): bool` | 执行 `isNumberStart` 条件判定，返回 `bool`。 |
| `pow10` | `(exp: i32): f64` | 调用 `pow10` 并返回 `f64`。 |
| `parseSvgNumber` | `(s: string, start: usize): ParsedNum` | 通过 `parseSvgNumber` 解析输入数据，返回 `ParsedNum`。 |
| `parseSvgFlag` | `(s: string, start: usize): ParsedNum` | 通过 `parseSvgFlag` 解析输入数据，返回 `ParsedNum`。 |
| `parseSvgPath` | `(d: string): ParsedPath` | 通过 `parseSvgPath` 解析输入数据，返回 `ParsedPath`。 |
| `includePoint` | `(acc: BoundsAcc, x: f64, y: f64): BoundsAcc` | 调用 `includePoint` 并返回 `BoundsAcc`。 |
| `computeBounds` | `(p: ParsedPath): PathBounds` | 调用 `computeBounds` 并返回 `PathBounds`。 |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `appendFloatCompact` | `(out: []u8, start_pos: usize, val: f64): usize` | 调用 `appendFloatCompact` 并返回 `usize`。 |
| `formatSvgPath` | `(p: ParsedPath, out: []u8): string` | 通过 `formatSvgPath` 编码或格式化输出，返回 `string`。 |

## 最小调用示例

```cp
import { parseSvgNumber } from "parse-svg-path";

pub fn main() {
    let result = parseSvgNumber("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
