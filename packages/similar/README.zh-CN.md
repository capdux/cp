[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# similar

Zero-allocation line-level LCS diff, unified diff formatter, and similarity ratio engine for CP (clean-room MIT, inspired by Rust similar v2).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:similar` ([https://github.com/mitsuhiko/similar](https://github.com/mitsuhiko/similar))（原库名 `similar` → CP 包名 `similar`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `similar` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add similar
```

```cp
import {
    DiffOp,
    DiffStats,
    SmdLine,
    smdSplitLines,
    smdLineEq,
    smdEmitOp,
    smdWriteBytes,
    smdWriteStr,
} from "similar";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DiffOp` | `class` | 字段: `tag: u8, old_idx: usize, new_idx: usize` |
| `DiffStats` | `class` | 字段: `equal_lines: usize, deleted_lines: usize, inserted_lines: usize, total_ops: usize` |
| `SmdLine` | `class` | 字段: `start: usize, end: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `smdSplitLines` | `(text: string, out_lines: []SmdLine): usize` | 调用 `smdSplitLines` 并返回 `usize`。 |
| `smdLineEq` | `(a: string, la: SmdLine, b: string, lb: SmdLine): bool` | 调用 `smdLineEq` 并返回 `bool`。 |
| `smdEmitOp` | `(ops_out: []DiffOp, stats: &mut DiffStats, tag: u8, old_idx: usize, new_idx: usize)` | 就地执行 `smdEmitOp` 操作。 |
| `smdWriteBytes` | `(out: []u8, pos: usize, src: string, start: usize, end: usize): usize` | 调用 `smdWriteBytes` 并返回 `usize`。 |
| `smdWriteStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `smdWriteStr` 并返回 `usize`。 |
| `smdWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `smdWriteByte` 并返回 `usize`。 |
| `smdWriteUsize` | `(out: []u8, pos: usize, val: usize): usize` | 调用 `smdWriteUsize` 并返回 `usize`。 |
| `diffLines` | `(old_text: string, new_text: string, ops_out: []DiffOp): DiffStats` | 调用 `diffLines` 并返回 `DiffStats`。 |
| `hasChanges` | `(old_text: string, new_text: string): bool` | 执行 `hasChanges` 条件判定，返回 `bool`。 |
| `similarityRatio` | `(old_text: string, new_text: string): u32` | 调用 `similarityRatio` 并返回 `u32`。 |
| `formatUnifiedDiff` | `(old_name: string, new_name: string, old_text: string, new_text: string, out: []u8): usize` | 通过 `formatUnifiedDiff` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { formatUnifiedDiff } from "similar";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatUnifiedDiff("example", "example", "example", "example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
