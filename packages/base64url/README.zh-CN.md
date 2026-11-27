[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# base64url

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 4648 Section 5 Base64URL (no-padding) encoder, decoder, and standard Base64 converter (clean-room port of npm base64url)

## 安装与导入

```bash
cap add base64url
```

```cp
import { encodeBase64Url } from "base64url";
```

## 核心导出 API

- `export fn encodeBase64Url(src: &[]u8, out: []u8): usize;`
- `export fn decodeBase64Url(src: &[]u8, out: []u8): DecodeResult;`
- `export fn encodedBase64UrlLen(input_len: usize): usize;`
- `export fn fromStandardBase64(b64: string, out: []u8): usize;`
- `export fn toStandardBase64(b64url: string, out: []u8): usize;`
- `export fn isValidBase64Url(s: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`base64url@1.0.0`)，许可证：MIT
