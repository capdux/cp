[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pkg-types

> 版本：`1.3.1` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap package.json and cap.json manifest parser and package name validator

## 安装与导入

```bash
cap add pkg-types
```

```cp
import { pkgtIsSpace } from "pkg-types";
```

## 核心导出 API

- `export fn pkgtIsSpace(c: u8): bool;`
- `export fn pkgtSkipSpace(s: string, start: usize): usize;`
- `export fn pkgtStrEq(a: string, b: string): bool;`
- `export fn pkgtScanStringEnd(s: string, open_quote: usize): usize;`
- `export fn pkgtSkipValue(s: string, start: usize): usize;`
- `export fn pkgtEmptyManifest(): PkgManifest;`
- `export fn parsePkgManifest(json_text: string): PkgManifest;`
- `export fn pkgtIsValidSegmentChar(c: u8): bool;`
- `export fn pkgtIsValidSegment(seg: string): bool;`
- `export fn isValidPkgName(name: string): bool;`
- `export fn splitPkgScope(name: string): PkgNameParts;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`pkg-types@1.3.1`)（https://github.com/unjs/pkg-types），许可证：MIT
