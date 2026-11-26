# `@cap/binary-extensions`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:binary-extensions` ([https://github.com/sindresorhus/binary-extensions](https://github.com/sindresorhus/binary-extensions))

Zero-heap O(log n) lookup of 127 common binary file extensions and categories in pure CP (clean-room counterpart to npm `binary-extensions` & `is-binary-path`).

## Features

- **100% Zero Heap Allocation**: Static sorted table of 127 binary extensions across 8 categories (Image, Audio, Video, Archive, Font, Executable, Document, Data/ML including `.onnx`, `.safetensors`, `.gguf`, `.parquet`, `.wasm`).
- **O(log n) Case-Insensitive Lookup**: `isBinaryExtension` accepts both `".png"` and `"PNG"`.
- **Path-Aware Helper**: `isBinaryPath` handles POSIX `/` and Windows `\` paths, ignores dots in parent directories (`dir.zip/readme.txt` -> `false`), and ignores dotfiles (`.gitignore` -> `false`).
- **Category Classification**: `extensionCategory` returns `CAT_NONE`..`CAT_DATA_ML`.

## Quick Start

```cp
import { isBinaryExtension, isBinaryPath, extensionCategory, CAT_DATA_ML } from "@cap/binary-extensions";

const ok1 = isBinaryExtension(".PNG"); // true
const ok2 = isBinaryPath("C:\\models\\llama.gguf"); // true
const cat = extensionCategory("safetensors"); // CAT_DATA_ML (8)
```

## License

MIT
