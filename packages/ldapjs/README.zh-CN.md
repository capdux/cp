[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ldapjs

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 4514 LDAP Distinguished Name parser, value escaper/unescaper, validator, and case-insensitive attribute lookup.

## 安装与导入

```bash
cap add ldapjs
```

```cp
import { isWs } from "ldapjs";
```

## 核心导出 API

- `export fn isWs(c: u8): bool;`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn eqIgnoreCase(a: string, b: string): bool;`
- `export fn fromHex(c: u8): i32;`
- `export fn isValidAttrType(attr: string): bool;`
- `export fn isEscapedAt(s: string, start: usize, idx: usize): bool;`
- `export fn hasValidEscapes(raw: string): bool;`
- `export fn parseDn(dn: string, out: []RdnAttr): usize;`
- `export fn escapeValue(value: string, out: []u8): usize;`
- `export fn unescapeValue(raw: string, out: []u8): usize;`
- `export fn findAttr(dn: string, target_attr: string, out: []u8, out_len: &mut usize): bool;`
- `export fn isValidDn(dn: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ldapjs@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`ldap3@1.0.0`)，许可证：MIT
