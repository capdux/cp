[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/handlebars-tpl

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:handlebars` ([https://github.com/handlebars-lang/handlebars.js](https://github.com/handlebars-lang/handlebars.js))（原库名 `handlebars` → CP 包名 `@cap/handlebars-tpl`）

Zero-allocation Handlebars template engine for CP — clean-room MIT parity with npm `handlebars` v4. Composes `@cap/escape-html` for XSS-safe `{{var}}` escaping with zero duplication.

纯 CP 零堆分配 Handlebars 模板引擎（对标 npm `handlebars` v4），跨包复用 `@cap/escape-html` 实现 `{{var}}` 自动 HTML 转义防 XSS。

## Install / 安装

```bash
cap add @cap/handlebars-tpl
```

## Dependencies / 跨包复用

- `@cap/escape-html`: 零堆分配单遍 HTML 实体转义（`&`, `<`, `>`, `"`, `'`）

## Supported Syntax / 支持语法

| Feature | Syntax | Description |
| --- | --- | --- |
| Escaped interpolation | `{{ key }}` | HTML-escaped variable output via `@cap/escape-html` |
| Raw interpolation | `{{{ key }}}` | Unescaped raw variable output |
| Comments | `{{! note }}`, `{{!-- block --}}` | Stripped from rendered output |
| Conditional blocks | `{{#if key}}...{{else}}...{{/if}}` | Truthy branch (non-empty string, `true`, count `> 0`) |
| Inverse blocks | `{{#unless key}}...{{else}}...{{/unless}}` | Falsy branch |
| Repeat blocks | `{{#repeat key}}...{{/repeat}}` | Repeats body `N` times for `setCount` bindings (supports nesting) |
| Inline helpers | `{{upper key}}`, `{{lower key}}`, `{{default key fallback}}` | Built-in zero-alloc string transform & fallback helpers |

## API

- `HbsContext` (fixed 32-entry stack pool):
  - `HbsContext.new(): HbsContext`
  - `setStr(*_, k: string, v: string)`
  - `setBool(*_, k: string, v: bool)`
  - `setCount(*_, k: string, n: usize)`
  - `size(): usize`
- `renderHandlebars(tpl: string, ctx: HbsContext, out: []u8): usize`

## Usage / 使用示例

```cp
import { HbsContext, renderHandlebars } from "@cap/handlebars-tpl";

let ctx = HbsContext.new();
ctx.setStr("name", "<b>CP</b>");
ctx.setBool("vip", true);

let out: [256]u8 = undefined;
const n = renderHandlebars(
    "Hello {{name}}! {{#if vip}}Welcome {{upper name}}{{/if}}",
    ctx,
    out[0..256]
);
```
