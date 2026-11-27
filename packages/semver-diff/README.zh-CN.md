[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# semver-diff

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation SemVer upgrade diff classifier (major, minor, patch, pre*, build) and breaking-upgrade detector for CP, reusing semver.

## 安装与导入

```bash
cap add semver-diff
```

```cp
import { svdfBuildEq } from "semver-diff";
```

## 核心导出 API

- `export fn svdfBuildEq(a: SemVer, b: SemVer): bool;`
- `export fn semverDiffKind(ver_a: string, ver_b: string): u8;`
- `export fn semverDiff(ver_a: string, ver_b: string): string?;`
- `export fn isBreakingUpgrade(ver_a: string, ver_b: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`semver-diff@1.0.0`)，许可证：MIT
