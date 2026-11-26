# `@cap/phone` — 零分配国际电话号码解析、校验与格式化引擎

> Zero-allocation international phone number parser, validator, and formatter for CP (Clean-room MIT implementation modeled after npm `libphonenumber-js` and Rust `phonenumber`).

## 概述 / Overview

`@cap/phone` 提供纯 CP、零外部依赖、零堆分配的国际电话号码解析、校验与格式化能力：

- **45 个主流国家/地区区号与长度元数据表**：覆盖北美（US/CA）、亚太（CN/AU/NZ/JP/KR/SG/HK/TW/MO/MY/TH/VN/ID/PH/IN）、欧洲（GB/DE/FR/IT/ES/NL/CH/SE/IE/PL/PT/BE/AT/NO/DK/FI/GR/RU/TR）、中东与非洲（AE/SA/IL/ZA）、拉美（BR/MX/AR/CL/CO）。
- **宽容解析与规范化**：支持 `+` 国际前缀、`00` 国际冠码、`tel:` URI 前缀，自动清理空格、括号、短横线、点号等视觉分隔符；支持默认国家补全及国内干线前导 `0` 剥离（如澳大利亚 `0412 345 678`、英国 `07700 900123`、日本 `090-1234-5678`）。
- **4 种标准格式化输出（零堆分配）**：直接写入调用方栈缓冲 `out: []u8`，支持 `formatE164`、`formatInternational`、`formatNational`、`formatRfc3966`。
- **严格合法性校验**：结合 ITU-T E.164 长度上限（≤ 15 位）、各国家/地区合理号码长度范围及主流国家号段规则进行校验。

## 安装 / Installation

```bash
cap add @cap/phone
```

## 导出 API / API Reference

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `PhoneNumber` | 栈分配解析结果结构体（`national_buf: [16]u8`、`national_len: u8`、`country_code: u16`、`iso2: string`、`is_valid: bool`、`is_possible: bool`） |
| `PhoneNumber.invalid(): PhoneNumber` | 构造空无效号码对象 |
| `phone.formatE164(out: []u8): string` | 格式化为 E.164 标准串（如 `"+61412345678"`） |
| `phone.formatInternational(out: []u8): string` | 格式化为可读国际格式（如 `"+61 412 345 678"`） |
| `phone.formatNational(out: []u8): string` | 格式化为国内本地格式（如 `"0412 345 678"`、`"(415) 555-2671"`） |
| `phone.formatRfc3966(out: []u8): string` | 格式化为 RFC 3966 URI（如 `"tel:+61-412-345-678"`） |
| `parsePhoneNumber(raw: string, default_iso2: string): PhoneNumber` | 解析国际或国内电话号码 |
| `isValidPhone(raw: string, default_iso2: string): bool` | 快捷判断电话号码是否合法 |
| `countryCodeFromIso2(iso2: string): u16` | 根据 ISO-3166 alpha-2 查询国际区号（大小写不敏感，未知返回 `0`） |
| `iso2FromCountryCode(cc: u16): string` | 根据国际区号反查主 ISO-3166 国家码（未知返回 `""`） |
| `supportedCountryCount(): usize` | 返回内置支持的国家/地区总数（`45`） |

## 使用示例 / Usage Examples

### 示例 1：解析并输出 4 种标准格式

```cp
import { parsePhoneNumber } from "@cap/phone";

fn demoFormat() {
    const phone = parsePhoneNumber("0412 345 678", "AU");
    if (phone.is_valid) {
        let buf: [32]u8 = [0; 32];
        const e164 = phone.formatE164(buf[0..32]);          // "+61412345678"
        const intl = phone.formatInternational(buf[0..32]); // "+61 412 345 678"
        const nat  = phone.formatNational(buf[0..32]);      // "0412 345 678"
        const uri  = phone.formatRfc3966(buf[0..32]);       // "tel:+61-412-345-678"
        _ = e164;
        _ = intl;
        _ = nat;
        _ = uri;
    }
}
```

### 示例 2：国际号码与 `00` 冠码解析、区号双向映射

```cp
import {
    parsePhoneNumber,
    isValidPhone,
    countryCodeFromIso2,
    iso2FromCountryCode,
} from "@cap/phone";

fn demoLookup() {
    const cn = parsePhoneNumber("0086 138-0013-8000", "");
    const ok = isValidPhone("+1 (415) 555-2671", "");
    const cc = countryCodeFromIso2("hk");  // 852
    const iso = iso2FromCountryCode(81);   // "JP"
    _ = cn;
    _ = ok;
    _ = cc;
    _ = iso;
}
```

## 许可协议 / License

MIT
