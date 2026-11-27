[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# libphonenumber-js

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation international phone number parser, validator, and formatter (E.164, International, National, RFC 3966) for CP

## 安装与导入

```bash
cap add libphonenumber-js
```

```cp
import { findCountryIndexByIso2 } from "libphonenumber-js";
```

## 核心导出 API

- `export fn findCountryIndexByIso2(iso2: string): i32;`
- `export fn findCountryIndexByCode(cc: u16): i32;`
- `export fn countryCodeDigitLen(cc: u16): usize;`
- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn pushGroupRange(out: []u8, pos: usize, buf: [16]u8, start: usize, end: usize): usize;`
- `export fn writeGroupedNational(out: []u8, pos: usize, cc: u16, buf: [16]u8, len: usize, sep: u8): usize;`
- `export fn validateNationalRules(cc: u16, buf: [16]u8, len: usize): bool;`
- `export fn supportedCountryCount(): usize;`
- `export fn countryCodeFromIso2(iso2: string): u16;`
- `export fn iso2FromCountryCode(cc: u16): string;`
- `export fn matchCallingCodePrefix(digits: [24]u8, n: usize): CcMatch;`
- `export fn parsePhoneNumber(raw: string, default_iso2: string): PhoneNumber;`
- `export fn isValidPhone(raw: string, default_iso2: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`libphonenumber-js@1.0.0`)，许可证：MIT
