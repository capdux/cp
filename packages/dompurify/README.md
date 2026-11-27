[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`dompurify`](https://github.com/cure53/DOMPurify)（`Apache-2.0`）（包名 `@cap/dompurify`）。

# `@cap/dompurify`

Zero-allocation HTML sanitizer in pure CP (clean-room Apache-2.0 implementation inspired by npm `DOMPurify`).

## Features

- **`SanitizeHtmlOptions` / `defaultSanitizeOptions()`** — Configure `allow_links`, `allow_images`, and `strip_comments`.
- **`sanitizeHtml(html, opts, out)`** — Removes `<script>`, `<style>`, `<iframe>`, `<object>`, `<embed>`, and `<form>` tags with their content; strips all `on*` event handler attributes; blocks `javascript:`, `vbscript:`, and `data:` URIs in `href`/`src`; preserves safe formatting/structure tags and safe attributes.
- **`isSafeHtml(html)`** — Zero-allocation validation predicate.

## Usage

```cp
import { SanitizeHtmlOptions, defaultSanitizeOptions, sanitizeHtml, isSafeHtml } from "@cap/dompurify";
```

## License

Apache-2.0
