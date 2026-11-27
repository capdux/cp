[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `escape-html`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:escape-html` ([https://github.com/component/escape-html](https://github.com/component/escape-html)) / `npm:entities` ([https://github.com/fb55/entities](https://github.com/fb55/entities))

Zero-heap single-pass HTML entity escaper and unescaper in pure CP (clean-room counterpart to npm `escape-html`).

## Features

- **100% Zero Heap Allocation**: Escapes and unescapes directly into caller-provided `out: []u8`.
- **Standard 5-Char Escaping**: `&` -> `&amp;`, `<` -> `&lt;`, `>` -> `&gt;`, `"` -> `&quot;`, `'` -> `&#39;`.
- **Exact Length Pre-Calculation**: `escapedHtmlLen(input)` and `needsHtmlEscape(input)`.
- **Safe Unescaping**: `unescapeHtml` decodes `&amp;`, `&lt;`, `&gt;`, `&quot;`, `&#39;`, `&#x27;`, `&apos;` while preserving unknown entities verbatim.

## Quick Start

```cp
import { escapeHtml, unescapeHtml, escapedHtmlLen } from "escape-html";

let buf: [128]u8 = undefined;
const n = escapeHtml("<a href=\"/\">A & B</a>", &buf);
// "&lt;a href=&quot;/&quot;&gt;A &amp; B&lt;/a&gt;"
```

## License

MIT
