# bech32@1.0.0 (Agent Quickref)

> Zero-allocation BIP-173 Bech32 and BIP-350 Bech32m encoder, decoder, 8-to-5 bit converter, and BCH checksum verifier for CP | License: MIT
> Import: `import { ... } from "bech32";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum Bech32Variant`

## Functions
- `export fn toLowerAscii(c: u8): u8;`
- `export fn charsetIndex(c: u8): u8?;`
- `export fn polymodStep(chk: u32, v: u8): u32;`
- `export fn polymodHrp(hrp: string): u32;`
- `export fn variantConst(v: Bech32Variant): u32;`
- `export fn isValidHrp(hrp: string): bool;`
- `export fn to5BitWords(bytes: string, out: []u8): usize?;`
- `export fn from5BitWords(words: &[]u8, out: []u8): usize?;`
- `export fn encodeBech32(hrp: string, payload_bytes: string, variant: Bech32Variant, out: []u8): string?;`
- `export fn validateStructureAndChecksum(encoded: string): ParsedBech32Meta;`
- `export fn verifyBech32(encoded: string): bool;`
- `export fn decodeBech32(encoded: string, hrp_out: []u8, data_out: []u8): Bech32Decoded;`
