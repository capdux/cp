[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# credit-card-type

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation payment card BIN network detector, Luhn validator, formatter, and PCI-DSS masker for CP

## 安装与导入

```bash
cap add credit-card-type
```

```cp
import { luhnValid } from "credit-card-type";
```

## 核心导出 API

- `export enum CardNetwork`
- `export fn luhnValid(raw: string): bool;`
- `export fn inspectCard(raw: string): CardInfo;`
- `export fn networkName(n: CardNetwork): string;`
- `export fn formatCardNumber(raw: string, out: []u8): string;`
- `export fn maskCardPci(raw: string, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`credit-card-type@1.0.0`)，许可证：MIT
