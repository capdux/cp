# libphonenumber-js@1.0.0 — AI Agent Quick Ref

> Zero-alloc international phone parser, validator & E.164/Intl/National/RFC3966 formatter (45 countries) | Source: `npm:libphonenumber-js` | License: MIT
> Import: `import { PhoneNumber, parsePhoneNumber, isValidPhone, countryCodeFromIso2, iso2FromCountryCode } from "libphonenumber-js";` | Types: `src/index.d.cp`

## Core Types

```cp
export class PhoneNumber {
    national_buf: [16]u8,
    national_len: u8,
    country_code: u16,
    iso2: string,
    is_valid: bool,
    is_possible: bool,
    fn invalid(): PhoneNumber;
    fn formatE164(out: []u8): string;
    fn formatInternational(out: []u8): string;
    fn formatNational(out: []u8): string;
    fn formatRfc3966(out: []u8): string;
}
```

## Core Functions

- `parsePhoneNumber(raw: string, default_iso2: string): PhoneNumber` — Accepts `+`, `00`, `tel:`, or national numbers with `default_iso2` (e.g. `"US"`, `"AU"`, `"CN"`, or `""` for international-only).
- `isValidPhone(raw: string, default_iso2: string): bool` — Fast validity check.
- `countryCodeFromIso2(iso2: string): u16` — Case-insensitive ISO2 -> calling code (`0` if unknown).
- `iso2FromCountryCode(cc: u16): string` — Calling code -> primary ISO2 (`""` if unknown).
- `supportedCountryCount(): usize` — Returns `45`.

## Minimal Usage

```cp
import { parsePhoneNumber } from "libphonenumber-js";

const p = parsePhoneNumber("0412 345 678", "AU");
if (p.is_valid) {
    let buf: [32]u8 = undefined;
    const e164 = p.formatE164(buf[0..32]); // "+61412345678"
}
```

## Pitfalls & Rules

1. **Buffer aliasing**: `formatE164`/`formatInternational`/`formatNational`/`formatRfc3966` return slices of `out`. Use separate buffers if keeping multiple formatted strings simultaneously.
2. **Buffer size**: Allocate at least `[32]u8` for formatted outputs (especially `formatRfc3966`, e.g. `"tel:+61-412-345-678"`).
3. **Invalid numbers**: Check `phone.is_valid` before formatting; invalid `PhoneNumber` formatters return `""`.
4. **National numbers need `default_iso2`**: Pass `""` only when `raw` starts with `+`, `00`, or `tel:+`; otherwise pass a 2-letter ISO code.
5. **Coverage**: 45 major countries built-in (US/CA, CN/AU/NZ/JP/KR/SG/HK/TW/MO/MY/TH/VN/ID/PH/IN, GB/DE/FR/IT/ES/NL/CH/SE/IE/PL/PT/BE/AT/NO/DK/FI/GR/RU/TR, AE/SA/IL/ZA, BR/MX/AR/CL/CO).
