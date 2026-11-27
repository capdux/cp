[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# xml-name-validator

> 版本：`1.0.0` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation W3C XML 1.0 (5th Ed.) & Namespaces in XML 1.0 Char, Name, NCName, and QName validator in pure CP

## 安装与导入

```bash
cap add xml-name-validator
```

```cp
import { xnvDecodeUtf8 } from "xml-name-validator";
```

## 核心导出 API

- `export fn xnvDecodeUtf8(s: string, pos: usize): XnvDecoded;`
- `export fn xnvValidateName(s: string, allow_colon: bool): bool;`
- `export fn isXmlChar(cp: u32): bool;`
- `export fn isNameStartChar(cp: u32): bool;`
- `export fn isNameChar(cp: u32): bool;`
- `export fn isValidXmlName(s: string): bool;`
- `export fn isValidNCName(s: string): bool;`
- `export fn isValidQName(s: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`xml-name-validator@1.0.0`)，许可证：Apache-2.0
