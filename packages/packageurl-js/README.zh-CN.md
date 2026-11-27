[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# packageurl-js

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Package URL (PURL) parser, validator, qualifier lookup, and canonical serializer (SBOM standard) in pure CP

## 安装与导入

```bash
cap add packageurl-js
```

```cp
import { invalidPurl } from "packageurl-js";
```

## 核心导出 API

- `export fn invalidPurl(): Purl;`
- `export fn purlIsSpace(b: u8): bool;`
- `export fn isValidType(pkg_type: string): bool;`
- `export fn trimSlashes(s: string): string;`
- `export fn parsePurl(input: string): Purl;`
- `export fn getQualifier(purl: Purl, key: string): string;`
- `export fn pushSlice(dst: []u8, pos: usize, src: string): i64;`
- `export fn pushLowerSlice(dst: []u8, pos: usize, src: string): i64;`
- `export fn formatPurl(p: Purl, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`packageurl-js@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`packageurl@1.0.0`)，许可证：MIT
