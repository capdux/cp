# media-typer@1.0.0 (Agent Quickref)

> Zero-heap RFC 6838 HTTP media type parser, validator, and formatter (clean-room port of npm media-typer) | License: MIT
> Import: `import { ... } from "media-typer";` | Full types: `src/index.d.cp`
> Deps: `unicase`

## Functions
- `export fn mtIsAlphaNum(b: u8): bool;`
- `export fn mtIsRestrictedChar(b: u8): bool;`
- `export fn mtIsValidName(s: string): bool;`
- `export fn mtPushLower(out: []u8, pos: usize, s: string): usize;`
- `export fn testMediaType(raw: string): bool;`
- `export fn parseMediaType(raw: string): MediaType;`
- `export fn formatMediaType(main_type: string, subtype: string, suffix: string, out: []u8): usize;`
- `export fn mediaTypeEq(a: string, b: string): bool;`
