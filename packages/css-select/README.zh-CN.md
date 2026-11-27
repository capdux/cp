[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# css-select

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap CSS selector parser, W3C Selectors Level 4 specificity calculator, and compound/list element matcher (clean-room port of css-select + css-what)

## 安装与导入

```bash
cap add css-select
```

```cp
import { csssIsSpace } from "css-select";
```

## 核心导出 API

- `export fn csssIsSpace(c: u8): bool;`
- `export fn csssIsCombinator(c: u8): bool;`
- `export fn csssIsIdentStart(c: u8): bool;`
- `export fn csssIsIdentChar(c: u8): bool;`
- `export fn csssStrEq(a: string, b: string): bool;`
- `export fn csssEqMaybeCi(a: string, b: string, ci: bool): bool;`
- `export fn csssTrim(s: string): string;`
- `export fn csssSkipIdent(s: string, pos: usize): usize;`
- `export fn csssFindBracketEnd(s: string, pos: usize): usize;`
- `export fn csssSkipParen(s: string, pos: usize): usize;`
- `export fn csssHasToken(list: string, target: string, ci: bool): bool;`
- `export fn csssContains(haystack: string, needle: string, ci: bool): bool;`
- `export fn csssIsLegacyPseudoElement(name: string): bool;`
- `export fn csssComputeSingleSpecificity(s: string, depth: u32): Specificity;`
- `export fn csssComputeListSpecificity(selector: string, depth: u32): Specificity;`
- `export fn computeSpecificity(selector: string): Specificity;`
- `export fn csssMatchAttr(raw_inner: string, el: CssElement): bool;`
- `export fn matchesCompoundSelector(selector: string, el: CssElement): bool;`
- `export fn matchesSelectorList(selector_list: string, el: CssElement): bool;`
- `export fn countSelectorParts(selector: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`css-select@1.0.0`)，许可证：BSD-2-Clause
