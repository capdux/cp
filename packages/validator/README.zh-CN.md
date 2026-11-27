[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# validator

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation schema & format validation engine (Zod / validator.js / Rust validator parity: email, URL, UUID, ULID, IPv4/IPv6/CIDR, SemVer, ISO-8601, Luhn, E.164 & structured multi-error collector) for CP

## 安装与导入

```bash
cap add validator
```

```cp
import { isAscii } from "validator";
```

## 核心导出 API

- `export fn isAscii(s: string): bool;`
- `export fn isAlphanumeric(s: string): bool;`
- `export fn isNumeric(s: string): bool;`
- `export fn isHex(s: string): bool;`
- `export fn isBase64(s: string): bool;`
- `export fn isSlug(s: string): bool;`
- `export fn hasMinLen(s: string, min_len: usize): bool;`
- `export fn hasMaxLen(s: string, max_len: usize): bool;`
- `export fn hasLenBetween(s: string, min_len: usize, max_len: usize): bool;`
- `export fn startsWith(s: string, prefix: string): bool;`
- `export fn endsWith(s: string, suffix: string): bool;`
- `export fn containsStr(s: string, needle: string): bool;`
- `export fn isOneOf(s: string, allowed: []string): bool;`
- `export fn isEmail(s: string): bool;`
- `export fn isIpv4(s: string): bool;`
- `export fn isIpv6(s: string): bool;`
- `export fn isCidr(s: string): bool;`
- `export fn isUrl(s: string): bool;`
- `export fn isUuid(s: string): bool;`
- `export fn isUlid(s: string): bool;`
- `export fn isSemver(s: string): bool;`
- `export fn isIsoDate(s: string): bool;`
- `export fn isIsoDateTime(s: string): bool;`
- `export fn isLuhn(s: string): bool;`
- `export fn isE164Phone(s: string): bool;`
- `export fn isPort(p: i64): bool;`
- `export fn inRangeI64(v: i64, min_v: i64, max_v: i64): bool;`
- `export fn inRangeF64(v: f64, min_v: f64, max_v: f64): bool;`
- `export fn isMultipleOf(v: i64, step: i64): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`validator@1.0.0`)，许可证：MIT
