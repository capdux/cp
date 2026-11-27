[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# validate-npm-package-license

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation npm package license field validator supporting SPDX, UNLICENSED, SEE LICENSE IN, and legacy values for CP.

## 安装与导入

```bash
cap add validate-npm-package-license
```

```cp
import { vnlContainsWhitespace } from "validate-npm-package-license";
```

## 核心导出 API

- `export fn vnlContainsWhitespace(s: string): bool;`
- `export fn vnlIsLegacyLicense(license: string): bool;`
- `export fn validateLicense(license: string): LicenseValidation;`
- `export fn isValidNewLicense(license: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`validate-npm-package-license@1.0.0`)，许可证：Apache-2.0
