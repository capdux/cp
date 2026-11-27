[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `fresh`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fresh` ([https://github.com/jshttp/fresh](https://github.com/jshttp/fresh))

Zero-heap-allocation RFC 9110 HTTP conditional request freshness testing in pure CP.

## Features

- **`isFresh(req, res)`**: Strict RFC 9110 §13.2.2 evaluation for deciding `304 Not Modified`.
- **ETag Matching**: `etagWeakMatch` & `etagStrongMatch`, comma-separated `If-None-Match` lists, and `*` wildcard.
- **HTTP Date Parsing**: `parseHttpDate` converts RFC 9110 IMF-fixdate strings to Unix timestamps with leap-year validation.
- **`Cache-Control` Inspection**: Case-insensitive `hasNoCache` with directive-boundary awareness.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    RequestHeaders,
    ResponseHeaders,
    hasNoCache,
    etagWeakMatch,
    etagStrongMatch,
    parseHttpDate,
    isFresh,
} from "fresh";
```

## License

MIT
