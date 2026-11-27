[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# normpath

Zero-heap cross-platform lexical path normalization (POSIX, Windows drive/UNC/verbatim) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:normpath` ([https://github.com/dylni/normpath](https://github.com/dylni/normpath))

| 元信息 | 值 |
| --- | --- |
| 包名 | `normpath` |
| 版本 | `1.0.0` |
| 许可证 | `MIT OR Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add normpath
```

```cp
import {
    npIsAsciiAlpha,
    npToUpper,
    npIsWinSep,
    npWriteByte,
    npWriteSlice,
    npSegEqDot,
    npSegEqDotDot,
    npNormalizePosixTwo,
} from "normpath";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `npIsAsciiAlpha` | `(b: u8): bool` | 调用 `npIsAsciiAlpha` 并返回 `bool`。 |
| `npToUpper` | `(b: u8): u8` | 调用 `npToUpper` 并返回 `u8`。 |
| `npIsWinSep` | `(b: u8): bool` | 调用 `npIsWinSep` 并返回 `bool`。 |
| `npWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `npWriteByte` 并返回 `usize`。 |
| `npWriteSlice` | `(out: []u8, pos: usize, src: string): usize` | 调用 `npWriteSlice` 并返回 `usize`。 |
| `npSegEqDot` | `(src: string, start: usize, len: usize): bool` | 调用 `npSegEqDot` 并返回 `bool`。 |
| `npSegEqDotDot` | `(src: string, start: usize, len: usize): bool` | 调用 `npSegEqDotDot` 并返回 `bool`。 |
| `npNormalizePosixTwo` | `(p1: string, p2: string, hasP2: bool, out: []u8): usize` | 调用 `npNormalizePosixTwo` 并返回 `usize`。 |
| `normalizePosix` | `(path: string, out: []u8): usize` | 调用 `normalizePosix` 并返回 `usize`。 |
| `joinNormalized` | `(base: string, rel: string, out: []u8): usize` | 调用 `joinNormalized` 并返回 `usize`。 |
| `isVerbatimWindows` | `(path: string): bool` | 执行 `isVerbatimWindows` 条件判定，返回 `bool`。 |
| `stripVerbatimPrefix` | `(path: string): string` | 调用 `stripVerbatimPrefix` 并返回 `string`。 |
| `npHasUncTag` | `(path: string, off: usize): bool` | 调用 `npHasUncTag` 并返回 `bool`。 |
| `normalizeWindows` | `(path: string, out: []u8): usize` | 调用 `normalizeWindows` 并返回 `usize`。 |
| `npWinIsAbsolute` | `(path: string): bool` | 调用 `npWinIsAbsolute` 并返回 `bool`。 |
| `joinNormalizedWindows` | `(base: string, rel: string, out: []u8): usize` | 调用 `joinNormalizedWindows` 并返回 `usize`。 |
| `hasParentTraversal` | `(path: string): bool` | 执行 `hasParentTraversal` 条件判定，返回 `bool`。 |
| `escapesRoot` | `(path: string): bool` | 调用 `escapesRoot` 并返回 `bool`。 |
| `isNormalized` | `(path: string): bool` | 执行 `isNormalized` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isVerbatimWindows } from "normpath";

pub fn main() {
    let result = isVerbatimWindows("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
