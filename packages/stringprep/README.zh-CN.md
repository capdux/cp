[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# stringprep

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 3454 / RFC 4013 SASLprep & Nameprep string preparation for SCRAM-SHA-256, PostgreSQL, and LDAP in pure CP

## 安装与导入

```bash
cap add stringprep
```

```cp
import { decodeUtf8 } from "stringprep";
```

## 核心导出 API

- `export fn decodeUtf8(s: string, i: usize): DecCp;`
- `export fn encodeUtf8(cp: u32, out: []u8, pos: usize): i64;`
- `export fn isMappedToNothing(cp: u32): bool;`
- `export fn isNonAsciiSpace(cp: u32): bool;`
- `export fn isSaslProhibited(cp: u32): bool;`
- `export fn saslprep(input: string, out: []u8): PrepResult;`
- `export fn nameprepAscii(input: string, out: []u8): PrepResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`stringprep@1.0.0`)，许可证：MIT
