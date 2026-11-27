[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# webpack-sources

Zero-heap source concatenation and range replacement engine

| 元信息 | 值 |
| --- | --- |
| 包名 | `webpack-sources` |
| 版本 | `3.2.3` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add webpack-sources
```

```cp
import {
    SourceEdit,
    ReplaceSource,
    wsrcWriteStr,
    wsrcEmptyEdit,
    concatSources,
} from "webpack-sources";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SourceEdit` | `class` | 字段: `start: usize, end: usize, replacement: string` |
| `ReplaceSource` | `class` | 字段: `original: string, edits: [64]SourceEdit, edit_count: usize` · 方法: `init, replace, insert, remove, render` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `wsrcWriteStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `wsrcWriteStr` 并返回 `usize`。 |
| `wsrcEmptyEdit` | `(): SourceEdit` | 调用 `wsrcEmptyEdit` 并返回 `SourceEdit`。 |
| `concatSources` | `(parts: &[]string, sep: string, out: []u8): usize` | 调用 `concatSources` 并返回 `usize`。 |

## 最小调用示例

```cp
import { wsrcWriteStr } from "webpack-sources";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = wsrcWriteStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
