[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# parseurl

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation high-speed HTTP request URL pathname/query zero-copy splitter and path safety checker for CP (clean-room parity with Express parseurl)

## 安装与导入

```bash
cap add parseurl
```

```cp
import { parseRequestUrl } from "parseurl";
```

## 核心导出 API

- `export fn parseRequestUrl(raw_url: string): ParsedRequestUrl;`
- `export fn decodePathname(pathname: string, out: []u8): usize;`
- `export fn isSafeRequestPath(pathname: string): bool;`
- `export fn getRequestQueryParam(raw_url: string, key: string, out: []u8): usize?;`
- `export fn decodeQueryValue(raw: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`parseurl@1.0.0`)，许可证：MIT
