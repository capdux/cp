# @cap/magic-bytes

Zero-allocation binary file magic bytes signature sniffer for CP.

## Features

- **Zero heap allocation**: inspects raw `[]const u8` header bytes directly without copying.
- **25+ standard formats**:
  - **Images**: PNG, JPEG, GIF, WebP, BMP, ICO
  - **Archives**: ZIP, GZIP, ZSTD, LZ4, BZIP2, XZ, 7Z, TAR
  - **Documents**: PDF
  - **Executables**: ELF, WebAssembly (WASM), PE/MZ, Mach-O
  - **Database**: SQLite3
  - **Audio / Video**: MP3/ID3, OGG, FLAC, WAV, MP4/ISO-BMFF
- **Convenience helpers**: `isImage`, `isArchive`, `isExecutable`.

## Usage

```cp
import { detect, isImage } from "@cap/magic-bytes";

const ft = detect(headerBytes);
if (ft.matched) {
    // ft.ext, ft.mime, ft.kind
}
```
