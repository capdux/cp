[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# validate-npm-package-name

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap npm/cap package name validator with scoped @org/pkg rules, reserved names, and legacy vs new validation (npm validate-npm-package-name equivalent)

## 安装与导入

```bash
cap add validate-npm-package-name
```

```cp
import { vnpStrEq } from "validate-npm-package-name";
```

## 核心导出 API

- `export fn vnpStrEq(a: string, b: string): bool;`
- `export fn vnpEqIgnoreCase(a: string, b: string): bool;`
- `export fn vnpIsUrlSafeChar(c: u8): bool;`
- `export fn vnpIsSpecialWarnChar(c: u8): bool;`
- `export fn isBlacklistedName(name: string): bool;`
- `export fn isCoreModuleName(name: string): bool;`
- `export fn vnpCheckSegment(seg: string): u8;`
- `export fn validatePackageName(name: string): PackageNameValidation;`
- `export fn isValidNewPackageName(name: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`validate-npm-package-name@1.0.0`)，许可证：ISC
