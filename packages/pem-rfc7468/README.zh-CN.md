[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pem-rfc7468

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 7468 PEM encoder and strict decoder reusing base64ct for constant-time Base64 (RustCrypto pem-rfc7468 parity) in pure CP.

## 安装与导入

```bash
cap add pem-rfc7468
```

```cp
import { pemIsLabelChar } from "pem-rfc7468";
```

## 核心导出 API

- `export fn pemIsLabelChar(ch: u8): bool;`
- `export fn pemIsLabelValidBytes(s: string, start: usize, len: usize): bool;`
- `export fn isValidLabel(label: string): bool;`
- `export fn pemStartsWithAt(s: string, pos: usize, prefix: string): bool;`
- `export fn pemWriteBytes(out: []u8, pos: usize, src: string): usize;`
- `export fn pemWriteNewline(out: []u8, pos: usize, crlf: bool): usize;`
- `export fn pemAdvanceLineEnd(s: string, pos: usize): usize;`
- `export fn pemSkipLineEnding(s: string, pos: usize): usize;`
- `export fn pemFindDashes(s: string, start: usize, line_end: usize): i64;`
- `export fn pemOnlySpacesTabs(s: string, start: usize, end: usize): bool;`
- `export fn pemSliceEq(s: string, a_start: usize, a_len: usize, b_start: usize, b_len: usize): bool;`
- `export fn pemEncodedLen(label_len: usize, der_len: usize, crlf: bool): usize;`
- `export fn encodePem(label: string, der: &[]u8, line_ending_crlf: bool, out: []u8): i64;`
- `export fn locatePem(input: string): PemSlice;`
- `export fn decodeLabel(input: string, label_out: []u8): i64;`
- `export fn decodePem(input: string, label_out: []u8, label_len_out: &mut usize, der_out: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`pem-rfc7468@1.0.0`)，许可证：MIT OR Apache-2.0
