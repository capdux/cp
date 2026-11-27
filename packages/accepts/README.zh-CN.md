[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# accepts

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 9110 HTTP content negotiation engine for Accept, Accept-Encoding, and Accept-Language headers, composing @cap/negotiator for CP

## 安装与导入

```bash
cap add accepts
```

```cp
import { parseQValue } from "accepts";
```

## 核心导出 API

- `export fn parseQValue(s_in: &[]u8): i32;`
- `export fn negotiateEncoding(accept_header: &[]u8, available: &[]&[]u8): i64;`
- `export fn negotiateLanguage(accept_header: &[]u8, available: &[]&[]u8): i64;`
- `export fn negotiateMediaType(accept_header: &[]u8, available: &[]&[]u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`accepts@1.0.0`)，许可证：MIT
