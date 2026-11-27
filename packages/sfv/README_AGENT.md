# sfv@1.0.0 (Agent Quickref)

> Zero-allocation W3C User-Agent Client Hints (Sec-CH-UA*) RFC 8941 structured header parser and formatter for CP. | License: MIT
> Import: `import { ... } from "sfv";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseBrands(header: string, out: []BrandVersion): usize;`
- `export fn isGreaseBrand(brand: string): bool;`
- `export fn findBrandVersion(header: string, brand: string): string;`
- `export fn parseMobile(header: string): i32;`
- `export fn parsePlatform(header: string): string;`
- `export fn formatBrands(brands: &[]BrandVersion, out: []u8): usize;`
