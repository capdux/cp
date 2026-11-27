[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# github-slugger

Zero-allocation Markdown heading outline extractor, GitHub-compatible anchor slugger & automatic nested TOC generator (github-slugger / markdown-toc parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:github-slugger` ([https://github.com/Flet/github-slugger](https://github.com/Flet/github-slugger))（原库名 `github-slugger` → CP 包名 `github-slugger`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `github-slugger` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add github-slugger
```

```cp
import {
    Heading,
    TocResult,
    appendByte,
    githubSlug,
    trimHeadingText,
    isFenceLine,
    slugExists,
    extractToc,
} from "github-slugger";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Heading` | `class` | 字段: `level: u8, text: string, slug_start: usize, slug_len: usize, line: usize` |
| `TocResult` | `class` | 字段: `headings: [MAX_HEADINGS]Heading, count: usize, slug_pool: [MAX_SLUG_POOL]u8, slug_pool_len: usize` · 方法: `empty, slugAt, levelAt, textAt, lineAt` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `githubSlug` | `(heading: string, out: []u8): string` | 调用 `githubSlug` 并返回 `string`。 |
| `trimHeadingText` | `(raw: string): string` | 调用 `trimHeadingText` 并返回 `string`。 |
| `isFenceLine` | `(line: string): bool` | 执行 `isFenceLine` 条件判定，返回 `bool`。 |
| `slugExists` | `(res: &mut TocResult, cand: string): bool` | 调用 `slugExists` 并返回 `bool`。 |
| `extractToc` | `(markdown: string): TocResult` | 调用 `extractToc` 并返回 `TocResult`。 |
| `renderTocMarkdown` | `(toc: TocResult, min_level: u8, max_level: u8, out: []u8): string` | 调用 `renderTocMarkdown` 并返回 `string`。 |

## 最小调用示例

```cp
import { isFenceLine } from "github-slugger";

pub fn main() {
    let result = isFenceLine("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
