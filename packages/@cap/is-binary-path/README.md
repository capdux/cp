# `@cap/is-binary-path`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:is-binary-path` ([https://github.com/sindresorhus/is-binary-path](https://github.com/sindresorhus/is-binary-path))

Zero-heap binary/text file path classifier and batch filter in pure CP (clean-room counterpart to npm `is-binary-path`). Directly composes `@cap/binary-extensions` without duplicating extension tables.

## Features

- **Real Cross-Package Composition**: Directly imports `isBinaryPath`, `isBinaryExtension`, and `extensionCategory` from `@cap/binary-extensions`.
- **100% Zero Heap Allocation**: Batch filtering writes matching indices into caller-provided `out_indices: []usize`.
- **High-Level File Filtering API**: `isBinaryPath`, `isTextPath`, `filterBinaryPaths`, `filterTextPaths`, `countBinaryPaths`, `detectPathKind`.

## Quick Start

```cp
import { isBinaryPath, isTextPath, filterBinaryPaths, detectPathKind } from "@cap/is-binary-path";

const files: [3]string = ["src/main.cp", "assets/logo.png", "models/qwen.gguf"];
let bin_idx: [3]usize = undefined;
const n = filterBinaryPaths(&files, &bin_idx); // 2 ([1, 2])
```

## License

MIT
