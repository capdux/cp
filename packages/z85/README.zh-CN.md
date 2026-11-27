[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# z85

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Base85 multi-standard encoder/decoder supporting ZeroMQ Z85 (RFC 32) and Adobe Ascii85 with 'z' zero-compression and <~...~> framing (Rust z85 / ascii85 parity) for CP

## 安装与导入

```bash
cap add z85
```

```cp
import { z85Index } from "z85";
```

## 核心导出 API

- `export fn z85Index(c: u8): i8;`
- `export fn isSpace(c: u8): bool;`
- `export fn encodeZ85(input: string, out: []u8): string?;`
- `export fn decodeZ85(encoded: string, out: []u8): string?;`
- `export fn encodeAscii85(input: string, out: []u8): string?;`
- `export fn decodeAscii85(encoded: string, out: []u8): string?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`z85@1.0.0`)，许可证：MIT
