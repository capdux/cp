# istextorbinary@1.0.0 (Agent Quickref)

> Zero-allocation WHATWG MIME Sniffing Standard binary/text detector and HTML/XML/JSON/SVG content sniffer for CP. | License: MIT
> Import: `import { ... } from "istextorbinary";` | Full types: `src/index.d.cp`

## Functions
- `export fn isBinary(data: &[]u8): bool;`
- `export fn isText(data: &[]u8): bool;`
- `export fn sniffContentType(data: &[]u8): u8;`
- `export fn contentTypeToMime(ct: u8): string;`
- `export fn sniffMime(data: &[]u8): string;`
