# `@cap/encodeurl`

Zero-heap-allocation RFC 3986 non-double-encoding URL encoder and RegExp metacharacter escaper in pure CP.

## Features

- **`encodeUrl`**: Preserves valid RFC 3986 URI characters and existing `%XX` sequences; encodes unpaired `%` to `%25` and spaces, control characters (CRLF), `< > " \ ^ \` { | }`, and non-ASCII UTF-8 bytes to uppercase `%XX`.
- **`isEncodedUrl`**: Zero-copy predicate verifying whether a URL requires no further encoding.
- **`escapeRegExp`**: Escapes all RegExp metacharacters (`\ ^ $ * + ? . ( ) | { } [ ] -`).
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { encodeUrl, isEncodedUrl, escapeRegExp } from "@cap/encodeurl";
```

## License

MIT
