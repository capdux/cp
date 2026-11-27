[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# robots-parser

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 9309 robots.txt parser and URL allow/disallow evaluator for CP.

## 安装与导入

```bash
cap add robots-parser
```

```cp
import { isAllowed } from "robots-parser";
```

## 核心导出 API

- `export fn isAllowed(robots_txt: &[]u8, user_agent: &[]u8, path: &[]u8): bool;`
- `export fn crawlDelay(robots_txt: &[]u8, user_agent: &[]u8): i64;`
- `export fn firstSitemap(robots_txt: &[]u8): &[]u8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`robots-parser@1.0.0`)，许可证：MIT
