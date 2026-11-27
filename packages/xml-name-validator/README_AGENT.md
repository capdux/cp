# xml-name-validator@1.0.0 (Agent Quickref)

> Zero-allocation W3C XML 1.0 (5th Ed.) & Namespaces in XML 1.0 Char, Name, NCName, and QName validator in pure CP | License: Apache-2.0
> Import: `import { ... } from "xml-name-validator";` | Full types: `src/index.d.cp`

## Functions
- `export fn xnvDecodeUtf8(s: string, pos: usize): XnvDecoded;`
- `export fn xnvValidateName(s: string, allow_colon: bool): bool;`
- `export fn isXmlChar(cp: u32): bool;`
- `export fn isNameStartChar(cp: u32): bool;`
- `export fn isNameChar(cp: u32): bool;`
- `export fn isValidXmlName(s: string): bool;`
- `export fn isValidNCName(s: string): bool;`
- `export fn isValidQName(s: string): bool;`
