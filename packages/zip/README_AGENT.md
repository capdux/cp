# zip@1.0.0 (Agent Quickref)

> Zero-heap PKZIP Stored archive writer, local header & EOCD parser, and CRC-32 verifier (crates.io zip equivalent) | License: MIT
> Import: `import { ... } from "zip";` | Full types: `src/index.d.cp`
> Deps: `bytemuck`, `crc32fast`

## Functions
- `export fn zipEmptyLocalHeader(): ZipLocalHeader;`
- `export fn zipEmptyEocd(): ZipEocd;`
- `export fn zipCopyStr(dst: []u8, offset: usize, s: string);`
- `export fn zipCopyBytes(dst: []u8, offset: usize, src: []u8);`
- `export fn writeStoredZipSingle(filename: string, payload: []u8, out: []u8): usize?;`
- `export fn parseLocalHeader(buf: []u8, offset: usize): ZipLocalHeader;`
- `export fn findAndParseEocd(buf: []u8): ZipEocd;`
- `export fn verifyStoredEntry(buf: []u8, hdr: ZipLocalHeader): bool;`
