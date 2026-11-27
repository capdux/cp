# validator@1.0.0 (Agent Quickref)

> Zero-allocation schema & format validation engine (Zod / validator.js / Rust validator parity: email, URL, UUID, ULID, IPv4/IPv6/CIDR, SemVer, ISO-8601, Luhn, E.164 & structured multi-error collector) for CP | License: MIT
> Import: `import { ... } from "validator";` | Full types: `src/index.d.cp`

## Functions
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
- *(+5 more in `src/index.d.cp`)*
