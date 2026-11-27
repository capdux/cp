[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# mailto-link

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 6068 mailto: URI parser, recipient counter, email checker, and percent-encoding builder in pure CP

## 安装与导入

```bash
cap add mailto-link
```

```cp
import { invalidMailto } from "mailto-link";
```

## 核心导出 API

- `export fn invalidMailto(): MailtoUri;`
- `export fn isSpace(b: u8): bool;`
- `export fn toLowerAscii(b: u8): u8;`
- `export fn trimSlice(s: string): string;`
- `export fn eqIgnoreCase(a: string, lower_b: string): bool;`
- `export fn parseMailto(uri: string): MailtoUri;`
- `export fn countRecipients(to_list: string): usize;`
- `export fn isBasicEmail(addr: string): bool;`
- `export fn hexUpper(nibble: u8): u8;`
- `export fn isUnreserved(b: u8): bool;`
- `export fn pushSlice(dst: []u8, pos: usize, src: string): i64;`
- `export fn pushPctEncoded(dst: []u8, pos: usize, src: string, allow_addr_chars: bool): i64;`
- `export fn formatMailto(to: string, subject: string, body: string, cc: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`mailto-link@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`mailto@1.0.0`)，许可证：MIT
