# simdutf8@1.0.0 (Agent Quickref)

> Zero-heap strict RFC 3629 UTF-8 validator with ASCII fast-path, basic/compat validation, valid_up_to & error_len reporting (crates.io simdutf8 equivalent) | License: MIT
> Import: `import { ... } from "simdutf8";` | Full types: `src/index.d.cp`

## Functions
- `export fn sutfIsCont(b: u8): bool;`
- `export fn isAscii(buf: []u8): bool;`
- `export fn validateUtf8Compat(buf: []u8): Utf8CompatResult;`
- `export fn basicIsValidUtf8(buf: []u8): bool;`
- `export fn countCodepoints(buf: []u8): usize?;`
