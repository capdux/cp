# accepts@1.0.0 (Agent Quickref)

> Zero-allocation RFC 9110 HTTP content negotiation engine for Accept, Accept-Encoding, and Accept-Language headers, composing @cap/negotiator for CP | License: MIT
> Import: `import { ... } from "accepts";` | Full types: `src/index.d.cp`
> Deps: `negotiator`

## Functions
- `export fn parseQValue(s_in: &[]u8): i32;`
- `export fn negotiateEncoding(accept_header: &[]u8, available: &[]&[]u8): i64;`
- `export fn negotiateLanguage(accept_header: &[]u8, available: &[]&[]u8): i64;`
- `export fn negotiateMediaType(accept_header: &[]u8, available: &[]&[]u8): i64;`
