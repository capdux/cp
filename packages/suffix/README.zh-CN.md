[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# suffix

Zero-heap static Suffix Array + Kasai O(N) LCP array with O(M log N) binary search, occurrence counting, and longest repeated substring detection.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:suffix` ([https://github.com/BurntSushi/suffix](https://github.com/BurntSushi/suffix))（原库名 `suffix` → CP 包名 `suffix`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `suffix` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add suffix
```

```cp
import { LrsResult, SuffixArray, compareSuffixes } from "suffix";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LrsResult` | `class` | 字段: `start: usize, len: usize, found: bool` |
| `SuffixArray` | `class` | 字段: `text: [256]u8, sa: [256]u16, lcp: [256]u16, text_len: usize` · 方法: `build, len, suffixAt, lcpAt, comparePrefix, lowerBound, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `compareSuffixes` | `(buf: &[]u8, n: usize, a: usize, b: usize): i32` | 调用 `compareSuffixes` 并返回 `i32`。 |

## 最小调用示例

```cp
import { compareSuffixes } from "suffix";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = compareSuffixes(out_buf[0..256], 0, 0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
