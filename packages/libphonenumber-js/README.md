[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# libphonenumber-js

Zero-allocation international phone number parser, validator, and formatter (E.164, International, National, RFC 3966) for CP — clean-room MIT parity with npm `libphonenumber-js`.

Embeds metadata for 45 major countries/regions (North America, APAC, Europe, Middle East/Africa, LATAM), strips visual separators and national trunk `0` prefixes automatically, validates ITU-T E.164 length and country-specific numbering rules, and formats directly into caller stack buffers.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **npm** |
| Original Package | `libphonenumber-js` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/catamphetamine/libphonenumber-js` |
| License | MIT |

Keep the exact package name when migrating from TypeScript/Node.js to CP:

```ts
// TypeScript / Node.js (npm: libphonenumber-js)
import { parsePhoneNumber } from "libphonenumber-js";
const phone = parsePhoneNumber("0412 345 678", "AU");
const e164 = phone.format("E.164"); // "+61412345678"
```

```cp
// CP (cap: libphonenumber-js) — zero-allocation stack PhoneNumber & formatters
import { parsePhoneNumber } from "libphonenumber-js";

const phone = parsePhoneNumber("0412 345 678", "AU");
let buf: [32]u8 = undefined;
const e164 = phone.formatE164(buf[0..32]); // "+61412345678"
```

## Installation

```bash
cap add libphonenumber-js
```

## Usage

### Basic Usage: Parse & Format All 4 Standard Representations

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

### Edge Cases: `00` / `tel:` Prefixes, Country Lookups & Invalid Numbers

```cp
import {
    parsePhoneNumber,
    isValidPhone,
    countryCodeFromIso2,
    iso2FromCountryCode,
    supportedCountryCount,
} from "libphonenumber-js";

fn demoEdgeCases() {
    // 1. International "00" prefix & "tel:" URI without default_iso2
    const cn = parsePhoneNumber("0086 138-0013-8000", "");
    const us_ok = isValidPhone("+1 (415) 555-2671", ""); // true

    // 2. Case-insensitive ISO2 <-> Calling Code lookup
    const hk_cc = countryCodeFromIso2("hk");  // 852
    const jp_iso = iso2FromCountryCode(81);   // "JP"
    const total = supportedCountryCount();    // 45

    // 3. Invalid input returns PhoneNumber with is_valid == false; formatters return ""
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

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `PhoneNumber` | `class { national_buf: [16]u8, national_len: u8, country_code: u16, iso2: string, is_valid: bool, is_possible: bool }` | Stack-allocated parsed number with `formatE164`, `formatInternational`, `formatNational`, `formatRfc3966`, and `PhoneNumber.invalid()`. |
| `parsePhoneNumber` | `fn(raw: string, default_iso2: string): PhoneNumber` | Parses international (`+`/`00`/`tel:`) or national phone numbers. |
| `isValidPhone` | `fn(raw: string, default_iso2: string): bool` | Quick validation helper returning `parsePhoneNumber(raw, default_iso2).is_valid`. |
| `countryCodeFromIso2` | `fn(iso2: string): u16` | Maps ISO-3166 alpha-2 (case-insensitive) to calling code (`0` if unknown). |
| `iso2FromCountryCode` | `fn(cc: u16): string` | Maps calling code to primary ISO-3166 alpha-2 (`""` if unknown). |
| `supportedCountryCount` | `fn(): usize` | Returns the number of built-in country entries (`45`). |

## Performance & Safety Highlights

- **16-lane SIMD Acceleration**: Uses `cap:ascii` (`isDigit`, `eqIgnoreCase`, `trim`) and `cap:fmt` (`writeStr`, `writeU64`) for fast digit extraction and formatting.
- **Zero Heap Allocation**: Fixed stack buffers (`[16]u8` national digits, `[24]u8` scratch arrays) and static 45-country metadata table.
- **Zero-Clear Stack Buffers (`undefined`)**: Scratch digit buffers use `undefined` initialization, eliminating unnecessary zero-fills.
- **Strict E.164 & Buffer Safety**: Enforces ITU-T E.164 15-digit maximum and bounds-checks every write to caller `out: []u8`.

## License

MIT — Clean-room CP implementation inspired by npm `libphonenumber-js`.
