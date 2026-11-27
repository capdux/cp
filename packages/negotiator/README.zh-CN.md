[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# negotiator

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 7231 HTTP content negotiation for Accept, Accept-Encoding, Accept-Language, and Accept-Charset with q-value ranking (clean-room port of npm negotiator)

## 安装与导入

```bash
cap add negotiator
```

```cp
import { parseQValue } from "negotiator";
```

## 核心导出 API

- `export fn parseQValue(raw: string): u32;`
- `export fn parseWeightedList(header: string, out: []QualitySpec): usize;`
- `export fn negotiateEncoding(accept_encoding: string, available: &[]string): string?;`
- `export fn negotiateLanguage(accept_language: string, available: &[]string): string?;`
- `export fn negotiateCharset(accept_charset: string, available: &[]string): string?;`
- `export fn parseAcceptList(header: string, out: []NegSpec): usize;`
- `export fn bestEncoding(header: string, available: &[]string): i64;`
- `export fn bestMediaType(header: string, available: &[]string): i64;`
- `export fn bestLanguage(header: string, available: &[]string): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`negotiator@1.0.0`)，许可证：MIT
