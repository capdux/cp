[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# glob-match

Zero-allocation, anti-backtracking path glob and brace alternation matching engine for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:glob-match` ([https://github.com/devongovett/glob-match](https://github.com/devongovett/glob-match))

| 元信息 | 值 |
| --- | --- |
| 包名 | `glob-match` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add glob-match
```

```cp
import {
    GlobSpan,
    gbmFindClassClose,
    gbmMatchCharClass,
    gbmMatchCore,
    gbmFindBracePair,
    gbmCopySlice,
    gbmMatchWithBraces,
    globMatch,
} from "glob-match";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `GlobSpan` | `class` | 字段: `start: usize, end: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `gbmFindClassClose` | `(pattern: string, open_idx: usize): usize` | 调用 `gbmFindClassClose` 并返回 `usize`。 |
| `gbmMatchCharClass` | `(pattern: string, open_idx: usize, close_idx: usize, ch: u8): bool` | 调用 `gbmMatchCharClass` 并返回 `bool`。 |
| `gbmMatchCore` | `(pattern: string, path: string, record_caps: bool, out_caps: []GlobSpan, out_count: &mut usize): bool` | 调用 `gbmMatchCore` 并返回 `bool`。 |
| `gbmFindBracePair` | `(pattern: string, open_out: &mut usize, close_out: &mut usize): bool` | 调用 `gbmFindBracePair` 并返回 `bool`。 |
| `gbmCopySlice` | `(dst: []u8, pos: usize, src: string, s: usize, e: usize): usize` | 调用 `gbmCopySlice` 并返回 `usize`。 |
| `gbmMatchWithBraces` | `(pattern: string, path: string, depth: usize, steps: &mut usize, record_caps: bool, out_caps: []GlobSpan, out_count: &mut usize): bool` | 调用 `gbmMatchWithBraces` 并返回 `bool`。 |
| `globMatch` | `(pattern: string, path: string): bool` | 调用 `globMatch` 并返回 `bool`。 |
| `globMatchAny` | `(patterns: &[]string, path: string): bool` | 调用 `globMatchAny` 并返回 `bool`。 |
| `globMatchCaptures` | `(pattern: string, path: string, out_caps: []GlobSpan): usize` | 调用 `globMatchCaptures` 并返回 `usize`。 |
| `isStaticGlob` | `(pattern: string): bool` | 执行 `isStaticGlob` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isStaticGlob } from "glob-match";

pub fn main() {
    let result = isStaticGlob("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
