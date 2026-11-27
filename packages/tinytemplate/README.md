[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `tinytemplate` — 零分配、默认 XSS 安全防护模板引擎

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tinytemplate` ([https://github.com/bheisler/TinyTemplate](https://github.com/bheisler/TinyTemplate))（原库名 `tinytemplate` → CP 包名 `tinytemplate`）

> 独立 Clean-Room 纯 CP 实现（严格对标 npm `handlebars` / `mustache` [周下载 60M+] 与 Rust `minijinja` / `tera` crate）| 许可协议：MIT

## 架构概述

`tinytemplate` 是面向高并发 Web 服务、SSR 页面渲染、邮件/通知生成与 Agent Prompt 拼装设计的纯 CP、零外部依赖、零堆分配模板引擎：

- **零堆分配与 `undefined` 栈缓冲**：`TemplateContext` 与渲染栈帧全部常驻栈内存，数组默认 `undefined` 延迟初始化，零清零、零 `malloc`/`free` 开销；所有输出直接写入调用方栈缓冲区。
- **单遍词法扫描与状态机执行**：无需构建 AST 或中间字节码，单遍扫描模板并在遇到 `{{#each}}` 时原地回跳复用模板切片，渲染开销极低。
- **默认 HTML XSS 安全防护**：
  - 双花括号 `{{ key }}` 默认对 `&`、`&amp;`，`<`、`&lt;`，`>`、`&gt;`，`"`、`&quot;`，`'`、`&#39;` 全量转义，从根源阻断跨站脚本注入（XSS）；
  - 仅在显式使用三花括号 `{{{ key }}}` 时输出原始未转义内容；
  - 独立导出 `escapeHtml(input, out)` 供业务层复用。
- **完整模板语法与链式管道过滤器**：
  - 变量插值与点号扁平路径：`{{ user.name }}`
  - 链式管道过滤器：`{{ name | trim | upper }}`（内置 `upper`、`lower`、`trim`、`len`，未知过滤器立即返回 `TemplateError.UnknownFilter`）
  - 注释剥离：`{{! 单行注释 }}` 与 `{{!-- 块注释 --}}`
  - 可嵌套条件分支：`{{#if key}}...{{else}}...{{/if}}` 与 `{{#unless key}}...{{else}}...{{/unless}}`（空字符串、`""`、`"false"`、`"0"`、空列表或未定义均为 falsy）
  - 列表迭代：`{{#each list_key}}...{{else}}...{{/each}}`，自动暴露循环内建变量 `{{ this }}`、`{{ @index }}`、`{{ @first }}`、`{{ @last }}`（同时支持在 `#if`/`#unless` 条件中直接使用）
- **严格异常边界防护**：内置 `MAX_BLOCK_DEPTH = 16` 嵌套深度护栏，完整校验未闭合标签（`UnclosedTag`）、未闭合块（`UnclosedBlock`）、不匹配块（`MismatchedBlock`）、上下文容量溢出（`ContextFull`）与输出缓冲区不足（`BufferTooSmall`）。

## 版权与许可

Copyright (c) 2026 CP-Lang Contributors. Released under the **MIT License**.

## 导出 API

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `MAX_BLOCK_DEPTH` | 最大块嵌套深度护栏常量（`16`） |
| `MAX_VARS` / `MAX_LISTS` / `MAX_LIST_ITEMS` | `TemplateContext` 标量、列表与列表项容量上限（`64` / `16` / `128`） |
| `TemplateError` | 错误集：`BufferTooSmall`、`UnclosedTag`、`UnclosedBlock`、`MismatchedBlock`、`DepthExceeded`、`ContextFull`、`UnknownFilter` |
| `TemplateContext.init()` | 创建零堆分配栈上上下文 |
| `TemplateContext.reset(*_)` | 重置上下文状态以便高频复用 |
| `TemplateContext.set(*_, key, val)` | 设置/更新标量字符串（支持 `"user.name"` 点号扁平路径） |
| `TemplateContext.setBool(*_, key, b)` | 设置/更新布尔值（`"true"` / `"false"`） |
| `TemplateContext.setInt(*_, key, n)` | 设置/更新 64 位有符号整数（存入内部整数池，按值移动不悬空） |
| `TemplateContext.get(key)` | 查询标量值（未定义返回 `null`） |
| `TemplateContext.has(key)` | 判断标量键是否存在 |
| `TemplateContext.isTruthy(key)` | 按模板真值规则评估标量或列表 |
| `TemplateContext.pushListItem(*_, list_key, item)` | 向指定列表追加元素 |
| `TemplateContext.listLen(list_key)` / `listItem(list_key, idx)` | 查询列表长度与指定索引元素 |
| `render(tpl, ctx, out)` | 单遍零堆分配渲染模板至 `out`，返回 `([]const u8) ! TemplateError` |
| `escapeHtml(input, out)` | 将 `& < > " '` 转义为 HTML 实体，返回 `([]const u8) ! TemplateError` |

## 使用示例

### 示例 1：默认 HTML XSS 自动转义与 `{{{ raw }}}` 原始输出

```cp
import { TemplateContext, render } from "tinytemplate";

fn demoXssSafety() {
    let ctx = TemplateContext.init();
    ctx.set("user.name", "<script>alert('xss')</script>") catch return;
    ctx.set("badge", "<strong>PRO</strong>") catch return;

    const tpl = "<p>Hello, {{ user.name }}!</p><div>{{{ badge }}}</div>";
    let out: [256]u8 = undefined;
    const html = render(tpl, &ctx, out[0..]) catch return;
    // html == "<p>Hello, &lt;script&gt;alert(&#39;xss&#39;)&lt;/script&gt;!</p><div><strong>PRO</strong></div>"
    * = html;
}
```

### 示例 2：链式管道过滤器与嵌套条件分支

```cp
import { TemplateContext, render } from "tinytemplate";

fn demoFiltersAndConditionals() {
    let ctx = TemplateContext.init();
    ctx.set("title", "  hello cp  ") catch return;
    ctx.setBool("is_admin", true) catch return;
    ctx.setInt("unread", 0) catch return;

    const tpl = "{{! 导航栏 }}<h1>{{ title | trim | upper }} ({{ title | trim | len }})</h1>"
        ++ "{{#if is_admin}}Admin{{#unless unread}} [All Clear]{{/unless}}{{else}}Guest{{/if}}";
    let out: [256]u8 = undefined;
    const rendered = render(tpl, &ctx, out[0..]) catch return;
    // rendered == "<h1>HELLO CP (8)</h1>Admin [All Clear]"
    * = rendered;
}
```

### 示例 3：列表迭代 `#each` 与 `@index` / `@first` / `@last` / `this`

```cp
import { TemplateContext, render } from "tinytemplate";

fn demoEachLoop() {
    let ctx = TemplateContext.init();
    ctx.pushListItem("tags", "cp") catch return;
    ctx.pushListItem("tags", "zig") catch return;
    ctx.pushListItem("tags", "beam") catch return;

    const tpl = "{{#each tags}}{{#if @first}}[{{/if}}{{@index}}:{{this | upper}}{{#unless @last}}, {{/unless}}{{#if @last}}]{{/if}}{{/each}}";
    let out: [256]u8 = undefined;
    const rendered = render(tpl, &ctx, out[0..]) catch return;
    // rendered == "[0:CP, 1:ZIG, 2:BEAM]"
    * = rendered;
}
```
