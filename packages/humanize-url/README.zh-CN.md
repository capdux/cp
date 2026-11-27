[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# humanize-url

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation URL humanizer stripping protocol, www., and trailing slash (npm humanize-url v3 parity) for CP

## 安装与导入

```bash
cap add humanize-url
```

```cp
import { hurlIsSpace } from "humanize-url";
```

## 核心导出 API

- `export fn hurlIsSpace(c: u8): bool;`
- `export fn hurlTrim(s: string): string;`
- `export fn hurlToLowerAscii(c: u8): u8;`
- `export fn hurlStartsWithLower(s: string, lower_prefix: string): bool;`
- `export fn hurlHasDot(s: string): bool;`
- `export fn humanizeUrlSlice(url: string): string;`
- `export fn humanizeUrl(url: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`humanize-url@1.0.0`)，许可证：MIT
