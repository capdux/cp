[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/serialize-javascript

Zero-heap XSS-safe JavaScript/JSON literal serializer in pure CP escaping `<`, `>`, `/`, U+2028, and U+2029 for safe inline `<script>` embedding.

## API

- `escapeUnsafeJson(json_text: string, out: []u8): usize`
- `serializeString(s: string, out: []u8): usize`
- `hasUnsafeScriptChars(s: string): bool`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`serialize-javascript@6.0.2`)
- **Upstream Repository**: https://github.com/yahoo/serialize-javascript
- **License**: BSD-3-Clause (clean-room CP implementation)
