[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# tinytemplate

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Mustache/Handlebars-compatible template engine with default HTML XSS escaping, chained pipeline filters, nested conditionals, list iteration, and depth guards for CP

## 安装与导入

```bash
cap add tinytemplate
```

```cp
import { isValueTruthy } from "tinytemplate";
```

## 核心导出 API

- `export enum BlockKind`
- `export fn isValueTruthy(val: &[]u8): bool;`
- `export fn formatUsize(n: usize, buf: []u8): &[]u8;`
- `export fn formatI64(n: i64, buf: []u8): &[]u8;`
- `export fn appendBytes(out: []u8, pos: &mut usize, data: &[]u8): void ! TemplateError;`
- `export fn appendEscapedHtml(out: []u8, pos: &mut usize, input: &[]u8): void ! TemplateError;`
- `export fn escapeHtml(input: &[]u8, out: []u8): &[]u8 ! TemplateError;`
- `export fn findActiveEach(stack: &[16]BlockFrame, depth: usize): usize?;`
- `export fn resolveValue(key: &[]u8, ctx: &TemplateContext, stack: &[16]BlockFrame, depth: usize, index_buf: []u8): &[]u8;`
- `export fn resolveCondition(key: &[]u8, ctx: &TemplateContext, stack: &[16]BlockFrame, depth: usize): bool;`
- `export fn applyFilter(filter_name: &[]u8, input: &[]u8, dst: []u8): &[]u8 ! TemplateError;`
- `export fn evalExpressionAndEmit(expr: &[]u8, raw_output: bool, ctx: &TemplateContext, stack: &[16]BlockFrame, depth: usize, out: []u8, out_pos: &mut usize): void ! TemplateError;`
- `export fn findSubstring(haystack: &[]u8, start: usize, needle: &[]u8): usize?;`
- `export fn isBranchActive(stack: &[16]BlockFrame, depth: usize): bool;`
- `export fn render(tpl: &[]u8, ctx: &TemplateContext, out: []u8): &[]u8 ! TemplateError;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`tinytemplate@1.0.0`)，许可证：MIT
