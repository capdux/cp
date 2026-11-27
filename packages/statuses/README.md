# @cap/statuses

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:statuses` ([https://github.com/jshttp/statuses](https://github.com/jshttp/statuses))

Zero-heap RFC 9110 / IANA HTTP status code lookup, reason phrases, reverse lookup, and category predicates in pure CP (npm `statuses` / Rust `http::StatusCode` parity).

## Features

- `message(code)`: binary-search lookup across all 62 IANA registered 1xx–5xx status codes
- `codeFromMessage(msg)`: case-insensitive reverse lookup (`"Not Found"` -> `404`, `"ok"` -> `200`)
- Predicates: `isValid`, `isInformational`, `isSuccess`, `isRedirect`, `isEmpty`, `isRetry`, `isClientError`, `isServerError`

## Usage

```cp
import { message, codeFromMessage, isRetry } from "@cap/statuses";

const msg = message(429);               // "Too Many Requests"
const code = codeFromMessage("not found"); // 404
const retry = isRetry(503);             // true
```
