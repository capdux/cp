[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/language-tags

Zero-heap BCP 47 / RFC 5646 language tag parser, canonical formatter, and RFC 4647 basic range matcher in pure CP.

## API

- `Bcp47Tag` — parsed tag components (`language`, `script`, `region`, `variant`, `valid`)
- `parseLanguageTag(tag): Bcp47Tag` — parses BCP 47 tags (`en`, `en-US`, `zh-Hans-CN`, `es-419`, `de-DE-1996`, etc.) with `-` or `_` separators
- `formatCanonicalTag(t, out): usize` — formats to canonical BCP 47 casing (`language` lower, `Script` title, `REGION` upper)
- `matchesLanguageRange(tag, range): bool` — RFC 4647 basic prefix matching with `*` wildcard

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`language-tags@1.0.9`)
- **Upstream Repository**: https://github.com/mattcg/language-tags
- **License**: MIT (clean-room CP implementation)
