# pem@1.0.0 (Agent Quickref)

> Zero-heap RFC 7468 PEM armor multi-block parser, validator, and 64-column wrapped encoder for certificates and keys | License: MIT
> Import: `import { ... } from "pem";` | Full types: `src/index.d.cp`

## Functions
- `export fn isValidLabel(label: string): bool;`
- `export fn parsePemFrom(input: string, from_offset: usize): PemBlock;`
- `export fn parsePem(input: string): PemBlock;`
- `export fn parsePemBundle(input: string): PemBundle;`
- `export fn extractBase64(blk: PemBlock, out: []u8): string;`
- `export fn encodePem(label: string, base64_payload: string, out: []u8): string;`
