[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# spdx

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation SPDX 2.3 license expression parser, validator, and compatibility evaluator for CP

## 安装与导入

```bash
cap add spdx
```

```cp
import { lookupLicense } from "spdx";
```

## 核心导出 API

- `export enum LicenseCategory`
- `export fn lookupLicense(id: string): LicenseInfo?;`
- `export fn validateExpression(expr: string): SpdxResult;`
- `export fn isPermissiveOnly(expr: string): bool;`
- `export fn satisfies(expr: string, allowed: &[]string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`spdx@1.0.0`)，许可证：MIT OR Apache-2.0
