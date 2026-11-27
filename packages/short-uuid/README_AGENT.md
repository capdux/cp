# short-uuid@1.0.0 (Agent Quickref)

> Zero-allocation 128-bit UUID to 22-char Flickr Base58 / Base62 / Cookie Base90 lossless short ID translator for CP | License: MIT
> Import: `import { ... } from "short-uuid";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseHexNibble(c: u8): u8?;`
- `export fn hexNibbleLower(v: u8): u8;`
- `export fn uuidToBytes(uuid: string, out16: []u8): bool;`
- `export fn bytesToUuid(bytes16: []u8, out36: []u8): string;`
- `export fn validateAlphabet(alphabet: string): bool;`
- `export fn findInAlphabet(alphabet: string, c: u8): u32?;`
- `export fn maxEncodedLen(base: u32): usize;`
- `export fn shortenUuidWith(uuid: string, alphabet: string, out: []u8): string?;`
- `export fn expandUuidWith(short_id: string, alphabet: string, out36: []u8): string?;`
- `export fn shortenUuid(uuid: string, out: []u8): string?;`
- `export fn expandUuid(short_id: string, out36: []u8): string?;`
