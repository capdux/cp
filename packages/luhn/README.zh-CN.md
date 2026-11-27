[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# luhn

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap ISO/IEC 7812 Luhn Mod-10, Damm, and Verhoeff check digit algorithms in pure CP

## 安装与导入

```bash
cap add luhn
```

```cp
import { luhnValidate } from "luhn";
```

## 核心导出 API

- `export fn luhnValidate(input: &[]u8): bool;`
- `export fn luhnCheckDigit(payload: &[]u8): i32;`
- `export fn dammCheckDigit(payload: &[]u8): i32;`
- `export fn dammValidate(input: &[]u8): bool;`
- `export fn verhoeffCheckDigit(payload: &[]u8): i32;`
- `export fn verhoeffValidate(input: &[]u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`luhn@1.0.0`)，许可证：MIT
