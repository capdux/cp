# @cap/dot-prop

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:lodash` ([https://github.com/lodash/lodash](https://github.com/lodash/lodash)) / `npm:dot-prop` ([https://github.com/sindresorhus/dot-prop](https://github.com/sindresorhus/dot-prop))（原库名 `lodash` → CP 包名 `@cap/dot-prop`）

Zero-allocation dot-path parser and deep JSON path reader for CP — clean-room MIT parity with npm `dot-prop` + `lodash.get` / `dlv` (~280M/wk).

纯 CP 零堆分配 JSON 点路径解析与深度读取引擎，支持 `a.b[0].c`、`a.0.b`、转义键 `config.a\.b.value` 直接零拷贝查询原始 JSON 文本，无需构建堆 AST。

## Install / 安装

```bash
cap add @cap/dot-prop
```

## Features / 核心特性

- **Zero-Allocation Path Parser**: Supports dotted keys (`user.profile.name`), bracket indices (`items[0].tags[2]`), numeric dot indices (`a.0.b`), and backslash-escaped dots/brackets (`config.a\.b.value`).
- **Direct Raw JSON Querying**: Traverses raw JSON strings in-place without building an AST, correctly skipping nested objects/arrays and escaped quotes inside strings.
- **Safe Boundary Handling**: Gracefully returns `null` on missing keys, out-of-bounds array indices, type mismatches, or malformed paths/JSON.

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `PathSegment` | `struct { is_index: bool, key: string, index: usize }` | Parsed path segment |
| `ParsedDotPath` | `struct { segs: [32]PathSegment, count: usize, valid: bool }` | Fixed-capacity parsed path |
| `parseDotPath` | `(path: string, key_buf: []u8) -> ParsedDotPath` | Parse dot/bracket path with escape unescaping into `key_buf` |
| `getJsonPath` | `(json: string, path: string) -> string?` | Query raw JSON value slice at `path` |
| `getJsonString` | `(json: string, path: string) -> string?` | Query JSON string value and strip outer quotes |
| `hasJsonPath` | `(json: string, path: string) -> bool` | Check if `path` exists in `json` |
| `escapePathSegment` | `(seg: string, out: []u8) -> string` | Escape `.`, `[`, `]`, `\` in a property segment |

## Usage / 使用示例

```cp
import { getJsonPath, getJsonString, hasJsonPath } from "@cap/dot-prop";

const doc = "{\"user\":{\"name\":\"Alice\"},\"items\":[{\"tags\":[\"cp\",\"zig\"]}],\"a.b\":{\"ok\":true}}";
const name = getJsonString(doc, "user.name");       // "Alice"
const tag = getJsonString(doc, "items[0].tags[1]"); // "zig"
const ok = getJsonPath(doc, "a\\.b.ok");            // "true"
const exists = hasJsonPath(doc, "items.0.tags.0");  // true
```
