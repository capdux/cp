# z85@1.0.0 (Agent Quickref)

> Zero-allocation Base85 multi-standard encoder/decoder supporting ZeroMQ Z85 (RFC 32) and Adobe Ascii85 with 'z' zero-compression and <~...~> framing (Rust z85 / ascii85 parity) for CP | License: MIT
> Import: `import { ... } from "z85";` | Full types: `src/index.d.cp`

## Functions
- `export fn encodeZ85(input: string, out: []u8): string?;`
- `export fn decodeZ85(encoded: string, out: []u8): string?;`
- `export fn encodeAscii85(input: string, out: []u8): string?;`
- `export fn decodeAscii85(encoded: string, out: []u8): string?;`
