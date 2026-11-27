[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# npm-package-arg

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap npm package name validator and package specifier (@scope/name@version) parser and formatter.

## 安装与导入

```bash
cap add npm-package-arg
```

```cp
import { isValidSegment } from "npm-package-arg";
```

## 核心导出 API

- `export fn isValidSegment(seg: string): bool;`
- `export fn isValidPackageName(name: string): bool;`
- `export fn parsePackageSpec(input: string): PackageSpec;`
- `export fn appendSlice(out: []u8, pos: usize, s: string): i64;`
- `export fn formatPackageSpec(spec: PackageSpec, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`npm-package-arg@1.0.0`)，许可证：ISC
