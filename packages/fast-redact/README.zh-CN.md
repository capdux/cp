[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# fast-redact

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation PII & secret redaction/masking engine (fast-redact / maskdata / secrecy parity: emails, Luhn credit cards, JWT/API tokens, phones, IPv4, and sensitive JSON fields) for CP

## 安装与导入

```bash
cap add fast-redact
```

```cp
import { maskEmail } from "fast-redact";
```

## 核心导出 API

- `export fn maskEmail(email: string, out: []u8): string;`
- `export fn maskPhone(phone: string, out: []u8): string;`
- `export fn maskCard(card: string, out: []u8): string;`
- `export fn maskSecret(secret: string, out: []u8): string;`
- `export fn maskIp(ip: string, out: []u8): string;`
- `export fn redactText(input: string, opts: RedactOptions, out: []u8): string;`
- `export fn containsSensitive(input: string): bool;`
- `export fn redactJsonFields(json_str: string, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`fast-redact@1.0.0`)，许可证：MIT
