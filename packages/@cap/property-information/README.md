# @cap/property-information

Zero-allocation HTML/SVG attribute and DOM property name mapping library in pure CP.

## API

- `PropInfo { property: string, attribute: string, is_boolean: bool, is_space_separated: bool, is_comma_separated: bool, is_numeric: bool, found: bool }`
- `findHtmlProp(name: string): PropInfo` — case-insensitive bidirectional lookup for HTML/SVG attributes and DOM property names (including `aria-*` and `data-*`).
- `isBooleanAttr(name: string): bool`
- `isSpaceSeparatedAttr(name: string): bool`
- `isCommaSeparatedAttr(name: string): bool`
- `attrToDomProp(attr: string): string`
- `domPropToAttr(prop: string): string`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`property-information@6.5.0`)
- **Upstream Repository**: https://github.com/wooorm/property-information
- **License**: MIT (clean-room CP implementation)
