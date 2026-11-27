[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# property-information

> 版本：`6.5.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation HTML/SVG attribute and DOM property name mapping table

## 安装与导入

```bash
cap add property-information
```

```cp
import { pinfToLowerByte } from "property-information";
```

## 核心导出 API

- `export fn pinfToLowerByte(c: u8): u8;`
- `export fn pinfEqIgnoreCase(a: string, b: string): bool;`
- `export fn pinfHasPrefixIgnoreCase(s: string, prefix: string): bool;`
- `export fn pinfMake(property: string, attribute: string, is_boolean: bool, is_space_separated: bool, is_comma_separated: bool, is_numeric: bool): PropInfo;`
- `export fn pinfNotFound(name: string): PropInfo;`
- `export fn pinfMatchPair(name: string, prop: string, attr: string): bool;`
- `export fn findHtmlProp(name: string): PropInfo;`
- `export fn isBooleanAttr(name: string): bool;`
- `export fn isSpaceSeparatedAttr(name: string): bool;`
- `export fn isCommaSeparatedAttr(name: string): bool;`
- `export fn attrToDomProp(attr: string): string;`
- `export fn domPropToAttr(prop: string): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`property-information@6.5.0`)（https://github.com/wooorm/property-information），许可证：MIT
