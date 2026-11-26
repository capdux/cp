# `@cap/jwt-parse`

Zero-heap-allocation RFC 7519 JWT segment splitter, Base64URL decoder, `exp`/`nbf` leeway validator, and `alg:none` guard in pure CP.

## Features

- **`splitToken`**: Zero-copy validation and extraction of `header_b64`, `payload_b64`, `signature_b64`, and `signing_input`.
- **`decodeBase64Url` / `decodeHeader` / `decodePayload`**: Unpadded RFC 4648 Base64URL decoding into caller buffers.
- **`isExpired` / `isNotYetValid`**: Clock-skew leeway aware `exp` and `nbf` validation.
- **`isAlgNone`**: Case-insensitive detection of unsecured `"alg":"none"` JWT headers.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    JwtParts,
    splitToken,
    decodeBase64Url,
    decodeHeader,
    decodePayload,
    isExpired,
    isNotYetValid,
    isAlgNone,
} from "@cap/jwt-parse";
```

## License

MIT
