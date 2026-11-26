# @cap/iri-string

Zero-heap-allocation RFC 3986 / RFC 3987 URI/IRI parser, dot-segment normalizer, and relative reference resolver in pure CP (inspired by Rust `iri-string`).

## Features

- `parseIri(s: string): IriParts`: zero-copy span decomposition into `scheme`, `authority`, `path`, `query`, `fragment` with RFC 3987 UTF-8 IRI support and strict scheme/control-char validation
- `removeDotSegments(path: string, out: []u8): usize`: exact RFC 3986 Section 5.2.4 dot-segment removal
- `resolveReference(base: string, rel: string, out: []u8): usize`: exact RFC 3986 Section 5.2.2 / 5.2.3 relative reference resolution

## License

MIT
