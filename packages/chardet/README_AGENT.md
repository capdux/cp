# chardet@2.1.0 (Agent Quickref)

> Zero-allocation character encoding detector (BOM, UTF-8, UTF-16/32, ASCII, Shift-JIS, GBK, Windows-1252, ISO-8859-1) | License: MIT
> Import: `import { ... } from "chardet";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum DetectedCharset`

## Functions
- `export fn charsetName(cs: DetectedCharset): string;`
- `export fn detect(data: &[]u8): CharsetMatch;`
- `export fn detectName(data: &[]u8): string;`
