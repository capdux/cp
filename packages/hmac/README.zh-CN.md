[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# hmac

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 2104 streaming HMAC-SHA256 MAC with constant-time verify and hex output (crates.io hmac equivalent, reuses @cap/cookie-signature)

## 安装与导入

```bash
cap add hmac
```

```cp
import { hmcHexDigit } from "hmac";
```

## 核心导出 API

- `export fn hmcHexDigit(n: u8): u8;`
- `export fn encodeHex32(digest: []u8, out64: []u8): usize;`
- `export fn hmacSha256Hex(key: string, msg: string, out64: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`hmac@1.0.0`)，许可证：MIT OR Apache-2.0
