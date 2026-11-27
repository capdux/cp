[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# quoted-printable

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 2045 Section 6.7 MIME Quoted-Printable encoder and decoder with 76-col soft line breaks in pure CP

## 安装与导入

```bash
cap add quoted-printable
```

```cp
import { isDirectPrintable } from "quoted-printable";
```

## 核心导出 API

- `export fn isDirectPrintable(b: u8): bool;`
- `export fn encode(data: &[]u8, out: []u8): usize;`
- `export fn decode(input: string, out: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`quoted-printable@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`quoted_printable@1.0.0`)，许可证：MIT
