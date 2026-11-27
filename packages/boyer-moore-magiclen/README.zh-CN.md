[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# boyer-moore-magiclen

Zero-heap Boyer-Moore-Horspool sublinear substring search with reusable 256-byte bad-character table, reverse search, counting, and ASCII case-insensitive search.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:boyer-moore-magiclen` ([https://github.com/magiclen/boyer-moore-magiclen](https://github.com/magiclen/boyer-moore-magiclen))（原库名 `boyer-moore-magiclen` → CP 包名 `boyer-moore-magiclen`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `boyer-moore-magiclen` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add boyer-moore-magiclen
```

```cp
import {
    BmhPattern,
    compilePattern,
    findWithCompiled,
    findFirst,
    findLast,
    countNonOverlapping,
    findIgnoreCaseAscii,
} from "boyer-moore-magiclen";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BmhPattern` | `class` | 字段: `shift: [256]usize, pat_len: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `compilePattern` | `(needle: &[]u8): BmhPattern` | 调用 `compilePattern` 并返回 `BmhPattern`。 |
| `findWithCompiled` | `(haystack: &[]u8, needle: &[]u8, compiled: BmhPattern): i64` | 查询或读取目标数据，返回 `i64`。 |
| `findFirst` | `(haystack: &[]u8, needle: &[]u8): i64` | 查询或读取目标数据，返回 `i64`。 |
| `findLast` | `(haystack: &[]u8, needle: &[]u8): i64` | 查询或读取目标数据，返回 `i64`。 |
| `countNonOverlapping` | `(haystack: &[]u8, needle: &[]u8): usize` | 调用 `countNonOverlapping` 并返回 `usize`。 |
| `findIgnoreCaseAscii` | `(haystack: string, needle: string): i64` | 查询或读取目标数据，返回 `i64`。 |

## 最小调用示例

```cp
import { findIgnoreCaseAscii } from "boyer-moore-magiclen";

pub fn main() {
    let result = findIgnoreCaseAscii("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
