# `cap:html_escape` — HTML Entity Escaper & Unescaper

Zero-allocation HTML entity escaper (`&amp;`, `&lt;`, `&gt;`, `&quot;`, `&#x27;`) and named/numeric character reference (`&#39;`, `&#x1F600;`) UTF-8 unescaper.

## Exports

- `needsHtmlEscape(src: []const u8): bool`
- `escapeHtml(src: []const u8, out: []u8): ([]const u8)?`
- `unescapeHtml(src: []const u8, out: []u8): ([]const u8)?`
