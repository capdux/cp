# `@cap/mailto-uri`

Zero-heap-allocation RFC 6068 `mailto:` URI parser, recipient counter, email validator, and percent-encoding builder in pure CP.

## Features

- **`parseMailto`**: Case-insensitive `mailto:` parser extracting `to`, `cc`, `bcc`, `subject`, and `body` (including `?to=` fallback).
- **`countRecipients` & `isBasicEmail`**: Zero-copy comma-separated recipient counting and lightweight email syntax validation.
- **`formatMailto`**: Serializes `mailto:<to>?subject=...&body=...&cc=...` with RFC 3986 `%20` percent-encoding.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    MailtoUri,
    parseMailto,
    countRecipients,
    formatMailto,
    isBasicEmail,
} from "@cap/mailto-uri";
```

## License

MIT
