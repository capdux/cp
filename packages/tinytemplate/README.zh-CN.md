[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# tinytemplate

纯 CP 实现的零堆分配 **Mustache / Handlebars 兼容模板引擎**，支持默认 HTML XSS 转义、链式管道过滤器（`trim`、`upper`、`lower`、`len`）、嵌套条件与列表遍历（`{{#if}}` / `{{#unless}}` / `{{#each}}`）以及严格的栈容量与嵌套深度防护。

## 源包溯源与零成本迁移

| 字段 | 说明 |
| :--- | :--- |
| **源平台** | [crates.io](https://crates.io/crates/tinytemplate) |
| **原包名与版本** | `tinytemplate`（v1 对齐） |
| **上游仓库** | https://github.com/bheisler/TinyTemplate |
| **CP 包名与版本** | `tinytemplate`（`1.0.0`） |
| **开源协议** | MIT |

在 CP 中保持原包名直接导入，无需改名：

```rust
// Rust (crates.io)
use tinytemplate::TinyTemplate;
```

```typescript
// CP (cap)
import { TemplateContext, TemplateError, render, escapeHtml } from "tinytemplate";
```

## 安装

```bash
cap add tinytemplate
```

## 核心特性与安全亮点

- **零堆分配**：`TemplateContext` 在栈上固定提供 64 个变量槽、16 个列表、128 个列表项与 512 字节整数格式化池，支持 `ctx.reset()` 复用。
- **默认 HTML XSS 转义**：`{{expr}}` 自动转义 `& < > " '`；仅在明确信任内容时使用三花括号 `{{{expr}}}` 输出原始字节。
- **链式管道过滤器**：支持 `{{ name | trim | upper }}`、`lower`、`len` 等链式管道求值。
- **完整块级控制流**：支持嵌套 `{{#if}}` / `{{else}}` / `{{/if}}`、`{{#unless}}` 以及 `{{#each list}}`（内置 `{{this}}` 与 `{{@index}}`），最大嵌套深度 `MAX_BLOCK_DEPTH = 16`。
- **强类型错误联合**：所有越界、未闭合标签、深度超限与未知过滤器均返回 `TemplateError`，绝不 panic 或静默截断。

## 快速上手

```typescript
import { TemplateContext, render } from "tinytemplate";

fn main(): !void {
    let ctx = TemplateContext.init();
    try ctx.set("user", "  <b>Ada</b>  ");
    try ctx.setBool("show_list", true);
    try ctx.setInt("year", 2026);
    try ctx.pushListItem("tags", "cp");
    try ctx.pushListItem("tags", "zero-alloc");

    const tpl =
        "<h1>{{ user | trim }} ({{year}})</h1>" +
        "{{#if show_list}}<ul>{{#each tags}}<li>{{@index}}: {{ this | upper }}</li>{{/each}}</ul>{{/if}}";

    let out: [1024]u8 = undefined;
    const rendered = try render(tpl, &ctx, out[0..]);
    _ = rendered;
}
```

## API 速览

| 导出符号 | 签名 / 值 | 说明 |
| :--- | :--- | :--- |
| `TemplateError` | 错误枚举 | `BufferTooSmall`, `UnclosedTag`, `UnclosedBlock`, `MismatchedBlock`, `DepthExceeded`, `ContextFull`, `UnknownFilter` |
| `TemplateContext.init` / `reset` | 构造与重置 | 初始化或清空上下文以便复用 |
| `TemplateContext.set` / `setBool` / `setInt` | 绑定变量 | 绑定字节串、布尔值或 64 位有符号整数 |
| `TemplateContext.pushListItem` | `(list_key: &[]u8, item: &[]u8) -> void ! TemplateError` | 向命名列表追加元素供 `{{#each}}` 遍历 |
| `render` | `(tpl: &[]u8, ctx: &TemplateContext, out: []u8) -> &[]u8 ! TemplateError` | 渲染模板并返回写入切片视图 |
| `escapeHtml` | `(input: &[]u8, out: []u8) -> &[]u8 ! TemplateError` | 独立 HTML XSS 转义函数 |

## 开源协议

MIT
