[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# resolve-pkg-maps

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap package.json exports subpath wildcard and condition resolver

## 安装与导入

```bash
cap add resolve-pkg-maps
```

```cp
import { rpmEq } from "resolve-pkg-maps";
```

## 核心导出 API

- `export fn rpmEq(a: string, b: string): bool;`
- `export fn rpmFindStar(s: string): usize;`
- `export fn rpmHasParentTraversal(path: string): bool;`
- `export fn rpmConditionRank(cond: string, conditions: &[]string): usize;`
- `export fn matchSubpathPattern(pattern: string, subpath: string, matched_star: []u8): i64;`
- `export fn resolveExportTarget(rules: &[]ExportRule, subpath: string, conditions: &[]string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`resolve-pkg-maps@1.0.0`)（https://github.com/privatenumber/resolve-pkg-maps），许可证：MIT
