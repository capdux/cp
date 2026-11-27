[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# version-compare

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 version-compare

## 安装与导入

```bash
cap add version-compare
```

```cp
import { vcIsSep } from "version-compare";
```

## 核心导出 API

- `export enum VersionCmp`
- `export fn vcIsSep(b: u8): bool;`
- `export fn vcTagRank(s: string, start: usize, len: usize): i32;`
- `export fn vcCmpLexLower(a: string, aStart: usize, aLen: usize, b: string, bStart: usize, bLen: usize): i32;`
- `export fn vcParse(raw: string): VcParsed;`
- `export fn compareVersions(a: string, b: string): VersionCmp;`
- `export fn isLess(a: string, b: string): bool;`
- `export fn isEqual(a: string, b: string): bool;`
- `export fn isGreater(a: string, b: string): bool;`
- `export fn satisfies(ver: string, op: string, target: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`version-compare@1.0.0`)，许可证：MIT
