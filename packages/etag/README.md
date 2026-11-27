[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `etag`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:etag` ([https://github.com/jshttp/etag](https://github.com/jshttp/etag))

Zero-heap-allocation RFC 9110 HTTP `ETag` generator, parser, and `If-None-Match` / `If-Match` evaluator in pure CP.

## Features

- **Strict Parsing (`parseEtag`)**: Parses strong `"abc"` and weak `W/"abc"` ETags; rejects lowercase `w/`, missing quotes, and illegal `etagc` bytes.
- **RFC 9110 Comparisons**: `strongEqual` and `weakEqual`.
- **Generators**: `generateStatEtag` (`W/"<hex-size>-<hex-mtime>"`) and deterministic FNV-1a `generateContentEtag`.
- **Conditional Header Matchers**: `matchIfNoneMatch` (weak) and `matchIfMatch` (strong), supporting `*` and comma-separated lists.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    EntityTag,
    parseEtag,
    formatEtag,
    strongEqual,
    weakEqual,
    generateStatEtag,
    generateContentEtag,
    matchIfNoneMatch,
    matchIfMatch,
} from "etag";
```

## License

MIT
