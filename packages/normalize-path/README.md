[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/normalize-path`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:normalize-path` ([https://github.com/jonschlinkert/normalize-path](https://github.com/jonschlinkert/normalize-path))

Zero-heap POSIX/Windows path normalizer in pure CP (clean-room counterpart to npm `normalize-path`).

## Features

- **100% Zero Heap Allocation**: Writes normalized POSIX paths directly into caller-provided `out: []u8`.
- **Full `normalize-path` Semantics**: Converts `\` to `/`, collapses duplicate slashes, preserves UNC (`//server/share`) and Windows device prefixes (`//?/`), and preserves root `/` and drive roots `C:/`.
- **Configurable Trailing Slash**: `normalizePath(path, strip_trailing, out)` and `normalizePathDefault(path, out)`.
- **Fast Predicates**: `hasBackslash(path)` and `isUncPath(path)`.

## Quick Start

```cp
import { normalizePath, normalizePathDefault } from "@cap/normalize-path";

let buf: [128]u8 = undefined;
const n1 = normalizePathDefault("foo\\bar\\\\baz\\", &buf); // "foo/bar/baz"
const n2 = normalizePath("\\\\server\\share\\dir\\", true, &buf); // "//server/share/dir"
```

## License

MIT
