[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# set-cookie-parser

> 版本：`2.7.1` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 6265 Set-Cookie HTTP header parser and splitter

## 安装与导入

```bash
cap add set-cookie-parser
```

```cp
import { parseSetCookie } from "set-cookie-parser";
```

## 核心导出 API

- `export fn parseSetCookie(header: string): ParsedCookie;`
- `export fn splitCookiesCount(combined: string): usize;`
- `export fn splitCookiesGet(combined: string, target_idx: usize): string?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`set-cookie-parser@2.7.1`)（https://github.com/nfriedly/set-cookie-parser），许可证：MIT
