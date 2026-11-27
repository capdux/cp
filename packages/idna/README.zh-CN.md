[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# idna

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation UTS #46 Internationalized Domain Names (IDNA domainToAscii / domainToUnicode) reusing @cap/icu-normalizer for NFC and @cap/punycode for RFC 3492 Punycode (idna crate parity) in pure CP.

## 安装与导入

```bash
cap add idna
```

```cp
import { idnaToLowerAscii } from "idna";
```

## 核心导出 API

- `export fn idnaToLowerAscii(ch: u8): u8;`
- `export fn idnaIsLdhLower(ch: u8): bool;`
- `export fn idnaIsLdhAnyCase(ch: u8): bool;`
- `export fn isPunycodeLabel(label: string): bool;`
- `export fn idnaMapDomain(domain: string, out: []u8): i64;`
- `export fn idnaValidateAsciiLabelLower(label: &[]u8): i64;`
- `export fn idnaAppendBytes(out: []u8, pos: &mut usize, src: &[]u8): bool;`
- `export fn domainToAscii(domain: string, out: []u8): i64;`
- `export fn domainToUnicode(domain: string, out: []u8): i64;`
- `export fn isValidIdnaAscii(domain: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`idna@1.0.0`)，许可证：MIT
