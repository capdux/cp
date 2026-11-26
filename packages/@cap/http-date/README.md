# `@cap/http-date`

Zero-heap-allocation RFC 9110 Section 5.6.7 HTTP-date parser and formatter in pure CP.

## Supported Formats

1. **IMF-fixdate**: `Sun, 06 Nov 1994 08:49:37 GMT`
2. **RFC 850**: `Sunday, 06-Nov-94 08:49:37 GMT`
3. **ANSI C `asctime()`**: `Sun Nov  6 08:49:37 1994`

## API

```cp
import {
    HttpDateTime,
    parseHttpDate,
    toUnixSeconds,
    fromUnixSeconds,
    formatImfFixdate,
    formatUnixHttpDate,
    parseUnixSeconds,
} from "@cap/http-date";
```

## License

MIT
