[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# vcard

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 6350 vCard 4.0 (.vcf) contact generator and parser for CP.

## 安装与导入

```bash
cap add vcard
```

```cp
import { formatVCard } from "vcard";
```

## 核心导出 API

- `export fn formatVCard(card: VCardData, dst: []u8): i64;`
- `export fn parseVCard(vcf: &[]u8): ParsedVCard;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`vcard@1.0.0`)，许可证：MIT
