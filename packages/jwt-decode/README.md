[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `jwt-decode`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jwt-decode` ([https://github.com/auth0/jwt-decode](https://github.com/auth0/jwt-decode)) / `crates.io:jsonwebtoken` ([https://github.com/Keats/jsonwebtoken](https://github.com/Keats/jsonwebtoken))（原库名 `jwt-decode` → CP 包名 `jwt-decode`）

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
} from "jwt-decode";
```

## License

MIT
