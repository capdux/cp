[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/mustache-tpl

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mustache` ([https://github.com/janl/mustache.js](https://github.com/janl/mustache.js))（原库名 `mustache` → CP 包名 `@cap/mustache-tpl`）

零堆分配的 CP 原生 Mustache v1.4 无逻辑模板引擎，Clean-Room MIT 实现对标 npm 高频模板包 `mustache` v4（6000 万+/周下载）。

跨包零重复复用 `@cap/escape-html`（`escapeHtml`）自动转义 `{{var}}` 插值防止 XSS 注入，全程零堆分配与小缓冲区安全截断。

## 特性

- **`MustacheContext`**：固定池（最多 32 条键值），支持 `setStr(key, val)`、`setBool(key, val)`、`setListCount(key, count)`
- **`renderMustache(template, ctx, out)`**：
  - `{{ key }}`：自动经 `@cap/escape-html` 转义的 HTML 安全插值
  - `{{{ key }}}` / `{{& key }}`：原始未转义插值
  - `{{# key }}...{{/ key }}`：Truthy 条件区块与列表重复渲染
  - `{{^ key }}...{{/ key }}`：Falsy 反转区块
  - `{{! comment }}`：注释静默剔除
  - 支持嵌套区块、未闭合标签容错与小缓冲区安全截断

## 依赖复用

- `@cap/escape-html`: `1.0.0`

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`janl/mustache.js`](https://github.com/janl/mustache.js)。
