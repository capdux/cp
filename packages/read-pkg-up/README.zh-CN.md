[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# read-pkg-up

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation upward package.json finder, parser, and normalizer for CP, reusing find-up and read-pkg.

## 安装与导入

```bash
cap add read-pkg-up
```

```cp
import { rpuStrEq } from "read-pkg-up";
```

## 核心导出 API

- `export fn rpuStrEq(a: string, b: string): bool;`
- `export fn rpuCopyBytes(out: []u8, src: string): usize;`
- `export fn readPackageUpInEntries(cwd: string, stop_at: string, normalize: bool, entries: []PkgFileEntry, path_out: []u8): ReadPkgUpResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`read-pkg-up@1.0.0`)，许可证：MIT
