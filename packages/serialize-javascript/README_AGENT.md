# serialize-javascript@6.0.2 (Agent Quickref)

> Zero-heap XSS-safe JSON/JavaScript literal serializer escaping HTML script breakouts and line separators | License: BSD-3-Clause
> Import: `import { ... } from "serialize-javascript";` | Full types: `src/index.d.cp`

## Functions
- `export fn escapeUnsafeJson(json_text: string, out: []u8): usize;`
- `export fn serializeString(s: string, out: []u8): usize;`
- `export fn hasUnsafeScriptChars(s: string): bool;`
