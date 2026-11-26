# `@cap/quoted-printable`

Zero-heap-allocation RFC 2045 Section 6.7 MIME Quoted-Printable encoder and decoder in pure CP.

## Features

- **`encode`**: Strict RFC 2045 encoding with `=XX` uppercase hex escapes, mandatory end-of-line space/tab encoding (`=20`/`=09`), and automatic 76-column soft line wrapping (`=\r\n`).
- **`decode`**: Strips `=\r\n` and `=\n` soft line breaks and decodes case-insensitive `=XX` escapes, rejecting invalid `=` sequences.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { encode, decode } from "@cap/quoted-printable";
```

## License

MIT
