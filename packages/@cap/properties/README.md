# @cap/properties

Zero-heap Java / Spring Boot / Android / i18n `.properties` configuration parser and formatter in pure CP.

## Features

- `Properties.parse(src)`: zero-heap static storage (`MAX_PROPS = 128`, `VAL_BUF_CAP = 2048`)
- Supports `=`, `:`, and whitespace key-value separators, `#` and `!` comment lines, and `\` multiline continuations
- Full escape decoding: `\n`, `\r`, `\t`, `\\`, `\=`, `\:`, and `\uXXXX` Unicode escapes into UTF-8
- Last-wins duplicate key semantics (`get`, `has`, `len`)
- `formatEntry(key, value, dst)`: spec-compliant escaping for keys and values

## Usage

```cp
import { Properties, formatEntry } from "@cap/properties";

const src = "app.name = Cap\napp.title : \\u4E2D\\u6587\n";
const props = Properties.parse(src);
const title = props.get("app.title"); // "中文"
```
