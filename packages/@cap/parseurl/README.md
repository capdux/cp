# `@cap/parseurl`

Zero-heap, zero-copy HTTP request URL parser in pure CP (clean-room counterpart to npm `parseurl`).

## Features

- **100% Zero Heap Allocation & Zero Copy**: Returns exact byte-range spans (`ParsedUrl`) directly into the input URL string.
- **Fast Path & Full URI Support**: Handles relative request targets (`/users/42?sort=asc`), `#hash` truncation, empty queries (`/a?`), and full absolute URIs (`https://host:8443/v2/users?id=1`).
- **Security Guards**: Rejects empty inputs and ASCII control characters (`< 0x20`, `0x7F`), plus fast `hasDotDotSegment` path-traversal detection and `stripQueryAndHash`.

## Quick Start

```cp
import { parseUrl, stripQueryAndHash, hasDotDotSegment } from "@cap/parseurl";

const u = "/search?q=cp&lang=zh#top";
const p = parseUrl(u);
const pathname = stripQueryAndHash(u); // "/search"
const safe = !hasDotDotSegment(pathname); // true
```

## License

MIT
