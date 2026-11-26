# @cap/strip-json-comments

Zero-allocation JSONC (`//` & `/* ... */`) comment and trailing-comma stripper with line/column-preserving whitespace mode for CP — clean-room MIT parity with npm `strip-json-comments` (150M+/wk).

纯 CP 零堆分配 JSONC 预处理引擎（如 `tsconfig.json` / `cap.json`），严格保护字符串内 `//` 与 `\"` 转义，支持空格替换以 100% 保持原行号与列号。

## Install / 安装

```bash
cap add @cap/strip-json-comments
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `StripOptions` | `struct { preserve_whitespace: bool, strip_trailing_commas: bool }` | Stripping mode configuration |
| `stripComments` | `(src: []const u8, dst: []u8, opts: StripOptions) -> i64` | Strip comments/trailing commas into `dst` (`-1` on overflow) |
| `stripJsonc` | `(src: []const u8, dst: []u8) -> i64` | Default `preserve_whitespace = true, strip_trailing_commas = true` |
| `stripCompact` | `(src: []const u8, dst: []u8) -> i64` | Compact `preserve_whitespace = false, strip_trailing_commas = true` |

## Usage / 使用示例

```cp
import { stripJsonc, stripCompact } from "@cap/strip-json-comments";

let out: [256]u8 = undefined;
const n = stripCompact("{\"a\": 1, /* note */ }", out[0..256]);
// out[0..n] == "{\"a\": 1  }"
```
