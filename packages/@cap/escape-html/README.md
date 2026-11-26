# `@cap/escape-html`

Zero-heap single-pass HTML entity escaper and unescaper in pure CP (clean-room counterpart to npm `escape-html`).

## Features

- **100% Zero Heap Allocation**: Escapes and unescapes directly into caller-provided `out: []u8`.
- **Standard 5-Char Escaping**: `&` -> `&amp;`, `<` -> `&lt;`, `>` -> `&gt;`, `"` -> `&quot;`, `'` -> `&#39;`.
- **Exact Length Pre-Calculation**: `escapedHtmlLen(input)` and `needsHtmlEscape(input)`.
- **Safe Unescaping**: `unescapeHtml` decodes `&amp;`, `&lt;`, `&gt;`, `&quot;`, `&#39;`, `&#x27;`, `&apos;` while preserving unknown entities verbatim.

## Quick Start

```cp
import { escapeHtml, unescapeHtml, escapedHtmlLen } from "@cap/escape-html";

let buf: [128]u8 = undefined;
const n = escapeHtml("<a href=\"/\">A & B</a>", &buf);
// "&lt;a href=&quot;/&quot;&gt;A &amp; B&lt;/a&gt;"
```

## License

MIT
