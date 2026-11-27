# @cap/tel-uri

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:phonenumber` ([https://github.com/whisperfish/rust-phonenumber](https://github.com/whisperfish/rust-phonenumber))（原库名 `phonenumber` → CP 包名 `@cap/tel-uri`）

Zero-heap RFC 3966 `tel:` URI parser, E.164 international phone number normalizer/validator, and URI formatter in pure CP.

## Features

- `parseTelUri(uri)`: case-insensitive `tel:` parsing supporting global `+` numbers, local numbers with required `;phone-context=`, and `;ext=`
- `normalizeE164(input, out)`: strips `-`, `.`, `(`, `)`, spaces and validates E.164 (`+` + 7..15 digits, non-zero first digit)
- `isValidE164(number)`: strict E.164 validation
- `formatTelUri(e164_number, ext, out)`: formats `tel:<e164>[;ext=<ext>]`

## Usage

```cp
import { parseTelUri, normalizeE164 } from "@cap/tel-uri";

const t = parseTelUri("tel:+1-201-555-0123;ext=42");
let e164: [32]u8 = undefined;
const n = normalizeE164(t.number, e164[0..32]); // "+12015550123"
```
