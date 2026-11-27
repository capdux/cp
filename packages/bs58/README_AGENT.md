# bs58@1.0.0 (Agent Quickref)

> Zero-allocation Base58 encoder/decoder supporting Bitcoin, Ripple, and Flickr alphabets with leading-zero preservation (Rust bs58 / npm bs58 parity) for CP | License: MIT
> Import: `import { ... } from "bs58";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum Base58Alphabet`

## Functions
- `export fn alphabetChars(alpha: Base58Alphabet): string;`
- `export fn charIndex(alphabet: string, c: u8): i8;`
- `export fn isValidBase58(encoded: string, alpha: Base58Alphabet): bool;`
- `export fn encodeBase58With(input: string, alpha: Base58Alphabet, out: []u8): string?;`
- `export fn decodeBase58With(encoded: string, alpha: Base58Alphabet, out: []u8): string?;`
- `export fn encodeBase58(input: string, out: []u8): string?;`
- `export fn decodeBase58(encoded: string, out: []u8): string?;`
