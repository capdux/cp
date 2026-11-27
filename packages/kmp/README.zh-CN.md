[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# kmp

Zero-allocation O(N+M) Knuth-Morris-Pratt (KMP) substring search and LPS failure function engine for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:kmp` ([https://github.com/magiclen/kmp](https://github.com/magiclen/kmp))（原库名 `kmp` → CP 包名 `kmp`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `kmp` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add kmp
```

```cp
import {
    computeLps,
    findFirst,
    findAll,
    countMatches,
    contains,
} from "kmp";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `computeLps` | `(pattern: &[]u8, out_lps: []usize): usize` | 调用 `computeLps` 并返回 `usize`。 |
| `findFirst` | `(haystack: &[]u8, needle: &[]u8): i64` | 查询或读取目标数据，返回 `i64`。 |
| `findAll` | `(haystack: &[]u8, needle: &[]u8, overlapping: bool, out_offsets: []usize): usize` | 查询或读取目标数据，返回 `usize`。 |
| `countMatches` | `(haystack: &[]u8, needle: &[]u8, overlapping: bool): usize` | 调用 `countMatches` 并返回 `usize`。 |
| `contains` | `(haystack: &[]u8, needle: &[]u8): bool` | 调用 `contains` 并返回 `bool`。 |

## 最小调用示例

```cp
import { computeLps } from "kmp";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = computeLps(out_buf[0..256], /* out_lps: []usize */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
