[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# sitemap

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Sitemaps.org 0.9 XML sitemap & sitemapindex generator and validator in pure CP (modeled after npm sitemap)

## 安装与导入

```bash
cap add sitemap
```

```cp
import { freqName } from "sitemap";
```

## 核心导出 API

- `export fn freqName(f: u8): &[]u8;`
- `export fn isValidPriorityTenths(p: i8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`sitemap@1.0.0`)，许可证：MIT
