# `@cap/mime-guess`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:mime_guess` ([https://github.com/abonander/mime_guess](https://github.com/abonander/mime_guess))（原库名 `mime_guess` → CP 包名 `@cap/mime-guess`）

Zero-allocation file path & extension MIME type guesser for CP (clean-room MIT implementation inspired by Rust's `mime_guess` v2, backed directly by `@cap/mime-types`).

## Features

- **Zero Heap Allocation & Zero Table Duplication**: Reuses `@cap/mime-types` (`lookup`, `extension`, `charset`, `isCompressible`) directly.
- **Cross-Platform Path Parsing**: Handles POSIX (`/`) and Windows (`\`) paths, dot-directories (`/a.b/file`), hidden dotfiles (`.gitignore`, `.env`), and multi-extension names (`archive.tar.gz` -> `gz`).
- **Rust `mime_guess` Ergonomics**: `MimeGuess.fromPath`, `MimeGuess.fromExt`, `first`, `firstOrOctetStream`, `firstOrTextPlain`, `isText`, `isImage`, `isAudio`, `isVideo`, `isCompressible`.

## Usage

```typescript
import {
    MimeGuess,
    extractExt,
    getExtForMime,
    getCharsetForMime,
} from "@cap/mime-guess";

const g = MimeGuess.fromPath("/var/www/assets/LOGO.PNG");
const mime: string = g.firstOrOctetStream(); // "image/png"
const img: bool = g.isImage();               // true

const fallback: string = MimeGuess.fromPath(".gitignore").firstOrTextPlain(); // "text/plain"
const ext = getExtForMime("application/json"); // "json"
const cs = getCharsetForMime("text/html");     // "UTF-8"
```

## License

MIT
