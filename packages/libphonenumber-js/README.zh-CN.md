[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# libphonenumber-js

纯 CP 零堆分配国际电话号码解析、校验与格式化引擎（支持 E.164、国际、国内与 RFC 3966 格式）—— 洁净室 MIT 实现，对标 npm `libphonenumber-js`。

内置 45 个主流国家/地区元数据表（北美、亚太、欧洲、中东非洲、拉美），自动清理视觉分隔符并剥离国内干线前导 `0`（如澳大利亚 `0412`、英国 `07700`、日本 `090`），严格遵循 ITU-T E.164 长度与号段规则，格式化结果直接写入调用方栈缓冲区。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源平台 | **npm** |
| 原包名 | `libphonenumber-js`（CP 版 `v1.0.0`） |
| 上游仓库 | `https://github.com/catamphetamine/libphonenumber-js` |
| 许可证 | MIT |

从 TypeScript/Node.js 迁移至 CP 时无需改名：

```ts
// TypeScript / Node.js (npm: libphonenumber-js)
import { parsePhoneNumber } from "libphonenumber-js";
const phone = parsePhoneNumber("0412 345 678", "AU");
const e164 = phone.format("E.164"); // "+61412345678"
```

```cp
// CP (cap: libphonenumber-js) — 零堆分配栈上 PhoneNumber 与格式化输出
import { parsePhoneNumber } from "libphonenumber-js";

const phone = parsePhoneNumber("0412 345 678", "AU");
let buf: [32]u8 = undefined;
const e164 = phone.formatE164(buf[0..32]); // "+61412345678"
```

## 安装

```bash
cap add libphonenumber-js
```

## 使用示例

### 基础用法：解析并输出 4 种标准格式

```cp
import { parsePhoneNumber } from "libphonenumber-js";

fn demoFormat() {
    const phone = parsePhoneNumber("0412 345 678", "AU");
    if (phone.is_valid) {
        let b1: [32]u8 = undefined;
        let b2: [32]u8 = undefined;
        let b3: [32]u8 = undefined;
        let b4: [32]u8 = undefined;

        const e164 = phone.formatE164(b1[0..32]);          // "+61412345678"
        const intl = phone.formatInternational(b2[0..32]); // "+61 412 345 678"
        const nat  = phone.formatNational(b3[0..32]);      // "0412 345 678"
        const uri  = phone.formatRfc3966(b4[0..32]);       // "tel:+61-412-345-678"
        _ = e164;
        _ = intl;
        _ = nat;
        _ = uri;
    }
}
```

### 边界用法：`00` / `tel:` 前缀、区号互查与非法号码处理

```cp
import {
    parsePhoneNumber,
    isValidPhone,
    countryCodeFromIso2,
    iso2FromCountryCode,
    supportedCountryCount,
} from "libphonenumber-js";

fn demoEdgeCases() {
    // 1. 支持 00 国际冠码与 + 前缀（无需传 default_iso2）
    const cn = parsePhoneNumber("0086 138-0013-8000", "");
    const us_ok = isValidPhone("+1 (415) 555-2671", ""); // true

    // 2. ISO2 与国际区号双向查询（大小写不敏感）
    const hk_cc = countryCodeFromIso2("hk");  // 852
    const jp_iso = iso2FromCountryCode(81);   // "JP"
    const total = supportedCountryCount();    // 45

    // 3. 非法号码返回 is_valid == false，格式化安全返回 ""
    const bad = parsePhoneNumber("123", "US");
    let buf: [32]u8 = undefined;
    const empty = bad.formatE164(buf[0..32]); // ""
    _ = cn;
    _ = us_ok;
    _ = hk_cc;
    _ = jp_iso;
    _ = total;
    _ = empty;
}
```

## 核心 API 速查

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `PhoneNumber` | `class { national_buf: [16]u8, national_len: u8, country_code: u16, iso2: string, is_valid: bool, is_possible: bool }` | 栈上解析结果，提供 `formatE164`、`formatInternational`、`formatNational`、`formatRfc3966` 与 `invalid()` |
| `parsePhoneNumber` | `fn(raw: string, default_iso2: string): PhoneNumber` | 解析国际（`+`/`00`/`tel:`）或国内电话号码 |
| `isValidPhone` | `fn(raw: string, default_iso2: string): bool` | 快捷判断号码是否合法 |
| `countryCodeFromIso2` | `fn(iso2: string): u16` | ISO-3166 alpha-2 转国际区号（未知返回 `0`） |
| `iso2FromCountryCode` | `fn(cc: u16): string` | 国际区号反查主 ISO-3166 国家码（未知返回 `""`） |
| `supportedCountryCount` | `fn(): usize` | 返回内置支持的国家/地区总数（`45`） |

## 性能与安全亮点

- **16-lane SIMD 加速**：底层使用 `cap:ascii`（`isDigit`、`eqIgnoreCase`、`trim`）与 `cap:fmt`（`writeStr`、`writeU64`）加速数字提取与格式化。
- **零堆分配与静态表**：`PhoneNumber` 全栈存储（`[16]u8`），内置 45 国编译期只读元数据表。
- **`undefined` 零清零优化**：内部数字清洗缓冲（`[24]u8 = undefined`）跳过冗余清零开销。
- **E.164 与边界安全**：严格校验 ITU-T E.164 15 位上限与调用方 `out: []u8` 缓冲区边界。

## 许可证

MIT — 洁净室纯 CP 实现，灵感源自 npm `libphonenumber-js`。
