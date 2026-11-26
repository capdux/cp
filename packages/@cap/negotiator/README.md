# `@cap/negotiator`

Zero-heap RFC 7231 Section 5.3 HTTP content negotiation in pure CP (clean-room counterpart to npm `negotiator`).

## Features

- **100% Zero Heap Allocation**: Parses comma-separated `Accept`, `Accept-Encoding`, `Accept-Language`, and `Accept-Charset` headers into stack slices.
- **Full Quality & Specificity Ranking**: Supports `q=0.000..1.000` (milli-units `0..1000`), case-insensitive `Q=`, explicit `q=0` rejection, and specificity hierarchy (`exact` > `type/*` > `*/*`).
- **Four Core APIs**: `parseAcceptList`, `bestEncoding`, `bestMediaType`, and `bestLanguage` (with BCP-47 prefix fallback `en-US` <-> `en`).

## Quick Start

```cp
import { bestEncoding, bestMediaType, bestLanguage } from "@cap/negotiator";

const encs: [2]string = ["br", "gzip"];
const enc_idx = bestEncoding("gzip, deflate, br;q=0.9", &encs); // 1 ("gzip")

const types: [2]string = ["text/plain", "application/json"];
const media_idx = bestMediaType("text/*;q=0.8, application/json;q=0.9", &types); // 1
```

## License

MIT
