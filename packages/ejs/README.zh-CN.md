[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ejs

Zero-heap EJS template engine supporting <%= escaped %>, <%- raw %>, <%# comments %>, <% if/else %>, and <% repeat %> tags (clean-room port of npm ejs)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ejs` ([https://github.com/mde/ejs](https://github.com/mde/ejs))（原库名 `ejs` → CP 包名 `ejs`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `ejs` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ejs
```

```cp
import {
    EjsEntry,
    EjsContext,
    ejsWriteStr,
    ejsLookupStr,
    ejsLookupBool,
    ejsLookupCount,
    ejsFindTagOpen,
    ejsFindTagClose,
} from "ejs";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `EjsEntry` | `class` | 字段: `key: string, str_val: string, bool_val: bool, count_val: usize, is_count: bool` |
| `EjsContext` | `class` | 字段: `entries: [32]EjsEntry, count: usize` · 方法: `new, setStr, setBool, setRepeatCount, size` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ejsWriteStr` | `(s: string, out: []u8, pos: usize): usize` | 调用 `ejsWriteStr` 并返回 `usize`。 |
| `ejsLookupStr` | `(ctx: EjsContext, key: string): string` | 调用 `ejsLookupStr` 并返回 `string`。 |
| `ejsLookupBool` | `(ctx: EjsContext, key: string): bool` | 调用 `ejsLookupBool` 并返回 `bool`。 |
| `ejsLookupCount` | `(ctx: EjsContext, key: string): usize` | 调用 `ejsLookupCount` 并返回 `usize`。 |
| `ejsFindTagOpen` | `(tpl: string, start_pos: usize): usize` | 调用 `ejsFindTagOpen` 并返回 `usize`。 |
| `ejsFindTagClose` | `(tpl: string, start_pos: usize): usize` | 调用 `ejsFindTagClose` 并返回 `usize`。 |
| `ejsFindBlockBounds` | `(tpl: string, body_start: usize, out_pos: []usize): bool` | 调用 `ejsFindBlockBounds` 并返回 `bool`。 |
| `ejsRenderSlice` | `(tpl: string, ctx: EjsContext, out: []u8, start_out: usize, depth: usize): usize` | 调用 `ejsRenderSlice` 并返回 `usize`。 |
| `renderEjs` | `(tpl: string, ctx: EjsContext, out: []u8): usize` | 调用 `renderEjs` 并返回 `usize`。 |

## 最小调用示例

```cp
import { ejsWriteStr } from "ejs";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ejsWriteStr("example", out_buf[0..256], 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
