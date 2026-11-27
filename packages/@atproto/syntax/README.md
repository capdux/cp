[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@atproto/syntax`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@atproto/syntax` ([https://github.com/bluesky-social/atproto](https://github.com/bluesky-social/atproto)) / `crates.io:atrium-api` ([https://github.com/sugyan/atrium](https://github.com/sugyan/atrium))（原库名 `@atproto/syntax` → CP 包名 `@atproto/syntax`）

Zero-heap-allocation AT Protocol / Bluesky `at://` URI, NSID, and Record Key (`rkey`) parser, validator, and formatter in pure CP.

## Features

- **`parseAtUri`**: Zero-copy parsing of `at://<authority>[/<collection>[/<rkey>]]` with DID/handle detection, NSID validation, and rkey validation.
- **`isValidNsid`**: Strict ATProto NSID grammar validator (`<= 317` chars, >= 3 segments).
- **`isValidRkey`**: Record key validator (1..512 chars, rejects `.` / `..`).
- **`formatAtUri`**: Zero-heap `at://` URI serializer.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { AtUri, isValidNsid, isValidRkey, parseAtUri, formatAtUri } from "@atproto/syntax";
```

## License

MIT
