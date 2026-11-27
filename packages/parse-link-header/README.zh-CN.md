[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# parse-link-header

Zero-allocation RFC 8288 HTTP Link header parser and builder with GitHub/REST pagination helpers for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:parse-link-header` ([https://github.com/thlorenz/parse-link-header](https://github.com/thlorenz/parse-link-header))（原库名 `parse-link-header` → CP 包名 `parse-link-header`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `parse-link-header` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add parse-link-header
```

```cp
import {
    LinkEntry,
    LinkHeader,
    relMatches,
    skipToNextEntry,
    parseLinkHeader,
    appendByte,
    appendParam,
    formatLinkHeader,
} from "parse-link-header";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LinkEntry` | `class` | 字段: `url: string, rel: string, title: string, type_val: string, hreflang: string` · 方法: `empty` |
| `LinkHeader` | `class` | 字段: `entries: [32]LinkEntry, count: usize` · 方法: `empty, entryAt, getByRel, nextUrl, prevUrl, firstUrl, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `relMatches` | `(rel_attr: string, target: string): bool` | 调用 `relMatches` 并返回 `bool`。 |
| `skipToNextEntry` | `(s: string, start: usize): usize` | 调用 `skipToNextEntry` 并返回 `usize`。 |
| `parseLinkHeader` | `(header: string): LinkHeader` | 通过 `parseLinkHeader` 解析输入数据，返回 `LinkHeader`。 |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `appendParam` | `(out: []u8, pos: usize, name: string, val: string): usize` | 调用 `appendParam` 并返回 `usize`。 |
| `formatLinkHeader` | `(entries: []LinkEntry, out: []u8): string` | 通过 `formatLinkHeader` 编码或格式化输出，返回 `string`。 |
| `appendPageLink` | `(out: []u8, start_pos: usize, base_url: string, sep: u8, target_page: usize, rel: string, emitted: usize): usize` | 调用 `appendPageLink` 并返回 `usize`。 |
| `formatPaginationLinks` | `(base_url: string, page: usize, total_pages: usize, out: []u8): string` | 通过 `formatPaginationLinks` 编码或格式化输出，返回 `string`。 |

## 最小调用示例

```cp
import { parseLinkHeader } from "parse-link-header";

pub fn main() {
    let result = parseLinkHeader("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
