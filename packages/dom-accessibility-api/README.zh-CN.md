[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# dom-accessibility-api

> 版本：`0.6.3` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap W3C Accessible Name and Description computation (AccName) in pure CP

## 安装与导入

```bash
cap add dom-accessibility-api
```

```cp
import { a11yIsSpace } from "dom-accessibility-api";
```

## 核心导出 API

- `export fn a11yIsSpace(c: u8): bool;`
- `export fn a11yToLower(c: u8): u8;`
- `export fn a11yEqIgnoreCase(a: string, b: string): bool;`
- `export fn a11yUsesAlt(tag: string): bool;`
- `export fn collapseWhitespace(input: string, out: []u8): usize;`
- `export fn computeAccessibleName(node: A11yNodeInput, out: []u8): usize;`
- `export fn computeAccessibleDescription(aria_desc: string, title: string, used_title_as_name: bool, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`dom-accessibility-api@0.6.3`)（https://github.com/eps1lon/dom-accessibility-api），许可证：MIT
