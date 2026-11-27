[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# punycode

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 3492 Punycode / IDNA (xn--...) encoder and decoder for internationalized domain names in CP.

## 安装与导入

```bash
cap add punycode
```

```cp
import { isAsciiOnly } from "punycode";
```

## 核心导出 API

- `export fn isAsciiOnly(s: string): bool;`
- `export fn encodeLabel(utf8_label: string, out: []u8): i64;`
- `export fn decodeLabel(ascii_label: string, out_utf8: []u8): i64;`
- `export fn toAsciiDomain(utf8_domain: string, out: []u8): i64;`
- `export fn toUnicodeDomain(ascii_domain: string, out: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`punycode@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`punycode@1.0.0`)，许可证：MIT
