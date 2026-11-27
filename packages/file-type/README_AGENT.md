# file-type@1.0.0 (Agent Quickref)

> Zero-allocation binary file magic bytes signature sniffer (25+ image, archive, document, executable, audio, video, and database formats) for CP. | License: MIT
> Import: `import { ... } from "file-type";` | Full types: `src/index.d.cp`

## Functions
- `export fn detect(data: &[]u8): FileType;`
- `export fn isImage(data: &[]u8): bool;`
- `export fn isArchive(data: &[]u8): bool;`
- `export fn isExecutable(data: &[]u8): bool;`
