[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# istanbul-lib-coverage

Zero-heap code coverage metrics, summary aggregation, and LCOV codec

| 元信息 | 值 |
| --- | --- |
| 包名 | `istanbul-lib-coverage` |
| 版本 | `3.2.2` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add istanbul-lib-coverage
```

```cp
import {
    CoverageMetric,
    CoverageSummary,
    LineHit,
    icovWriteByte,
    icovParseU32,
    formatLcovRecord,
    parseLcovSummary,
} from "istanbul-lib-coverage";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CoverageMetric` | `class` | 字段: `total: u32, covered: u32, skipped: u32` · 方法: `pct, merge` |
| `CoverageSummary` | `class` | 字段: `lines: CoverageMetric, statements: CoverageMetric, functions: CoverageMetric, branches: CoverageMetric` · 方法: `merge, isAboveThreshold` |
| `LineHit` | `class` | 字段: `line: u32, hits: u32` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `icovWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `icovWriteByte` 并返回 `usize`。 |
| `icovParseU32` | `(s: string): u32` | 调用 `icovParseU32` 并返回 `u32`。 |
| `formatLcovRecord` | `(file_path: string, hits: &[]LineHit, out: []u8): usize` | 通过 `formatLcovRecord` 编码或格式化输出，返回 `usize`。 |
| `parseLcovSummary` | `(lcov_text: string): CoverageMetric` | 通过 `parseLcovSummary` 解析输入数据，返回 `CoverageMetric`。 |

## 最小调用示例

```cp
import { formatLcovRecord } from "istanbul-lib-coverage";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatLcovRecord("example", /* hits: &[]LineHit */, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
