# packageurl-js@1.0.0 (Agent Quickref)

> Zero-allocation Package URL (PURL) parser, validator, qualifier lookup, and canonical serializer (SBOM standard) in pure CP | License: MIT
> Import: `import { ... } from "packageurl-js";` | Full types: `src/index.d.cp`

## Functions
- `export fn invalidPurl(): Purl;`
- `export fn purlIsSpace(b: u8): bool;`
- `export fn isValidType(pkg_type: string): bool;`
- `export fn trimSlashes(s: string): string;`
- `export fn parsePurl(input: string): Purl;`
- `export fn getQualifier(purl: Purl, key: string): string;`
- `export fn pushSlice(dst: []u8, pos: usize, src: string): i64;`
- `export fn pushLowerSlice(dst: []u8, pos: usize, src: string): i64;`
- `export fn formatPurl(p: Purl, out: []u8): usize;`
