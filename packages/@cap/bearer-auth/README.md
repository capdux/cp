# `@cap/bearer-auth`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:express-bearer-token` ([https://github.com/tkellen/js-express-bearer-token](https://github.com/tkellen/js-express-bearer-token))（原库名 `express-bearer-token` → CP 包名 `@cap/bearer-auth`）

Zero-heap-allocation RFC 6750 HTTP Bearer token extractor, JWT structure precheck, constant-time verifier, and `WWW-Authenticate` challenge formatter in pure CP.

## Features

- **`extractBearer` & `isValidTokenChars`**: Case-insensitive `Bearer` scheme parsing with strict RFC 6750 `b64token` validation.
- **`formatBearer`**: Formats `Bearer <token>` into caller buffers.
- **`isJwtLike`**: Zero-copy 3-segment base64url JWT structure precheck.
- **`constantTimeEq` & `verifyBearer`**: Constant-time token verification resistant to timing side-channels.
- **`formatChallenge`**: Generates RFC 6750 `WWW-Authenticate: Bearer realm="...", error="...", error_description="..."` with quote escaping.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    isValidTokenChars,
    extractBearer,
    formatBearer,
    isJwtLike,
    constantTimeEq,
    verifyBearer,
    formatChallenge,
} from "@cap/bearer-auth";
```

## License

MIT
