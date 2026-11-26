# `@cap/digest-auth`

Zero-heap-allocation RFC 7616 / RFC 2617 HTTP Digest Authentication challenge & authorization parser and formatter in pure CP.

## Features

- **`parseChallenge` / `formatChallenge`**: Handles `realm`, `nonce`, `opaque`, `qop`, `algorithm`, and `stale=true` with quoted comma/escape support.
- **`parseAuthorization` / `formatAuthorization`**: Handles `username`, `realm`, `nonce`, `uri`, `response`, `qop`, `nc`, `cnonce`, and `opaque` with required-field enforcement.
- **`supportsQop`**: Case-insensitive matching over comma-separated `qop` lists (`"auth, auth-int"`).
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    DigestChallenge,
    DigestResponse,
    parseChallenge,
    parseAuthorization,
    supportsQop,
    formatAuthorization,
    formatChallenge,
} from "@cap/digest-auth";
```

## License

MIT
