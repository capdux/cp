[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# handlebars

纯 CP 实现的零堆分配 **Handlebars** 模板引擎，底层组合 `escape-html` 实现默认 XSS 安全转义，支持内联 Helper（`upper` / `lower` / `default`）、模板注释以及带递归深度保护的 `{{#if}}` / `{{#unless}}` / `{{#repeat}}` 块级语法。

## 源包溯源与零成本迁移

| 字段 | 说明 |
| :--- | :--- |
| **源平台** | [npm](https://www.npmjs.com/package/handlebars) |
| **原包名与版本** | `handlebars`（v4 对齐） |
| **上游仓库** | https://github.com/handlebars-lang/handlebars.js |
| **CP 包名与版本** | `handlebars`（`1.0.0`） |
| **开源协议** | MIT |

在 CP 中保持原包名直接导入，无需改名：

```typescript
// TypeScript (npm)
import Handlebars from "handlebars";

// CP (cap)
import { HbsContext, renderHandlebars } from "handlebars";
```

## 安装

```bash
cap add handlebars
```

## 核心特性与安全亮点

- **默认 XSS 防护**：`{{var}}` 默认经 `escape-html` 转义 HTML 特殊字符；仅当明确需要输出可信 HTML 时使用三花括号 `{{{raw}}}`。
- **零堆分配**：`HbsContext` 在栈上静态存储最多 32 个键值绑定（字符串、布尔、无符号计数），`renderHandlebars` 直接渲染至调用方 `[]u8` 缓冲区。
- **内置内联 Helper**：支持 `{{upper key}}`、`{{lower key}}`、`{{default key "fallback"}}` 以及注释 `{{! ... }}`。
- **嵌套块与深度防护**：支持 `{{#if}}` / `{{else}}` / `{{/if}}`、`{{#unless}}` 与 `{{#repeat}}`，内置 16 层最大嵌套深度保护，防止恶意深层模板导致栈耗尽。

## 快速上手

```typescript
import { HbsContext, renderHandlebars } from "handlebars";

fn main(): void {
    let ctx = HbsContext.new();
    ctx.setStr("user", "<script>alert(1)</script>");
    ctx.setStr("role", "admin");
    ctx.setBool("active", true);
    ctx.setCount("stars", 3);

    const tpl =
        "<h1>Hello {{user}} ({{upper role}})</h1>" +
        "{{#if active}}<span>Status: {{default status \"online\"}}</span>{{else}}<span>offline</span>{{/if}}" +
        "{{#repeat stars}}*{{/repeat}}";

    let out: [512]u8 = undefined;
    const len = renderHandlebars(tpl, ctx, out[0..]);
    const html = out[0..len];
    _ = html;
}
```

## API 速览

| 导出符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `HbsContext.new` | `() -> HbsContext` | 创建空上下文（最多 32 条绑定） |
| `HbsContext.setStr` | `(k: string, v: string)` | 绑定或更新字符串变量 |
| `HbsContext.setBool` | `(k: string, v: bool)` | 绑定或更新布尔变量 |
| `HbsContext.setCount` | `(k: string, n: usize)` | 绑定或更新无符号计数（供 `{{#repeat}}` 与插值使用） |
| `HbsContext.size` | `() -> usize` | 返回当前已绑定变量数 |
| `renderHandlebars` | `(tpl: string, ctx: HbsContext, out: []u8) -> usize` | 渲染模板至 `out` 并返回写入字节数 |

## 开源协议

MIT
