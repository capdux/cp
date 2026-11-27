[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# css-what

> 版本：`1.0.0` | 许可证：`BSD-2-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation CSS selector tokenizer, parser, and serializer (Cheerio / css-select core) in pure CP

## 安装与导入

```bash
cap add css-what
```

```cp
import { cwhIsIdentStart } from "css-what";
```

## 核心导出 API

- `export enum CssTokenKind`
- `export enum AttrAction`
- `export fn cwhIsIdentStart(c: u8): bool;`
- `export fn cwhIsIdentChar(c: u8): bool;`
- `export fn cwhSkipSpace(s: string, start: usize): usize;`
- `export fn cwhScanIdent(s: string, start: usize): usize;`
- `export fn cwhIsCombinatorChar(c: u8): bool;`
- `export fn cwhToken(kind: CssTokenKind, name: string, value: string, action: AttrAction, ignore_case: bool): CssSelectorToken;`
- `export fn parseCssSelector(selector: string, out: []CssSelectorToken): i64;`
- `export fn stringifyCssSelector(tokens: &[]CssSelectorToken, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`css-what@1.0.0`)，许可证：BSD-2-Clause
