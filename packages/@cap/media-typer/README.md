# @cap/media-typer

Pure CP, zero-allocation RFC 6838 Section 4.2 media type parser and formatter (parity with npm `media-typer`).

## Features

- `parse(input: string): MediaType` — Zero-copy span parsing of `type/subtype` and `type/subtype+suffix` with strict RFC 6838 validation
- `testMediaType(input: string): bool` — Fast RFC 6838 validity check
- `format(t: string, subtype: string, suffix: string, out: []u8): usize` — Validates and formats a media type into a caller-provided buffer
- `isValidToken(s: string): bool` — RFC 6838 restricted-name token validation

## Usage

```cp
import { parse, format, testMediaType } from "@cap/media-typer";

let input = "application/vnd.api+json";
let mt = parse(input);
// mt.valid == true
// input[mt.type_start..mt.type_start + mt.type_len] == "application"
// input[mt.subtype_start..mt.subtype_start + mt.subtype_len] == "vnd.api"
// input[mt.suffix_start..mt.suffix_start + mt.suffix_len] == "json"

let buf: [64]u8 = undefined;
let n = format("application", "ld", "json", buf[0..64]);
// buf[0..n] == "application/ld+json"
```

## License

MIT
