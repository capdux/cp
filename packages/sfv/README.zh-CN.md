[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# sfv

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation W3C User-Agent Client Hints (Sec-CH-UA*) RFC 8941 structured header parser and formatter for CP.

## 安装与导入

```bash
cap add sfv
```

```cp
import { parseBrands } from "sfv";
```

## 核心导出 API

- `export fn parseBrands(header: string, out: []BrandVersion): usize;`
- `export fn isGreaseBrand(brand: string): bool;`
- `export fn findBrandVersion(header: string, brand: string): string;`
- `export fn parseMobile(header: string): i32;`
- `export fn parsePlatform(header: string): string;`
- `export fn formatBrands(brands: &[]BrandVersion, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`sfv@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`ua-client-hints@1.0.0`)，许可证：MIT
