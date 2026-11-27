[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# data-uri-to-buffer

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 2397 Data URI parser, Base64/percent decoder, and encoder for CP

## 安装与导入

```bash
cap add data-uri-to-buffer
```

```cp
import { toLowerAscii } from "data-uri-to-buffer";
```

## 核心导出 API

- `export fn toLowerAscii(c: u8): u8;`
- `export fn isWs(c: u8): bool;`
- `export fn trimWs(s: string): string;`
- `export fn eqIgnoreCase(a: string, b: string): bool;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendStr(out: []u8, pos: usize, s: string): usize;`
- `export fn hexDigitUpper(v: u8): u8;`
- `export fn parseHexNibble(c: u8): u8?;`
- `export fn isUnreservedUriByte(c: u8): bool;`
- `export fn parseDataUri(uri: string): DataUri;`
- `export fn isDataUri(s: string): bool;`
- `export fn b64Val(c: u8): u8?;`
- `export fn decodeBase64Payload(payload: string, out: []u8): ([]u8)?;`
- `export fn decodePercentPayload(payload: string, out: []u8): ([]u8)?;`
- `export fn decodeDataUri(uri: string, out: []u8): ([]u8)?;`
- `export fn encodeDataUriBase64(media_type: string, payload: []u8, out: []u8): string;`
- `export fn encodeDataUriText(media_type: string, text: string, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`data-uri-to-buffer@1.0.0`)，许可证：MIT
