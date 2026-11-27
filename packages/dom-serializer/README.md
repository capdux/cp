[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/dom-serializer

Zero-heap HTML/XML DOM node serializer in pure CP (covers `dom-serializer`, `domelementtype`, and `w3c-xmlserializer`).

## API

- `DomNodeKind` enum (`Element`, `Text`, `Comment`, `Cdata`, `Doctype`)
- `DomAttr { name: string, value: string }`
- `SerializeOptions { xml_mode: bool, self_closing_tags: bool }`
- `isVoidElement(tag: string): bool`
- `serializeText(text: string, out: []u8): usize`
- `serializeOpenTag(tag: string, attrs: []const DomAttr, self_close: bool, out: []u8): usize`
- `serializeElement(tag: string, attrs: []const DomAttr, inner_html: string, opts: SerializeOptions, out: []u8): usize`
- `serializeComment(text: string, out: []u8): usize`
- `serializeCdata(text: string, out: []u8): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`dom-serializer@2.0.0`, `domelementtype@2.3.0`, `w3c-xmlserializer@5.0.0`)
- **Upstream Repository**: https://github.com/cheeriojs/dom-serializer
- **License**: MIT (clean-room CP implementation)
